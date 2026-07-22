/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include "ai/openai/codex/frontend/Messages.h"
#include "apps/codex-backend-client/CommandDrainController.h"
#include "apps/codex-backend-client/CommandParser.h"
#include "support/TestResult.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace {
    namespace client = apps::codex_backend_client;
    namespace frontend = ai::openai::codex::frontend;

    frontend::ClientMessage command(std::string requestId, frontend::CommandParameters parameters) {
        return frontend::Command{std::move(requestId), std::move(parameters), frontend::Json::object(), frontend::Json::object()};
    }

    frontend::ServerMessage sync(std::uint64_t sequence = 1) {
        return frontend::SyncComplete{frontend::SequenceNumber{sequence}, frontend::Json::object()};
    }

    frontend::ServerMessage response(std::string requestId, bool ok = true) {
        return ok ? frontend::ServerMessage{frontend::Response::success(std::move(requestId))}
                  : frontend::ServerMessage{frontend::Response::failure(
                        std::move(requestId),
                        frontend::CommandError{frontend::ErrorCode::InvalidCommand, "rejected", std::nullopt, frontend::Json::object()})};
    }

    frontend::ServerMessage successfulResponse(std::string requestId, frontend::Json result) {
        return frontend::Response::success(std::move(requestId), std::move(result));
    }

    client::NewCommand
    newCommand(std::string startRequestId, std::string turnRequestId, std::string prompt, frontend::ThreadStart options = {}) {
        return client::NewCommand{std::move(startRequestId), std::move(turnRequestId), std::move(options), std::move(prompt)};
    }

    const frontend::Command* sentCommand(const frontend::ClientMessage& message) {
        return std::get_if<frontend::Command>(&message);
    }

    template <typename Parameters>
    const Parameters* sentParameters(const std::vector<frontend::ClientMessage>& sent, std::size_t index) {
        const frontend::Command* command = index < sent.size() ? sentCommand(sent[index]) : nullptr;
        return command == nullptr ? nullptr : std::get_if<Parameters>(&command->parameters);
    }

    struct Harness {
        Harness()
            : controller(client::CommandDrainCallbacks{.send =
                                                           [this](const frontend::ClientMessage& message) {
                                                               sent.push_back(message);
                                                               ++sendAttempts;
                                                               return sendSucceeds && (!failedSendAttempt.has_value() ||
                                                                                       sendAttempts != *failedSendAttempt);
                                                           },
                                                       .requestExit =
                                                           [this]() {
                                                               ++exitRequests;
                                                           },
                                                       .reportFailure =
                                                           [this](std::string message) {
                                                               failures.push_back(std::move(message));
                                                           }}) {
        }

        void establishAndSynchronize() {
            controller.connected();
            controller.receive(sync());
        }

        bool sendSucceeds = true;
        std::optional<std::size_t> failedSendAttempt;
        std::size_t sendAttempts = 0;
        std::vector<frontend::ClientMessage> sent;
        std::vector<std::string> failures;
        std::size_t exitRequests = 0;
        client::CommandDrainController controller;
    };

    void testPreconnectionQueueAndInteractiveBehavior(tests::support::TestResult& result) {
        Harness harness;
        result.expectTrue(harness.controller.enqueue(command("before-1", frontend::ControllerAcquire{})),
                          "a command arriving before connection is accepted for ordered delivery");
        result.expectTrue(harness.controller.enqueue(command("before-2", frontend::ThreadList{})),
                          "a second preconnection command is accepted");
        result.expectTrue(harness.sent.empty() && harness.controller.queuedCount() == 2,
                          "preconnection commands remain queued instead of being rejected as not connected");

        harness.controller.connected();
        result.expectTrue(harness.sent.empty() &&
                              harness.controller.sessionState() == client::CommandDrainController::SessionState::Synchronizing,
                          "transport connection alone does not release ordinary commands");
        harness.controller.receive(frontend::ServerMessage{frontend::Welcome{"session",
                                                                             frontend::SessionRole::Observer,
                                                                             frontend::SequenceNumber{1},
                                                                             frontend::SyncMode::Snapshot,
                                                                             frontend::Json::object()}});
        harness.controller.receive(
            frontend::ServerMessage{frontend::Snapshot{frontend::SequenceNumber{1}, frontend::Json::object(), frontend::Json::object()}});
        result.expectTrue(harness.sent.empty(), "welcome and snapshot do not release queued commands before sync.complete");

        harness.controller.receive(sync());
        const frontend::Command* first = harness.sent.size() > 0 ? sentCommand(harness.sent[0]) : nullptr;
        const frontend::Command* second = harness.sent.size() > 1 ? sentCommand(harness.sent[1]) : nullptr;
        result.expectTrue(harness.sent.size() == 2 && first != nullptr && second != nullptr && first->requestId == "before-1" &&
                              second->requestId == "before-2",
                          "initial sync.complete sends queued commands exactly once and in original order");
        harness.controller.receive(sync(2));
        result.expectTrue(harness.sent.size() == 2, "an unrelated sync.complete never resends an already delivered command");

        harness.controller.receive(response("before-2"));
        harness.controller.receive(response("before-1"));
        result.expectTrue(harness.exitRequests == 0 && harness.controller.outcome() == client::CommandDrainController::Outcome::Running,
                          "interactive operation stays connected after all responses when stdin remains open");

        result.expectTrue(harness.controller.enqueue(command("interactive", frontend::ThreadRead{"thread-1", std::nullopt})),
                          "an interactive command remains accepted after synchronization");
        result.expectTrue(harness.sent.size() == 3 && sentCommand(harness.sent.back())->requestId == "interactive",
                          "an interactive command is sent immediately after synchronization");
        harness.controller.receive(response("interactive"));
        result.expectTrue(harness.exitRequests == 0, "an interactive response does not trigger drain-and-exit without EOF");
        harness.controller.quit();
        result.expectTrue(harness.exitRequests == 1 && harness.controller.outcome() == client::CommandDrainController::Outcome::Success &&
                              harness.controller.inputState() == client::CommandDrainController::InputState::ImmediateQuit,
                          "explicit quit retains immediate successful shutdown behavior");
    }

    void testEofBeforeConnectionDrainsResponses(tests::support::TestResult& result) {
        Harness harness;
        result.expectTrue(harness.controller.enqueue(command("pipe-acquire", frontend::ControllerAcquire{})),
                          "the first piped command is accepted before connection");
        result.expectTrue(harness.controller.enqueue(command("pipe-threads", frontend::ThreadList{})),
                          "the second piped command is accepted before connection");
        harness.controller.inputEof();
        result.expectTrue(harness.controller.inputState() == client::CommandDrainController::InputState::DrainOnEof &&
                              harness.sent.empty() && harness.exitRequests == 0,
                          "EOF before connection enters drain state without dropping commands or exiting");

        harness.controller.connected();
        result.expectTrue(harness.sent.empty() && harness.exitRequests == 0,
                          "EOF before connection still waits for initial frontend synchronization");
        harness.controller.receive(sync());
        result.expectTrue(harness.sent.size() == 2 && harness.controller.pendingResponseCount() == 2 && harness.exitRequests == 0,
                          "initial synchronization flushes both piped commands and waits for both responses");

        harness.controller.receive(response("pipe-threads"));
        result.expectTrue(harness.controller.pendingResponseCount() == 1 && harness.exitRequests == 0,
                          "an out-of-order correlated response completes only its own request");
        harness.controller.receive(response("pipe-acquire"));
        result.expectTrue(harness.controller.pendingResponseCount() == 0 && harness.exitRequests == 1 &&
                              harness.controller.outcome() == client::CommandDrainController::Outcome::Success,
                          "EOF exits successfully only after every sent command receives its correlated response");
        harness.controller.disconnected();
        result.expectTrue(!harness.controller.failed() &&
                              harness.controller.sessionState() == client::CommandDrainController::SessionState::Closed &&
                              harness.exitRequests == 1,
                          "the controlled post-drain disconnect remains successful and idempotent");
    }

    void testSnapshotAndReplaySynchronization(tests::support::TestResult& result) {
        Harness snapshot;
        result.expectTrue(snapshot.controller.enqueue(command("snapshot-1", frontend::SnapshotGet{})),
                          "snapshot is accepted for piped draining");
        snapshot.controller.inputEof();
        snapshot.establishAndSynchronize();
        snapshot.controller.receive(response("snapshot-1"));
        result.expectTrue(snapshot.controller.pendingResponseCount() == 0 && snapshot.controller.pendingSyncCount() == 1 &&
                              snapshot.exitRequests == 0,
                          "a successful snapshot response leaves one explicit synchronization completion pending");
        snapshot.controller.receive(
            frontend::ServerMessage{frontend::Snapshot{frontend::SequenceNumber{2}, frontend::Json::object(), frontend::Json::object()}});
        result.expectTrue(snapshot.exitRequests == 0, "snapshot payload presentation alone does not finish snapshot draining");
        snapshot.controller.receive(sync(2));
        result.expectTrue(snapshot.exitRequests == 1 && snapshot.controller.outcome() == client::CommandDrainController::Outcome::Success,
                          "snapshot draining completes only after its subsequent sync.complete");

        Harness replay;
        result.expectTrue(replay.controller.enqueue(command("replay-1", frontend::ReplayAfter{frontend::SequenceNumber{7}})),
                          "replay is accepted for piped draining");
        replay.controller.inputEof();
        replay.establishAndSynchronize();
        replay.controller.receive(response("replay-1"));
        replay.controller.receive(frontend::ServerMessage{
            frontend::EventBatch{frontend::SequenceNumber{8}, frontend::SequenceNumber{8}, {}, frontend::Json::object()}});
        result.expectTrue(replay.exitRequests == 0 && replay.controller.pendingSyncCount() == 1,
                          "replay event batches do not substitute for the resulting sync.complete");
        replay.controller.receive(sync(8));
        result.expectTrue(replay.exitRequests == 1 && replay.controller.outcome() == client::CommandDrainController::Outcome::Success,
                          "replay draining completes after its correlated response and subsequent sync.complete");

        Harness rejectedReplay;
        result.expectTrue(rejectedReplay.controller.enqueue(command("replay-failed", frontend::ReplayAfter{frontend::SequenceNumber{99}})),
                          "a replay that the server will reject is still accepted for piped draining");
        rejectedReplay.controller.inputEof();
        rejectedReplay.establishAndSynchronize();
        rejectedReplay.controller.receive(response("replay-failed", false));
        result.expectTrue(rejectedReplay.exitRequests == 1 && rejectedReplay.controller.pendingSyncCount() == 0 &&
                              rejectedReplay.controller.outcome() == client::CommandDrainController::Outcome::Success,
                          "a failed replay response completes without waiting for a sync marker the server does not emit");
    }

    void testRawMessagesAndDuplicateIds(tests::support::TestResult& result) {
        Harness harness;
        result.expectTrue(harness.controller.enqueue(frontend::ClientMessage{frontend::Hello{std::nullopt, frontend::Json::object()}}),
                          "a validated raw non-command message is accepted without a response expectation");
        result.expectTrue(harness.controller.enqueue(command("raw-id", frontend::ControllerAcquire{})),
                          "a validated raw command is accepted for response tracking");
        result.expectTrue(harness.controller.enqueue(command("raw-id", frontend::ThreadList{})),
                          "a second raw command reusing the request ID remains queued in order");
        harness.establishAndSynchronize();
        result.expectTrue(harness.sent.size() == 2 && std::holds_alternative<frontend::Hello>(harness.sent[0]) &&
                              sentCommand(harness.sent[1]) != nullptr && harness.controller.queuedCount() == 1 &&
                              harness.controller.pendingResponseCount() == 1,
                          "raw non-command messages send in order without an expectation and duplicate command IDs serialize safely");

        result.expectTrue(
            harness.controller.enqueue(frontend::ClientMessage{frontend::Hello{frontend::SequenceNumber{4}, frontend::Json::object()}}),
            "a later raw non-command message is accepted behind the blocked duplicate command");
        result.expectTrue(harness.controller.enqueue(command("later-id", frontend::ControllerRelease{})),
                          "a later distinct command is accepted behind the existing ready-state backlog");
        result.expectTrue(harness.sent.size() == 2 && harness.controller.queuedCount() == 3,
                          "a ready-state backlog prevents later messages from bypassing its blocked head");
        harness.controller.inputEof();

        harness.controller.receive(response("raw-id"));
        result.expectTrue(harness.sent.size() == 5 && sentCommand(harness.sent[2]) != nullptr &&
                              std::holds_alternative<frontend::ThreadList>(sentCommand(harness.sent[2])->parameters) &&
                              std::holds_alternative<frontend::Hello>(harness.sent[3]) && sentCommand(harness.sent[4]) != nullptr &&
                              sentCommand(harness.sent[4])->requestId == "later-id" && harness.exitRequests == 0,
                          "releasing the blocked head sends every queued message exactly once in original order");
        harness.controller.receive(response("later-id"));
        result.expectTrue(harness.exitRequests == 0 && harness.controller.pendingResponseCount() == 1,
                          "the later command response cannot complete draining while the reused ID remains pending");
        harness.controller.receive(response("raw-id"));
        result.expectTrue(harness.exitRequests == 1 && harness.controller.outcome() == client::CommandDrainController::Outcome::Success,
                          "raw validated Commands participate fully in response tracking");
    }

    void testFailurePathsAndEmptyPipe(tests::support::TestResult& result) {
        Harness disconnected;
        result.expectTrue(disconnected.controller.enqueue(command("pending", frontend::ThreadList{})),
                          "a command is accepted for premature-disconnect coverage");
        disconnected.controller.inputEof();
        disconnected.establishAndSynchronize();
        disconnected.controller.disconnected();
        result.expectTrue(disconnected.controller.failed() && disconnected.exitRequests == 1 && disconnected.failures.size() == 1,
                          "disconnect before drain completion terminates once with failure instead of hanging");

        Harness connectFailure;
        connectFailure.controller.inputEof();
        connectFailure.controller.connectionFailed("connect failed");
        result.expectTrue(connectFailure.controller.failed() && connectFailure.exitRequests == 1 &&
                              connectFailure.controller.failureReason() == "connect failed",
                          "connection failure before synchronization produces a nonzero terminal outcome");

        Harness protocolError;
        result.expectTrue(protocolError.controller.enqueue(command("protocol", frontend::ThreadList{})),
                          "a command is accepted for draining protocol-error coverage");
        protocolError.controller.inputEof();
        protocolError.establishAndSynchronize();
        protocolError.controller.receive(frontend::ServerMessage{frontend::ProtocolErrorMessage{frontend::ErrorCode::InvalidCommand,
                                                                                                "bad command",
                                                                                                {},
                                                                                                false,
                                                                                                std::string{"protocol"},
                                                                                                std::nullopt,
                                                                                                frontend::Json::object()}});
        result.expectTrue(protocolError.controller.failed() && protocolError.exitRequests == 1,
                          "a decoded protocol error during draining terminates with failure even when it is non-closing");

        Harness codecError;
        codecError.controller.inputEof();
        codecError.controller.protocolFailed("malformed server frame");
        result.expectTrue(codecError.controller.failed() && codecError.exitRequests == 1,
                          "a local codec/protocol failure during draining terminates with failure");

        Harness sendFailure;
        sendFailure.sendSucceeds = false;
        result.expectTrue(sendFailure.controller.enqueue(command("send-failed", frontend::ThreadList{})),
                          "a command is queued before the deterministic send failure");
        sendFailure.controller.inputEof();
        sendFailure.establishAndSynchronize();
        result.expectTrue(sendFailure.controller.failed() && sendFailure.sent.size() == 1 && sendFailure.exitRequests == 1,
                          "a queued send failure is attempted once and terminates without retry or hang");

        Harness empty;
        empty.controller.inputEof();
        empty.controller.connected();
        empty.controller.receive(frontend::ServerMessage{frontend::Welcome{"empty",
                                                                           frontend::SessionRole::Observer,
                                                                           frontend::SequenceNumber{0},
                                                                           frontend::SyncMode::Snapshot,
                                                                           frontend::Json::object()}});
        empty.controller.receive(
            frontend::ServerMessage{frontend::Snapshot{frontend::SequenceNumber{0}, frontend::Json::object(), frontend::Json::object()}});
        result.expectTrue(empty.exitRequests == 0, "an empty pipe stays connected through welcome and initial snapshot");
        empty.controller.receive(sync(0));
        result.expectTrue(empty.exitRequests == 1 && empty.sent.empty() &&
                              empty.controller.outcome() == client::CommandDrainController::Outcome::Success,
                          "an empty pipe exits cleanly immediately after initial synchronization");
    }

    void testNewSynchronizationAndSuccessfulHandoff(tests::support::TestResult& result) {
        Harness beforeConnection;
        frontend::ThreadStart options;
        options.cwd = "/tmp/new project";
        options.model = "model-new";
        options.ephemeral = true;
        result.expectTrue(beforeConnection.controller.enqueue(newCommand("new-start", "new-turn", "preserve  spaces and --words", options)),
                          "new entered before connection is accepted as one compound command");
        result.expectTrue(beforeConnection.sent.empty() && beforeConnection.controller.queuedCount() == 1 &&
                              beforeConnection.controller.newStage() == client::CommandDrainController::NewStage::Queued,
                          "new remains queued before connection with an explicit queued stage");

        beforeConnection.controller.connected();
        result.expectTrue(beforeConnection.sent.empty() &&
                              beforeConnection.controller.sessionState() == client::CommandDrainController::SessionState::Synchronizing,
                          "connecting does not release new before initial synchronization");
        beforeConnection.controller.receive(sync());
        const frontend::Command* startCommand = beforeConnection.sent.empty() ? nullptr : sentCommand(beforeConnection.sent.front());
        const frontend::ThreadStart* start = sentParameters<frontend::ThreadStart>(beforeConnection.sent, 0);
        result.expectTrue(beforeConnection.sent.size() == 1 && startCommand != nullptr && startCommand->requestId == "new-start" &&
                              start != nullptr && *start == options &&
                              beforeConnection.controller.newStage() ==
                                  client::CommandDrainController::NewStage::AwaitingThreadStartResponse,
                          "sync.complete sends exactly the typed ThreadStart stage with every parsed option");

        beforeConnection.controller.receive(response("unrelated"));
        result.expectTrue(beforeConnection.sent.size() == 1 && beforeConnection.controller.newStage() ==
                                                                   client::CommandDrainController::NewStage::AwaitingThreadStartResponse,
                          "an unrelated response does not advance the compound command");

        const auto startPresentation = beforeConnection.controller.receive(
            successfulResponse("new-start", frontend::Json{{"thread", {{"id", "thread-created"}, {"status", "idle"}}}}));
        startCommand = beforeConnection.sent.empty() ? nullptr : sentCommand(beforeConnection.sent.front());
        const frontend::Command* turnCommand = beforeConnection.sent.size() > 1 ? sentCommand(beforeConnection.sent[1]) : nullptr;
        const frontend::TurnStart* turn = sentParameters<frontend::TurnStart>(beforeConnection.sent, 1);
        const frontend::TextInput* text =
            turn != nullptr && turn->input.size() == 1 ? std::get_if<frontend::TextInput>(&turn->input.front()) : nullptr;
        result.expectTrue(startPresentation.has_value() && startPresentation->kind == client::ResponsePresentation::Kind::NewThreadStart &&
                              startPresentation->threadId == std::optional<std::string>{"thread-created"},
                          "the matching start response exposes bounded presentation metadata for the created thread");
        result.expectTrue(beforeConnection.sent.size() == 2 && turnCommand != nullptr && turnCommand->requestId == "new-turn" &&
                              turn != nullptr && turn->threadId == "thread-created" && text != nullptr &&
                              text->text == "preserve  spaces and --words" &&
                              beforeConnection.controller.newStage() == client::CommandDrainController::NewStage::AwaitingTurnStartResponse,
                          "the matching start result submits exactly one TurnStart with its returned ID and original TextInput");
        result.expectTrue(startCommand != nullptr && turnCommand != nullptr && startCommand->requestId != turnCommand->requestId,
                          "the compound stages use distinct generated request IDs");
        result.expectTrue(std::holds_alternative<frontend::ThreadStart>(startCommand->parameters) &&
                              std::holds_alternative<frontend::TurnStart>(turnCommand->parameters),
                          "new emits only the real ThreadStart and TurnStart protocol operations");

        beforeConnection.controller.receive(
            successfulResponse("new-start", frontend::Json{{"thread", {{"id", "thread-created"}, {"status", "idle"}}}}));
        result.expectTrue(beforeConnection.sent.size() == 2, "a duplicate or late start response never submits a second initial turn");

        const auto turnPresentation = beforeConnection.controller.receive(response("new-turn"));
        result.expectTrue(turnPresentation.has_value() && turnPresentation->kind == client::ResponsePresentation::Kind::NewTurnStart &&
                              turnPresentation->threadId == std::optional<std::string>{"thread-created"} &&
                              beforeConnection.controller.newStage() == client::CommandDrainController::NewStage::None &&
                              beforeConnection.controller.outcome() == client::CommandDrainController::Outcome::Running,
                          "a matching successful turn response completes new while interactive input stays open");

        Harness duringSynchronization;
        duringSynchronization.controller.connected();
        result.expectTrue(duringSynchronization.controller.enqueue(newCommand("sync-start", "sync-turn", "during sync")),
                          "new entered during initial synchronization is accepted");
        result.expectTrue(duringSynchronization.sent.empty() && duringSynchronization.controller.queuedCount() == 1,
                          "new entered during synchronization remains behind the barrier");
        duringSynchronization.controller.receive(sync());
        result.expectTrue(duringSynchronization.sent.size() == 1 &&
                              sentParameters<frontend::ThreadStart>(duringSynchronization.sent, 0) != nullptr,
                          "the synchronization barrier releases the queued new ThreadStart exactly once");
    }

    void testNewEofDrainAndQueueOrdering(tests::support::TestResult& result) {
        Harness harness;
        result.expectTrue(harness.controller.enqueue(newCommand("pipe-new-start", "pipe-new-turn", "first prompt")),
                          "piped new is accepted before connection");
        result.expectTrue(harness.controller.enqueue(command("after-new", frontend::ThreadList{})),
                          "a later ordinary command queues behind new");
        harness.controller.inputEof();
        harness.establishAndSynchronize();
        result.expectTrue(harness.sent.size() == 1 && sentParameters<frontend::ThreadStart>(harness.sent, 0) != nullptr &&
                              harness.exitRequests == 0,
                          "EOF sends ThreadStart but neither exits nor lets later input bypass active new");

        harness.controller.receive(successfulResponse("pipe-new-start", frontend::Json{{"threadId", "thread-fallback"}}));
        const frontend::TurnStart* turn = sentParameters<frontend::TurnStart>(harness.sent, 1);
        result.expectTrue(harness.sent.size() == 2 && turn != nullptr && turn->threadId == "thread-fallback" && harness.exitRequests == 0 &&
                              harness.controller.pendingResponseCount() == 1,
                          "BackendAdapter's threadId fallback is handed to TurnStart and EOF waits for its response");

        harness.controller.receive(response("pipe-new-turn"));
        result.expectTrue(harness.sent.size() == 3 && sentParameters<frontend::ThreadList>(harness.sent, 2) != nullptr &&
                              harness.exitRequests == 0,
                          "only successful completion of both new stages releases the next queued command");
        harness.controller.receive(response("after-new"));
        result.expectTrue(harness.exitRequests == 1 && harness.controller.outcome() == client::CommandDrainController::Outcome::Success,
                          "EOF exits successfully after both new stages and all later queued work complete");

        Harness multiple;
        multiple.establishAndSynchronize();
        result.expectTrue(multiple.controller.enqueue(newCommand("first-start", "first-turn", "one")) &&
                              multiple.controller.enqueue(newCommand("second-start", "second-turn", "two")),
                          "multiple new operations are accepted in input order");
        result.expectTrue(multiple.sent.size() == 1 && sentCommand(multiple.sent[0])->requestId == "first-start",
                          "the second new cannot start while the first compound command is active");
        multiple.controller.receive(successfulResponse("first-start", frontend::Json{{"thread", {{"id", "thread-one"}}}}));
        multiple.controller.receive(response("first-turn"));
        result.expectTrue(multiple.sent.size() == 3 && sentCommand(multiple.sent[2])->requestId == "second-start" &&
                              sentParameters<frontend::ThreadStart>(multiple.sent, 2) != nullptr,
                          "multiple new operations serialize at compound-command boundaries");
    }

    void testNewFailures(tests::support::TestResult& result) {
        Harness startSendFailure;
        startSendFailure.failedSendAttempt = 1;
        startSendFailure.establishAndSynchronize();
        result.expectTrue(!startSendFailure.controller.enqueue(newCommand("unsent-start", "unsent-turn", "prompt")) &&
                              startSendFailure.controller.failed() && startSendFailure.sent.size() == 1 &&
                              startSendFailure.exitRequests == 1 &&
                              startSendFailure.controller.failureReason().find("new thread creation failed") != std::string::npos &&
                              startSendFailure.controller.failureReason().find("failed to send frontend message") != std::string::npos &&
                              startSendFailure.controller.failureReason().find("thread created id=") == std::string::npos,
                          "ThreadStart send failure terminates new once without claiming that a thread was created");

        Harness rejectedStart;
        rejectedStart.establishAndSynchronize();
        result.expectTrue(rejectedStart.controller.enqueue(newCommand("reject-start", "never-turn", "prompt")),
                          "new is submitted for start-failure coverage");
        const auto rejectedPresentation = rejectedStart.controller.receive(response("reject-start", false));
        result.expectTrue(rejectedStart.controller.failed() && rejectedStart.sent.size() == 1 && rejectedStart.exitRequests == 1 &&
                              rejectedPresentation.has_value() &&
                              rejectedPresentation->kind == client::ResponsePresentation::Kind::NewThreadStart &&
                              rejectedStart.controller.failureReason().find("thread.start failed") != std::string::npos,
                          "a failed ThreadStart prevents TurnStart and makes the compound operation fail");

        Harness missingId;
        missingId.establishAndSynchronize();
        result.expectTrue(missingId.controller.enqueue(newCommand("missing-id", "missing-id-turn", "prompt")),
                          "new is submitted for malformed-success coverage");
        missingId.controller.receive(successfulResponse("missing-id", frontend::Json{{"thread", {{"id", ""}}}}));
        result.expectTrue(missingId.controller.failed() && missingId.sent.size() == 1 && missingId.exitRequests == 1 &&
                              missingId.controller.failureReason().find("non-empty thread ID") != std::string::npos,
                          "a successful ThreadStart without a valid ID fails locally and never submits TurnStart");

        Harness turnSendFailure;
        turnSendFailure.failedSendAttempt = 2;
        turnSendFailure.establishAndSynchronize();
        result.expectTrue(turnSendFailure.controller.enqueue(newCommand("send-start", "send-turn", "prompt")),
                          "new is submitted for second-stage send-failure coverage");
        turnSendFailure.controller.receive(successfulResponse("send-start", frontend::Json{{"thread", {{"id", "thread-send-failed"}}}}));
        result.expectTrue(turnSendFailure.controller.failed() && turnSendFailure.sent.size() == 2 && turnSendFailure.exitRequests == 1 &&
                              turnSendFailure.controller.failureReason().find("thread created id=thread-send-failed") !=
                                  std::string::npos &&
                              turnSendFailure.controller.failureReason().find("failed to send frontend message") != std::string::npos,
                          "TurnStart send failure reports the successfully created thread ID and returns failure");

        Harness turnResponseFailure;
        turnResponseFailure.establishAndSynchronize();
        result.expectTrue(turnResponseFailure.controller.enqueue(newCommand("response-start", "response-turn", "prompt")),
                          "new is submitted for second-stage response-failure coverage");
        turnResponseFailure.controller.receive(
            successfulResponse("response-start", frontend::Json{{"thread", {{"id", "thread-turn-rejected"}}}}));
        const auto partialPresentation = turnResponseFailure.controller.receive(response("response-turn", false));
        result.expectTrue(
            turnResponseFailure.controller.failed() && turnResponseFailure.exitRequests == 1 && partialPresentation.has_value() &&
                partialPresentation->kind == client::ResponsePresentation::Kind::NewTurnStart &&
                partialPresentation->threadId == std::optional<std::string>{"thread-turn-rejected"} &&
                turnResponseFailure.controller.failureReason().find("thread created id=thread-turn-rejected") != std::string::npos &&
                turnResponseFailure.controller.failureReason().find("turn.start failed") != std::string::npos,
            "a failed TurnStart response reports partial failure with the preserved thread ID");
    }

    void testNewDisconnectsAndDuplicateTurnId(tests::support::TestResult& result) {
        Harness duringStart;
        duringStart.establishAndSynchronize();
        result.expectTrue(duringStart.controller.enqueue(newCommand("disconnect-start", "disconnect-turn", "prompt")),
                          "new is submitted for start-stage disconnect coverage");
        duringStart.controller.disconnected();
        result.expectTrue(duringStart.controller.failed() && duringStart.exitRequests == 1 &&
                              duringStart.controller.failureReason().find("new thread creation failed") != std::string::npos,
                          "disconnect during ThreadStart fails cleanly before any thread ID is claimed");

        Harness duringTurn;
        duringTurn.establishAndSynchronize();
        result.expectTrue(duringTurn.controller.enqueue(newCommand("disconnect-start-ok", "disconnect-turn-pending", "prompt")),
                          "new is submitted for turn-stage disconnect coverage");
        duringTurn.controller.receive(
            successfulResponse("disconnect-start-ok", frontend::Json{{"thread", {{"id", "thread-disconnected"}}}}));
        duringTurn.controller.disconnected();
        result.expectTrue(duringTurn.controller.failed() && duringTurn.exitRequests == 1 &&
                              duringTurn.controller.failureReason().find("thread created id=thread-disconnected") != std::string::npos,
                          "disconnect during TurnStart reports partial failure with the created thread ID");

        Harness duplicateTurnId;
        result.expectTrue(duplicateTurnId.controller.enqueue(command("shared-turn-id", frontend::ControllerAcquire{})),
                          "a raw command can reserve the future generated turn ID");
        result.expectTrue(duplicateTurnId.controller.enqueue(newCommand("distinct-start-id", "shared-turn-id", "prompt")),
                          "new with a colliding internal turn ID remains accepted for serialized delivery");
        duplicateTurnId.establishAndSynchronize();
        result.expectTrue(duplicateTurnId.sent.size() == 2,
                          "the earlier raw command and distinct ThreadStart are both sent in input order");
        duplicateTurnId.controller.receive(
            successfulResponse("distinct-start-id", frontend::Json{{"thread", {{"id", "thread-collision"}}}}));
        result.expectTrue(duplicateTurnId.sent.size() == 2 &&
                              duplicateTurnId.controller.newStage() == client::CommandDrainController::NewStage::WaitingToSubmitTurn,
                          "a pending duplicate request ID blocks the internal TurnStart without bypassing protection");
        duplicateTurnId.controller.receive(response("shared-turn-id"));
        const frontend::Command* delayedTurn = duplicateTurnId.sent.size() > 2 ? sentCommand(duplicateTurnId.sent[2]) : nullptr;
        result.expectTrue(duplicateTurnId.sent.size() == 3 && delayedTurn != nullptr && delayedTurn->requestId == "shared-turn-id" &&
                              std::holds_alternative<frontend::TurnStart>(delayedTurn->parameters),
                          "the internal TurnStart is submitted exactly once when its duplicate request ID becomes free");
    }

    void testNewCorrelatedProtocolErrors(tests::support::TestResult& result) {
        Harness duringStart;
        duringStart.establishAndSynchronize();
        result.expectTrue(duringStart.controller.enqueue(newCommand("protocol-start", "protocol-turn", "prompt")),
                          "new is submitted for correlated start-stage protocol-error coverage");
        duringStart.controller.receive(frontend::ServerMessage{frontend::ProtocolErrorMessage{frontend::ErrorCode::InvalidCommand,
                                                                                              "start protocol rejection",
                                                                                              {},
                                                                                              false,
                                                                                              std::string{"protocol-start"},
                                                                                              std::nullopt,
                                                                                              frontend::Json::object()}});
        result.expectTrue(duringStart.controller.failed() && duringStart.sent.size() == 1 && duringStart.exitRequests == 1 &&
                              duringStart.controller.failureReason().find("new thread creation failed") != std::string::npos,
                          "a matching non-closing protocol error terminates the interactive ThreadStart stage deterministically");

        Harness duringTurn;
        duringTurn.establishAndSynchronize();
        result.expectTrue(duringTurn.controller.enqueue(newCommand("protocol-start-ok", "protocol-turn-failed", "prompt")),
                          "new is submitted for correlated turn-stage protocol-error coverage");
        duringTurn.controller.receive(
            successfulResponse("protocol-start-ok", frontend::Json{{"thread", {{"id", "thread-protocol-error"}}}}));
        duringTurn.controller.receive(frontend::ServerMessage{frontend::ProtocolErrorMessage{frontend::ErrorCode::InvalidCommand,
                                                                                             "turn protocol rejection",
                                                                                             {},
                                                                                             false,
                                                                                             std::string{"protocol-turn-failed"},
                                                                                             std::nullopt,
                                                                                             frontend::Json::object()}});
        result.expectTrue(duringTurn.controller.failed() && duringTurn.sent.size() == 2 && duringTurn.exitRequests == 1 &&
                              duringTurn.controller.failureReason().find("thread created id=thread-protocol-error") != std::string::npos,
                          "a matching non-closing protocol error reports partial failure for the interactive TurnStart stage");
    }

    void testResponsePresentationCorrelation(tests::support::TestResult& result) {
        Harness harness;
        harness.establishAndSynchronize();
        result.expectTrue(harness.controller.enqueue(command("friendly-start", frontend::ThreadStart{})) &&
                              harness.controller.enqueue(command("friendly-resume", frontend::ThreadResume{"thread-resumed"})) &&
                              harness.controller.enqueue(command("friendly-turn", frontend::TurnStart{"thread-turned"})),
                          "friendly typed commands are accepted for presentation correlation");

        const auto resumed =
            harness.controller.receive(successfulResponse("friendly-resume", frontend::Json{{"thread", {{"id", "thread-resumed"}}}}));
        const auto started =
            harness.controller.receive(successfulResponse("friendly-start", frontend::Json{{"thread", {{"id", "thread-started"}}}}));
        const auto turned = harness.controller.receive(response("friendly-turn", false));
        result.expectTrue(resumed == std::optional<client::ResponsePresentation>{client::ResponsePresentation{
                                         client::ResponsePresentation::Kind::ThreadResume, std::string{"thread-resumed"}}} &&
                              started == std::optional<client::ResponsePresentation>{client::ResponsePresentation{
                                             client::ResponsePresentation::Kind::ThreadStart, std::string{"thread-started"}}} &&
                              turned == std::optional<client::ResponsePresentation>{client::ResponsePresentation{
                                            client::ResponsePresentation::Kind::TurnStart, std::string{"thread-turned"}}},
                          "out-of-order responses retain their correlated start, resume, and turn presentation context");
    }
} // namespace

int main() {
    tests::support::TestResult result;

    testPreconnectionQueueAndInteractiveBehavior(result);
    testEofBeforeConnectionDrainsResponses(result);
    testSnapshotAndReplaySynchronization(result);
    testRawMessagesAndDuplicateIds(result);
    testFailurePathsAndEmptyPipe(result);
    testNewSynchronizationAndSuccessfulHandoff(result);
    testNewEofDrainAndQueueOrdering(result);
    testNewFailures(result);
    testNewDisconnectsAndDuplicateTurnId(result);
    testNewCorrelatedProtocolErrors(result);
    testResponsePresentationCorrelation(result);

    return result.processResult();
}
