/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include "ai/openai/codex/frontend/Messages.h"
#include "apps/codex-backend-client/CommandDrainController.h"
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

    const frontend::Command* sentCommand(const frontend::ClientMessage& message) {
        return std::get_if<frontend::Command>(&message);
    }

    struct Harness {
        Harness()
            : controller(client::CommandDrainCallbacks{.send =
                                                           [this](const frontend::ClientMessage& message) {
                                                               sent.push_back(message);
                                                               return sendSucceeds;
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
} // namespace

int main() {
    tests::support::TestResult result;

    testPreconnectionQueueAndInteractiveBehavior(result);
    testEofBeforeConnectionDrainsResponses(result);
    testSnapshotAndReplaySynchronization(result);
    testRawMessagesAndDuplicateIds(result);
    testFailurePathsAndEmptyPipe(result);

    return result.processResult();
}
