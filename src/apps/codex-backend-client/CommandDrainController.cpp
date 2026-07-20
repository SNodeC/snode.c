/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include "apps/codex-backend-client/CommandDrainController.h"

#include <algorithm>
#include <exception>
#include <utility>
#include <variant>

namespace apps::codex_backend_client {

    namespace {
        namespace frontend = ai::openai::codex::frontend;

        bool waitsForSync(const frontend::Command& command) noexcept {
            return std::holds_alternative<frontend::SnapshotGet>(command.parameters) ||
                   std::holds_alternative<frontend::ReplayAfter>(command.parameters);
        }
    } // namespace

    CommandDrainController::CommandDrainController(CommandDrainCallbacks callbacks)
        : callbacks(std::move(callbacks)) {
    }

    bool CommandDrainController::enqueue(frontend::ClientMessage message) {
        if (currentOutcome != Outcome::Running || currentInputState != InputState::Reading) {
            return false;
        }

        if (currentSessionState != SessionState::Ready || !queuedMessages.empty()) {
            queuedMessages.push_back(std::move(message));
            if (currentSessionState == SessionState::Ready) {
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

    void CommandDrainController::connected() {
        if (currentOutcome != Outcome::Running || currentSessionState != SessionState::Connecting) {
            return;
        }
        currentSessionState = SessionState::Synchronizing;
    }

    void CommandDrainController::receive(const frontend::ServerMessage& message) {
        if (currentOutcome != Outcome::Running) {
            return;
        }

        if (const auto* protocolError = std::get_if<frontend::ProtocolErrorMessage>(&message)) {
            if (currentInputState == InputState::DrainOnEof || protocolError->closeConnection) {
                fail("frontend protocol error: " + protocolError->message);
            }
            return;
        }

        if (const auto* response = std::get_if<frontend::Response>(&message)) {
            completeResponse(*response);
            return;
        }

        if (!std::holds_alternative<frontend::SyncComplete>(message)) {
            return;
        }

        if (currentSessionState == SessionState::Synchronizing) {
            currentSessionState = SessionState::Ready;
            flushQueued();
            maybeCompleteDrain();
            return;
        }

        if (currentSessionState == SessionState::Ready) {
            completeSync();
        }
    }

    void CommandDrainController::inputEof() {
        if (currentOutcome != Outcome::Running || currentInputState != InputState::Reading) {
            return;
        }
        currentInputState = InputState::DrainOnEof;
        maybeCompleteDrain();
    }

    void CommandDrainController::inputFailed(std::string message) {
        fail(std::move(message));
    }

    void CommandDrainController::connectionFailed(std::string message) {
        fail(std::move(message));
    }

    void CommandDrainController::protocolFailed(std::string message) {
        fail(std::move(message));
    }

    void CommandDrainController::disconnected() {
        if (currentSessionState == SessionState::Closed) {
            return;
        }
        if (currentOutcome == Outcome::Running) {
            fail(currentInputState == InputState::DrainOnEof ? "frontend connection closed before command draining completed"
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

    bool CommandDrainController::sendNow(const frontend::ClientMessage& message) {
        std::list<PendingCommand>::iterator tracked = pendingCommands.end();
        if (const auto* command = std::get_if<frontend::Command>(&message)) {
            tracked =
                pendingCommands.insert(pendingCommands.end(), PendingCommand{command->requestId, true, waitsForSync(*command), false});
        }

        bool sent = false;
        try {
            sent = callbacks.send && callbacks.send(message);
        } catch (const std::exception& exception) {
            if (tracked != pendingCommands.end()) {
                pendingCommands.erase(tracked);
            }
            fail("failed to send frontend message: " + std::string(exception.what()));
            return false;
        } catch (...) {
            if (tracked != pendingCommands.end()) {
                pendingCommands.erase(tracked);
            }
            fail("failed to send frontend message");
            return false;
        }

        if (!sent) {
            if (tracked != pendingCommands.end()) {
                pendingCommands.erase(tracked);
            }
            fail("failed to send frontend message");
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
        while (currentOutcome == Outcome::Running && currentSessionState == SessionState::Ready && !queuedMessages.empty()) {
            const auto* command = std::get_if<frontend::Command>(&queuedMessages.front());
            if (command != nullptr && requestIdIsPending(command->requestId)) {
                // Raw commands may deliberately reuse an ID. Serialize that
                // reuse so response correlation stays deterministic.
                return;
            }

            frontend::ClientMessage message = std::move(queuedMessages.front());
            queuedMessages.pop_front();
            if (!sendNow(message)) {
                return;
            }
        }
    }

    void CommandDrainController::completeResponse(const frontend::Response& response) {
        const auto found = std::find_if(pendingCommands.begin(), pendingCommands.end(), [&response](const PendingCommand& pending) {
            return pending.responsePending && pending.requestId == response.requestId;
        });
        if (found == pendingCommands.end()) {
            if (currentInputState == InputState::DrainOnEof) {
                fail("received an uncorrelated frontend response while draining: " + response.requestId);
            }
            return;
        }

        found->responsePending = false;
        found->syncPending = response.ok && found->waitForSyncOnSuccess;
        removeCompletedCommands();
        flushQueued();
        maybeCompleteDrain();
    }

    void CommandDrainController::completeSync() {
        const auto found = std::find_if(pendingCommands.begin(), pendingCommands.end(), [](const PendingCommand& pending) {
            return pending.syncPending;
        });
        if (found != pendingCommands.end()) {
            found->syncPending = false;
            removeCompletedCommands();
            flushQueued();
        }
        maybeCompleteDrain();
    }

    void CommandDrainController::removeCompletedCommands() {
        pendingCommands.remove_if([](const PendingCommand& pending) {
            return !pending.responsePending && !pending.syncPending;
        });
    }

    void CommandDrainController::maybeCompleteDrain() {
        if (currentOutcome == Outcome::Running && currentInputState == InputState::DrainOnEof &&
            currentSessionState == SessionState::Ready && queuedMessages.empty() && pendingCommands.empty()) {
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

} // namespace apps::codex_backend_client
