/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include "apps/codex-backend-client/CommandDrainController.h"

#include "apps/codex-backend-client/CommandParser.h"

#include <algorithm>
#include <exception>
#include <map>
#include <nlohmann/json.hpp>
#include <optional>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace apps::codex_backend_client {

    namespace {
        namespace frontend = ai::openai::codex::frontend;

        bool waitsForSync(const frontend::Command& command) noexcept {
            return std::holds_alternative<frontend::SnapshotGet>(command.parameters) ||
                   std::holds_alternative<frontend::ReplayAfter>(command.parameters);
        }

        ResponsePresentation presentationFor(const frontend::Command& command) {
            ResponsePresentation presentation;
            if (std::holds_alternative<frontend::ThreadStart>(command.parameters)) {
                presentation.kind = ResponsePresentation::Kind::ThreadStart;
            } else if (const auto* resume = std::get_if<frontend::ThreadResume>(&command.parameters)) {
                presentation.kind = ResponsePresentation::Kind::ThreadResume;
                presentation.threadId = resume->threadId;
            } else if (const auto* turn = std::get_if<frontend::TurnStart>(&command.parameters)) {
                presentation.kind = ResponsePresentation::Kind::TurnStart;
                presentation.threadId = turn->threadId;
            }
            return presentation;
        }

        std::optional<std::string> threadIdFromResult(const std::optional<frontend::Json>& result) {
            if (!result.has_value() || !result->is_object()) {
                return std::nullopt;
            }

            const auto thread = result->find("thread");
            if (thread != result->end() && thread->is_object()) {
                const auto id = thread->find("id");
                if (id != thread->end() && id->is_string()) {
                    std::string value = id->get<std::string>();
                    if (!value.empty()) {
                        return value;
                    }
                }
            }

            // BackendAdapter normally returns the normalized thread snapshot
            // above. It deliberately falls back to threadId when the just-
            // completed thread is not present in its current snapshot.
            const auto id = result->find("threadId");
            if (id != result->end() && id->is_string()) {
                std::string value = id->get<std::string>();
                if (!value.empty()) {
                    return value;
                }
            }
            return std::nullopt;
        }

        std::string responseFailure(std::string_view operation, const frontend::Response& response) {
            std::string failure(operation);
            failure += " failed";
            if (response.error.has_value()) {
                failure += ": ";
                failure += frontend::toString(response.error->code);
                if (!response.error->message.empty()) {
                    failure += ": ";
                    failure += response.error->message;
                }
            } else {
                failure += " without error details";
            }
            return failure;
        }
    } // namespace

    CommandDrainController::CommandDrainController(CommandDrainCallbacks callbacks)
        : callbacks(std::move(callbacks)) {
    }

    bool CommandDrainController::enqueue(frontend::ClientMessage message) {
        if (currentOutcome != Outcome::Running || currentInputState != InputState::Reading) {
            return false;
        }

        if (currentSessionState != SessionState::Ready || activeNewCommand.has_value() || !queuedMessages.empty()) {
            queuedMessages.push_back(std::move(message));
            if (currentSessionState == SessionState::Ready && !activeNewCommand.has_value()) {
                flushQueued();
            }
            return currentOutcome == Outcome::Running;
        }

        if (const auto* command = std::get_if<frontend::Command>(&message); command != nullptr && requestIdIsPending(command->requestId)) {
            queuedMessages.push_back(std::move(message));
            return true;
        }

        const bool sent = sendNow(message);
        maybeCompleteDrain();
        return sent;
    }

    bool CommandDrainController::enqueue(NewCommand command) {
        if (currentOutcome != Outcome::Running || currentInputState != InputState::Reading) {
            return false;
        }

        queuedMessages.emplace_back(QueuedNewCommand{std::move(command.threadStartRequestId),
                                                     std::move(command.turnStartRequestId),
                                                     std::move(command.options),
                                                     std::move(command.prompt)});
        if (currentSessionState == SessionState::Ready && !activeNewCommand.has_value()) {
            flushQueued();
        }
        maybeCompleteDrain();
        return currentOutcome == Outcome::Running;
    }

    void CommandDrainController::connected() {
        if (currentOutcome != Outcome::Running || currentSessionState != SessionState::Connecting) {
            return;
        }
        currentSessionState = SessionState::Synchronizing;
    }

    std::optional<ResponsePresentation> CommandDrainController::receive(const frontend::ServerMessage& message) {
        if (currentOutcome != Outcome::Running) {
            return std::nullopt;
        }

        if (const auto* protocolError = std::get_if<frontend::ProtocolErrorMessage>(&message)) {
            const bool matchesActiveNew = activeNewCommand.has_value() && protocolError->requestId.has_value() &&
                                          ((activeNewCommand->stage == NewStage::AwaitingThreadStartResponse &&
                                            *protocolError->requestId == activeNewCommand->command.threadStartRequestId) ||
                                           (activeNewCommand->stage == NewStage::AwaitingTurnStartResponse &&
                                            *protocolError->requestId == activeNewCommand->command.turnStartRequestId));
            if (currentInputState == InputState::DrainOnEof || protocolError->closeConnection || matchesActiveNew) {
                failForActiveNew("frontend protocol error: " + protocolError->message);
            }
            return std::nullopt;
        }

        if (const auto* response = std::get_if<frontend::Response>(&message)) {
            return completeResponse(*response);
        }

        if (!std::holds_alternative<frontend::SyncComplete>(message)) {
            return std::nullopt;
        }

        if (currentSessionState == SessionState::Synchronizing) {
            currentSessionState = SessionState::Ready;
            flushQueued();
            maybeCompleteDrain();
            return std::nullopt;
        }

        if (currentSessionState == SessionState::Ready) {
            completeSync();
        }
        return std::nullopt;
    }

    void CommandDrainController::inputEof() {
        if (currentOutcome != Outcome::Running || currentInputState != InputState::Reading) {
            return;
        }
        currentInputState = InputState::DrainOnEof;
        maybeCompleteDrain();
    }

    void CommandDrainController::inputFailed(std::string message) {
        failForActiveNew(std::move(message));
    }

    void CommandDrainController::connectionFailed(std::string message) {
        failForActiveNew(std::move(message));
    }

    void CommandDrainController::protocolFailed(std::string message) {
        failForActiveNew(std::move(message));
    }

    void CommandDrainController::disconnected() {
        if (currentSessionState == SessionState::Closed) {
            return;
        }
        if (currentOutcome == Outcome::Running) {
            failForActiveNew(currentInputState == InputState::DrainOnEof ? "frontend connection closed before command draining completed"
                                                                         : "frontend connection closed unexpectedly");
        }
        currentSessionState = SessionState::Closed;
    }

    void CommandDrainController::quit() {
        if (currentOutcome != Outcome::Running) {
            return;
        }
        currentInputState = InputState::ImmediateQuit;
        finish(Outcome::Success);
    }

    CommandDrainController::SessionState CommandDrainController::sessionState() const noexcept {
        return currentSessionState;
    }

    CommandDrainController::InputState CommandDrainController::inputState() const noexcept {
        return currentInputState;
    }

    CommandDrainController::Outcome CommandDrainController::outcome() const noexcept {
        return currentOutcome;
    }

    bool CommandDrainController::failed() const noexcept {
        return currentOutcome == Outcome::Failure;
    }

    const std::string& CommandDrainController::failureReason() const noexcept {
        return currentFailureReason;
    }

    std::size_t CommandDrainController::queuedCount() const noexcept {
        return queuedMessages.size();
    }

    std::size_t CommandDrainController::pendingResponseCount() const noexcept {
        return static_cast<std::size_t>(std::count_if(pendingCommands.begin(), pendingCommands.end(), [](const PendingCommand& pending) {
            return pending.responsePending;
        }));
    }

    std::size_t CommandDrainController::pendingSyncCount() const noexcept {
        return static_cast<std::size_t>(std::count_if(pendingCommands.begin(), pendingCommands.end(), [](const PendingCommand& pending) {
            return pending.syncPending;
        }));
    }

    CommandDrainController::NewStage CommandDrainController::newStage() const noexcept {
        if (activeNewCommand.has_value()) {
            return activeNewCommand->stage;
        }
        const bool queued = std::any_of(queuedMessages.begin(), queuedMessages.end(), [](const QueuedEntry& entry) {
            return std::holds_alternative<QueuedNewCommand>(entry);
        });
        return queued ? NewStage::Queued : NewStage::None;
    }

    bool CommandDrainController::sendNow(const frontend::ClientMessage& message, PendingKind kind) {
        std::list<PendingCommand>::iterator tracked = pendingCommands.end();
        if (const auto* command = std::get_if<frontend::Command>(&message)) {
            ResponsePresentation presentation = presentationFor(*command);
            if (kind == PendingKind::NewThreadStart) {
                presentation.kind = ResponsePresentation::Kind::NewThreadStart;
            } else if (kind == PendingKind::NewTurnStart) {
                presentation.kind = ResponsePresentation::Kind::NewTurnStart;
            }
            tracked = pendingCommands.insert(
                pendingCommands.end(),
                PendingCommand{command->requestId, true, waitsForSync(*command), false, kind, std::move(presentation)});
        }

        bool sent = false;
        try {
            sent = callbacks.send && callbacks.send(message);
        } catch (const std::exception& exception) {
            if (tracked != pendingCommands.end()) {
                pendingCommands.erase(tracked);
            }
            const std::string failure = "failed to send frontend message: " + std::string(exception.what());
            if (kind == PendingKind::Ordinary) {
                fail(failure);
            } else {
                failForActiveNew(failure);
            }
            return false;
        } catch (...) {
            if (tracked != pendingCommands.end()) {
                pendingCommands.erase(tracked);
            }
            if (kind == PendingKind::Ordinary) {
                fail("failed to send frontend message");
            } else {
                failForActiveNew("failed to send frontend message");
            }
            return false;
        }

        if (!sent) {
            if (tracked != pendingCommands.end()) {
                pendingCommands.erase(tracked);
            }
            if (kind == PendingKind::Ordinary) {
                fail("failed to send frontend message");
            } else {
                failForActiveNew("failed to send frontend message");
            }
            return false;
        }
        return true;
    }

    bool CommandDrainController::requestIdIsPending(const std::string& requestId) const noexcept {
        return std::any_of(pendingCommands.begin(), pendingCommands.end(), [&requestId](const PendingCommand& pending) {
            return pending.requestId == requestId;
        });
    }

    void CommandDrainController::flushQueued() {
        while (currentOutcome == Outcome::Running && currentSessionState == SessionState::Ready && !activeNewCommand.has_value() &&
               !queuedMessages.empty()) {
            if (auto* message = std::get_if<frontend::ClientMessage>(&queuedMessages.front())) {
                const auto* command = std::get_if<frontend::Command>(message);
                if (command != nullptr && requestIdIsPending(command->requestId)) {
                    // Raw commands may deliberately reuse an ID. Serialize that
                    // reuse so response correlation stays deterministic.
                    return;
                }

                frontend::ClientMessage next = std::move(*message);
                queuedMessages.pop_front();
                if (!sendNow(next)) {
                    return;
                }
                continue;
            }

            auto* queuedNew = std::get_if<QueuedNewCommand>(&queuedMessages.front());
            if (queuedNew == nullptr || requestIdIsPending(queuedNew->threadStartRequestId)) {
                return;
            }

            QueuedNewCommand command = std::move(*queuedNew);
            queuedMessages.pop_front();
            activeNewCommand.emplace(ActiveNewCommand{std::move(command), NewStage::AwaitingThreadStartResponse, std::nullopt});

            frontend::Command start{activeNewCommand->command.threadStartRequestId,
                                    activeNewCommand->command.options,
                                    frontend::Json::object(),
                                    frontend::Json::object()};
            static_cast<void>(sendNow(frontend::ClientMessage{std::move(start)}, PendingKind::NewThreadStart));
            // A compound command remains the active queue head until both
            // protocol operations complete. Later input cannot bypass it.
            return;
        }
    }

    std::optional<ResponsePresentation> CommandDrainController::completeResponse(const frontend::Response& response) {
        const auto found = std::find_if(pendingCommands.begin(), pendingCommands.end(), [&response](const PendingCommand& pending) {
            return pending.responsePending && pending.requestId == response.requestId;
        });
        if (found == pendingCommands.end()) {
            if (currentInputState == InputState::DrainOnEof) {
                failForActiveNew("received an uncorrelated frontend response while draining: " + response.requestId);
            }
            return std::nullopt;
        }

        const PendingKind kind = found->kind;
        ResponsePresentation presentation = found->presentation;
        found->responsePending = false;
        found->syncPending = response.ok && found->waitForSyncOnSuccess;
        removeCompletedCommands();

        if (kind == PendingKind::NewThreadStart) {
            if (!activeNewCommand.has_value() || activeNewCommand->stage != NewStage::AwaitingThreadStartResponse) {
                fail("received a thread.start response without its active new command");
                return presentation;
            }
            if (!response.ok) {
                failForActiveNew(responseFailure("thread.start", response));
                return presentation;
            }

            const std::optional<std::string> threadId = threadIdFromResult(response.result);
            if (!threadId.has_value()) {
                presentation.kind = ResponsePresentation::Kind::Generic;
                failForActiveNew("successful thread.start response did not contain a non-empty thread ID");
                return presentation;
            }

            activeNewCommand->threadId = *threadId;
            activeNewCommand->stage = NewStage::WaitingToSubmitTurn;
            presentation.threadId = *threadId;
            static_cast<void>(submitActiveNewTurn());
            maybeCompleteDrain();
            return presentation;
        }

        if (kind == PendingKind::NewTurnStart) {
            if (!activeNewCommand.has_value() || activeNewCommand->stage != NewStage::AwaitingTurnStartResponse ||
                !activeNewCommand->threadId.has_value()) {
                fail("received a turn.start response without its active new command");
                return presentation;
            }
            presentation.threadId = activeNewCommand->threadId;
            if (!response.ok) {
                failForActiveNew(responseFailure("turn.start", response));
                return presentation;
            }

            activeNewCommand.reset();
            flushQueued();
            maybeCompleteDrain();
            return presentation;
        }

        if (response.ok && (presentation.kind == ResponsePresentation::Kind::ThreadStart ||
                            presentation.kind == ResponsePresentation::Kind::ThreadResume)) {
            if (const std::optional<std::string> threadId = threadIdFromResult(response.result); threadId.has_value()) {
                presentation.threadId = *threadId;
            }
        }
        static_cast<void>(submitActiveNewTurn());
        flushQueued();
        maybeCompleteDrain();
        return presentation;
    }

    void CommandDrainController::completeSync() {
        const auto found = std::find_if(pendingCommands.begin(), pendingCommands.end(), [](const PendingCommand& pending) {
            return pending.syncPending;
        });
        if (found != pendingCommands.end()) {
            found->syncPending = false;
            removeCompletedCommands();
            static_cast<void>(submitActiveNewTurn());
            flushQueued();
        }
        maybeCompleteDrain();
    }

    bool CommandDrainController::submitActiveNewTurn() {
        if (!activeNewCommand.has_value() || activeNewCommand->stage != NewStage::WaitingToSubmitTurn) {
            return true;
        }
        if (!activeNewCommand->threadId.has_value()) {
            failForActiveNew("cannot submit the initial turn without a created thread ID");
            return false;
        }
        if (requestIdIsPending(activeNewCommand->command.turnStartRequestId)) {
            return true;
        }

        frontend::TurnStart turn;
        turn.threadId = *activeNewCommand->threadId;
        turn.input.emplace_back(frontend::TextInput{activeNewCommand->command.prompt, frontend::Json::object()});
        frontend::Command command{
            activeNewCommand->command.turnStartRequestId, std::move(turn), frontend::Json::object(), frontend::Json::object()};
        activeNewCommand->stage = NewStage::AwaitingTurnStartResponse;
        return sendNow(frontend::ClientMessage{std::move(command)}, PendingKind::NewTurnStart);
    }

    void CommandDrainController::removeCompletedCommands() {
        pendingCommands.remove_if([](const PendingCommand& pending) {
            return !pending.responsePending && !pending.syncPending;
        });
    }

    void CommandDrainController::maybeCompleteDrain() {
        if (currentOutcome == Outcome::Running && currentInputState == InputState::DrainOnEof &&
            currentSessionState == SessionState::Ready && queuedMessages.empty() && pendingCommands.empty() &&
            !activeNewCommand.has_value()) {
            finish(Outcome::Success);
        }
    }

    void CommandDrainController::finish(Outcome outcome) {
        if (currentOutcome != Outcome::Running) {
            return;
        }
        currentOutcome = outcome;
        currentSessionState = SessionState::Closing;
        try {
            if (callbacks.requestExit) {
                callbacks.requestExit();
            }
        } catch (...) {
        }
    }

    void CommandDrainController::fail(std::string message) {
        if (currentOutcome != Outcome::Running) {
            return;
        }
        currentFailureReason = std::move(message);
        currentOutcome = Outcome::Failure;
        currentSessionState = SessionState::Closing;
        try {
            if (callbacks.reportFailure) {
                callbacks.reportFailure(currentFailureReason);
            }
        } catch (...) {
        }
        try {
            if (callbacks.requestExit) {
                callbacks.requestExit();
            }
        } catch (...) {
        }
    }

    void CommandDrainController::failForActiveNew(std::string message) {
        if (activeNewCommand.has_value()) {
            if (activeNewCommand->threadId.has_value()) {
                fail("thread created id=" + *activeNewCommand->threadId + ", but initial turn failed: " + message);
            } else {
                fail("new thread creation failed: " + message);
            }
            return;
        }
        fail(std::move(message));
    }

} // namespace apps::codex_backend_client
