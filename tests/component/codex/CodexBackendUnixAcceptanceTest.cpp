/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include "CodexBackendTestSupport.h"
#include "ai/openai/codex/backend/BackendCore.h"
#include "ai/openai/codex/frontend/Codec.h"
#include "apps/codex-backend/CodexFrontendSocketContextFactory.h"
#include "apps/codex-backend/Configuration.h"
#include "core/EventReceiver.h"
#include "core/SNodeC.h"
#include "core/socket/State.h"
#include "core/socket/stream/SocketConnection.h"
#include "core/socket/stream/SocketContext.h"
#include "core/socket/stream/SocketContextFactory.h"
#include "core/timer/Timer.h"
#include "net/un/SocketAddress.h"
#include "net/un/stream/legacy/SocketClient.h"
#include "net/un/stream/legacy/SocketServer.h"
#include "support/TestResult.h"
#include "utils/Timeval.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unistd.h>
#include <utility>
#include <variant>
#include <vector>

namespace {
    namespace backend = ai::openai::codex::backend;
    namespace frontend = ai::openai::codex::frontend;

    using FakeBackendCore = backend::BackendCore<tests::codex::FakeAppServerClient>;

    using ai::openai::codex::Json;
    using ai::openai::codex::detail::TransportCallbacks;

    constexpr std::size_t MaximumFrameSize = 1024;
    constexpr std::size_t EvictionEventCount = 4200;
    constexpr std::size_t EvictionChunkSize = 100;

    enum class ClientKind : std::size_t { ControllerA, ObserverB, ReplayB, OldReconnect, Malformed, Oversized, Count };

    constexpr std::size_t clientIndex(ClientKind kind) {
        return static_cast<std::size_t>(kind);
    }

    std::string clientName(ClientKind kind) {
        switch (kind) {
            case ClientKind::ControllerA:
                return "controller-a";
            case ClientKind::ObserverB:
                return "observer-b";
            case ClientKind::ReplayB:
                return "replay-b";
            case ClientKind::OldReconnect:
                return "old-reconnect";
            case ClientKind::Malformed:
                return "malformed";
            case ClientKind::Oversized:
                return "oversized";
            case ClientKind::Count:
                break;
        }
        return "unknown";
    }

    class TestFrontendClientContext;

    struct ClientHooks {
        std::function<void(ClientKind, TestFrontendClientContext&)> onConnected;
        std::function<void(ClientKind)> onDisconnected;
        std::function<void(ClientKind, frontend::ServerMessage, std::size_t)> onMessage;
        std::function<void(ClientKind, std::string)> onDecodeFailure;
    };

    class TestFrontendClientContext final : public core::socket::stream::SocketContext {
    public:
        TestFrontendClientContext(core::socket::stream::SocketConnection* socketConnection, ClientHooks& hooks, ClientKind kind)
            : core::socket::stream::SocketContext(socketConnection)
            , hooks(hooks)
            , kind(kind) {
        }

        void sendMessage(frontend::ClientMessage message) {
            const auto serialized = frontend::Codec::serializeClient(message);
            if (!serialized) {
                hooks.onDecodeFailure(kind, "client message serialization failed: " + serialized.error().message);
                return;
            }
            std::string frame = serialized.value();
            frame.push_back('\n');
            sendToPeer(frame.data(), frame.size());
        }

        void sendFragmented(frontend::ClientMessage message) {
            const auto serialized = frontend::Codec::serializeClient(message);
            if (!serialized) {
                hooks.onDecodeFailure(kind, "fragmented client message serialization failed: " + serialized.error().message);
                return;
            }
            fragmentedFrame = serialized.value();
            fragmentedFrame.push_back('\n');
            const std::size_t split = fragmentedFrame.size() / 2;
            sendToPeer(fragmentedFrame.data(), split);
            ++fragmentWrites;
            fragmentTimer = core::timer::Timer::singleshotTimer(
                [this, split]() {
                    if (split < fragmentedFrame.size()) {
                        sendToPeer(fragmentedFrame.data() + static_cast<std::ptrdiff_t>(split), fragmentedFrame.size() - split);
                        ++fragmentWrites;
                    }
                },
                utils::Timeval({0, 1000}));
        }

        void sendRaw(std::string bytes) {
            sendToPeer(bytes.data(), bytes.size());
        }

        void sendCoalesced(frontend::ClientMessage first, frontend::ClientMessage second) {
            std::string frames;
            for (const frontend::ClientMessage* message : {&first, &second}) {
                const auto serialized = frontend::Codec::serializeClient(*message);
                if (!serialized) {
                    hooks.onDecodeFailure(kind, "coalesced client message serialization failed: " + serialized.error().message);
                    return;
                }
                frames += serialized.value();
                frames.push_back('\n');
            }
            sendToPeer(frames.data(), frames.size());
            ++coalescedWrites;
        }

        void disconnect() {
            close();
        }

        std::size_t fragmentedWriteCount() const noexcept {
            return fragmentWrites;
        }

        std::size_t coalescedWriteCount() const noexcept {
            return coalescedWrites;
        }

    private:
        void onConnected() override {
            hooks.onConnected(kind, *this);
        }

        void onDisconnected() override {
            hooks.onDisconnected(kind);
        }

        std::size_t onReceivedFromPeer() override {
            std::array<char, 16 * 1024> chunk{};
            const std::size_t size = readFromPeer(chunk.data(), chunk.size());
            if (size == 0) {
                return size;
            }
            receiveBuffer.append(chunk.data(), size);
            if (receiveBuffer.size() > 4U * 1024U * 1024U) {
                hooks.onDecodeFailure(kind, "test frontend receive buffer exceeded deterministic bound");
                close();
                return size;
            }
            while (true) {
                const std::size_t newline = receiveBuffer.find('\n');
                if (newline == std::string::npos) {
                    break;
                }
                std::string frame = receiveBuffer.substr(0, newline);
                receiveBuffer.erase(0, newline + 1);
                const auto decoded = frontend::Codec::decodeServer(std::string_view(frame));
                if (!decoded) {
                    hooks.onDecodeFailure(kind, "server message decode failed: " + decoded.error().message);
                    continue;
                }
                hooks.onMessage(kind, decoded.value(), frame.size() + 1);
            }
            return size;
        }

        bool onSignal([[maybe_unused]] int signum) override {
            return true;
        }

        ClientHooks& hooks;
        ClientKind kind;
        std::string receiveBuffer;
        std::string fragmentedFrame;
        std::size_t fragmentWrites = 0;
        std::size_t coalescedWrites = 0;
        core::timer::Timer fragmentTimer;
    };

    class TestFrontendClientFactory final : public core::socket::stream::SocketContextFactory {
    public:
        TestFrontendClientFactory(ClientHooks& hooks, ClientKind kind)
            : hooks(hooks)
            , kind(kind) {
        }

        core::socket::stream::SocketContext* create(core::socket::stream::SocketConnection* socketConnection) override {
            return new TestFrontendClientContext(socketConnection, hooks, kind);
        }

    private:
        ClientHooks& hooks;
        ClientKind kind;
    };

    frontend::ClientMessage hello(std::optional<frontend::SequenceNumber> resumeAfter = std::nullopt) {
        return frontend::Hello{resumeAfter, Json::object()};
    }

    frontend::ClientMessage command(std::string requestId, frontend::CommandParameters parameters) {
        return frontend::Command{std::move(requestId), std::move(parameters), Json::object(), Json::object()};
    }

    Json agentItem(const std::string& id, const std::string& text = {}) {
        return {{"type", "agentMessage"}, {"id", id}, {"text", text}};
    }

    Json commandItem(const std::string& id, const std::string& status, std::optional<std::string> output = std::nullopt) {
        Json item{{"type", "commandExecution"},
                  {"id", id},
                  {"command", "printf acceptance"},
                  {"cwd", "/tmp/project"},
                  {"status", status},
                  {"commandActions", Json::array()}};
        if (output.has_value()) {
            item["aggregatedOutput"] = *output;
        }
        return item;
    }

    struct ClientRecord {
        TestFrontendClientContext* context = nullptr;
        bool connected = false;
        bool disconnected = false;
        std::size_t welcomeCount = 0;
        std::size_t snapshotCount = 0;
        std::size_t syncCompleteCount = 0;
        std::size_t eventBatchCount = 0;
        std::size_t maximumBatchEvents = 0;
        std::size_t maximumWireFrame = 0;
        std::size_t protocolErrorCount = 0;
        std::optional<frontend::Welcome> welcome;
        std::optional<frontend::Snapshot> latestSnapshot;
        std::map<std::string, frontend::Response> responses;
        std::vector<frontend::FrontendEvent> events;
        std::uint64_t lastSequence = 0;
        std::size_t agentContentUpdates = 0;
        std::size_t commandContentUpdates = 0;
        std::string agentContent;
        std::string commandOutput;
        std::optional<std::string> pendingRequestId;
        bool requestResolved = false;
        bool terminalTurn = false;
        std::uint64_t commandOutputSequence = 0;
        std::uint64_t terminalTurnSequence = 0;
        std::size_t highestEvictionExtension = 0;
        std::optional<frontend::ErrorCode> protocolError;
    };

    class AcceptanceScenario {
    public:
        AcceptanceScenario(tests::support::TestResult& result,
                           FakeBackendCore& backendCore,
                           std::shared_ptr<tests::codex::FakeTransportState> transport)
            : result(result)
            , backendCore(backendCore)
            , transport(std::move(transport)) {
            hooks.onConnected = [this](ClientKind kind, TestFrontendClientContext& context) {
                clientConnected(kind, context);
            };
            hooks.onDisconnected = [this](ClientKind kind) {
                clientDisconnected(kind);
            };
            hooks.onMessage = [this](ClientKind kind, frontend::ServerMessage message, std::size_t bytes) {
                serverMessage(kind, std::move(message), bytes);
            };
            hooks.onDecodeFailure = [this](ClientKind kind, std::string message) {
                ++decodeFailures;
                this->result.expectTrue(false, clientName(kind) + ": " + message);
                fail();
            };
        }

        ClientHooks& clientHooks() noexcept {
            return hooks;
        }

        void setConnector(std::function<void(ClientKind)> value) {
            connector = std::move(value);
        }

        void listenReady() {
            ++listenSuccesses;
            connect(ClientKind::ControllerA);
        }

        void listenFailed() {
            ++listenFailures;
            result.expectTrue(false, "Unix reference backend listen callback reports OK");
            fail();
        }

        bool isFinished() const noexcept {
            return finished;
        }

        std::size_t successfulListens() const noexcept {
            return listenSuccesses;
        }

        std::size_t failedListens() const noexcept {
            return listenFailures;
        }

        std::size_t connectionFailures() const noexcept {
            return connectFailures;
        }

        std::size_t decodingFailures() const noexcept {
            return decodeFailures;
        }

        bool typedTurnInputObserved() const noexcept {
            return sawTypedTurnInput;
        }

        bool approvalDeclineObserved() const noexcept {
            return approvalDeclineSent();
        }

        const ClientRecord& record(ClientKind kind) const {
            return clients[clientIndex(kind)];
        }

        void connectionResult(ClientKind kind, core::socket::State state) {
            if (state != core::socket::State::OK) {
                ++connectFailures;
                result.expectTrue(false, clientName(kind) + " connects to Unix reference backend");
                fail();
            }
        }

    private:
        enum class Phase {
            WaitingA,
            WaitingB,
            WaitingAcquire,
            WaitingThreadCommands,
            WaitingTurn,
            WaitingAgentBurst,
            WaitingApproval,
            WaitingApprovalResolution,
            WaitingTerminal,
            WaitingADisconnect,
            WaitingBAcquire,
            WaitingBDisconnect,
            WaitingReplay,
            WaitingReplayAcquire,
            EvictingJournal,
            WaitingOldSnapshot,
            WaitingMalformed,
            WaitingOversized,
            WaitingBackendStop,
            Finished
        };

        ClientRecord& client(ClientKind kind) {
            return clients[clientIndex(kind)];
        }

        void connect(ClientKind kind) {
            if (connector) {
                connector(kind);
            } else {
                ++connectFailures;
                fail();
            }
        }

        void clientConnected(ClientKind kind, TestFrontendClientContext& context) {
            ClientRecord& value = client(kind);
            value.context = &context;
            value.connected = true;
            switch (kind) {
                case ClientKind::ControllerA:
                    context.sendFragmented(hello());
                    break;
                case ClientKind::ObserverB:
                    context.sendMessage(hello());
                    break;
                case ClientKind::ReplayB:
                    context.sendMessage(hello(frontend::SequenceNumber{resumeAfterB}));
                    break;
                case ClientKind::OldReconnect:
                    context.sendMessage(hello(frontend::SequenceNumber{oldResumeSequence}));
                    break;
                case ClientKind::Malformed:
                    if (const auto serializedHello = frontend::Codec::serializeClient(hello())) {
                        context.sendRaw("{malformed-json\n" + serializedHello.value() + "\n");
                    } else {
                        ++decodeFailures;
                        result.expectTrue(false, "failed to serialize hello for coalesced malformed-client input-barrier test");
                        fail();
                    }
                    break;
                case ClientKind::Oversized:
                    context.sendRaw(std::string(MaximumFrameSize + 1, 'x'));
                    break;
                case ClientKind::Count:
                    break;
            }
        }

        void clientDisconnected(ClientKind kind) {
            ClientRecord& value = client(kind);
            value.disconnected = true;
            value.context = nullptr;
            advance();
        }

        void serverMessage(ClientKind kind, frontend::ServerMessage message, std::size_t wireBytes) {
            ClientRecord& value = client(kind);
            value.maximumWireFrame = std::max(value.maximumWireFrame, wireBytes);
            if (const auto* welcome = std::get_if<frontend::Welcome>(&message)) {
                ++value.welcomeCount;
                value.welcome = *welcome;
            } else if (const auto* snapshot = std::get_if<frontend::Snapshot>(&message)) {
                ++value.snapshotCount;
                value.latestSnapshot = *snapshot;
            } else if (std::holds_alternative<frontend::SyncComplete>(message)) {
                ++value.syncCompleteCount;
            } else if (const auto* response = std::get_if<frontend::Response>(&message)) {
                value.responses.insert_or_assign(response->requestId, *response);
            } else if (const auto* events = std::get_if<frontend::EventBatch>(&message)) {
                ++value.eventBatchCount;
                value.maximumBatchEvents = std::max(value.maximumBatchEvents, events->events.size());
                for (const frontend::FrontendEvent& event : events->events) {
                    result.expectTrue(event.sequence.value() > value.lastSequence,
                                      clientName(kind) + " receives strictly increasing frontend event sequences");
                    value.lastSequence = event.sequence.value();
                    value.events.push_back(event);
                    observeEvent(value, event);
                }
            } else if (const auto* error = std::get_if<frontend::ProtocolErrorMessage>(&message)) {
                ++value.protocolErrorCount;
                value.protocolError = error->code;
            }
            advance();
        }

        void observeEvent(ClientRecord& value, const frontend::FrontendEvent& event) {
            if (event.type == "item.content.updated" && event.data.is_object()) {
                const std::string channel = event.data.value("channel", "");
                if (channel == "agentText") {
                    ++value.agentContentUpdates;
                    value.agentContent = event.data.value("content", "");
                } else if (channel == "commandOutput") {
                    ++value.commandContentUpdates;
                    value.commandOutput = event.data.value("content", "");
                    value.commandOutputSequence = event.sequence.value();
                }
            } else if (event.type == "request.pending" && event.data.contains("request")) {
                value.pendingRequestId = event.data["request"].value("id", "");
            } else if (event.type == "request.resolved") {
                value.requestResolved = true;
            } else if (event.type == "turn.updated" && event.data.contains("turn") && event.data["turn"].value("terminal", false)) {
                value.terminalTurn = true;
                value.terminalTurnSequence = event.sequence.value();
            } else if (event.type == "codex.extension") {
                const std::string method = event.data.value("method", "");
                constexpr std::string_view prefix = "acceptance/evict/";
                if (method.starts_with(prefix)) {
                    try {
                        value.highestEvictionExtension =
                            std::max(value.highestEvictionExtension, static_cast<std::size_t>(std::stoull(method.substr(prefix.size()))));
                    } catch (...) {
                    }
                }
            }
        }

        const frontend::Response* response(ClientKind kind, const std::string& requestId) const {
            const ClientRecord& value = record(kind);
            const auto iterator = value.responses.find(requestId);
            return iterator == value.responses.end() ? nullptr : &iterator->second;
        }

        bool responseOk(ClientKind kind, const std::string& requestId) const {
            const frontend::Response* value = response(kind, requestId);
            return value && value->ok;
        }

        bool responseError(ClientKind kind, const std::string& requestId, frontend::ErrorCode code) const {
            const frontend::Response* value = response(kind, requestId);
            return value && !value->ok && value->error && value->error->code == code;
        }

        bool synchronized(ClientKind kind) const {
            const ClientRecord& value = record(kind);
            return value.welcomeCount == 1 && value.syncCompleteCount == 1;
        }

        void send(ClientKind kind, frontend::ClientMessage message) {
            ClientRecord& value = client(kind);
            if (value.context) {
                value.context->sendMessage(std::move(message));
            } else {
                result.expectTrue(false, clientName(kind) + " context remains connected for protocol send");
                fail();
            }
        }

        void advance() {
            if (advancing || finished) {
                return;
            }
            advancing = true;
            bool again = true;
            while (again && !finished) {
                again = false;
                switch (phase) {
                    case Phase::WaitingA:
                        if (synchronized(ClientKind::ControllerA)) {
                            const ClientRecord& a = record(ClientKind::ControllerA);
                            result.expectTrue(a.welcome && a.welcome->syncMode == frontend::SyncMode::Snapshot &&
                                                  a.welcome->role == frontend::SessionRole::Observer && a.snapshotCount == 1 &&
                                                  a.latestSnapshot && a.latestSnapshot->state.is_object(),
                                              "fragmented hello negotiates v1 and completes one full observer snapshot sync");
                            result.expectTrue(a.context && a.context->fragmentedWriteCount() == 2,
                                              "initial hello is emitted through two deterministic Unix-stream writes");
                            phase = Phase::WaitingB;
                            connect(ClientKind::ObserverB);
                        }
                        break;
                    case Phase::WaitingB:
                        if (synchronized(ClientKind::ObserverB)) {
                            result.expectTrue(record(ClientKind::ObserverB).welcome &&
                                                  record(ClientKind::ObserverB).welcome->role == frontend::SessionRole::Observer,
                                              "second Unix frontend independently begins as observer");
                            phase = Phase::WaitingAcquire;
                            send(ClientKind::ControllerA, command("acquire-a", frontend::ControllerAcquire{}));
                        }
                        break;
                    case Phase::WaitingAcquire:
                        if (responseOk(ClientKind::ControllerA, "acquire-a")) {
                            phase = Phase::WaitingThreadCommands;
                            send(ClientKind::ObserverB, command("observer-thread-start", frontend::ThreadStart{}));
                            frontend::ThreadStart start;
                            start.cwd = "/tmp/acceptance";
                            client(ClientKind::ControllerA)
                                .context->sendCoalesced(command("coalesced-snapshot", frontend::SnapshotGet{}),
                                                        command("thread-start", std::move(start)));
                        }
                        break;
                    case Phase::WaitingThreadCommands:
                        if (responseError(ClientKind::ObserverB, "observer-thread-start", frontend::ErrorCode::PermissionDenied) &&
                            responseOk(ClientKind::ControllerA, "coalesced-snapshot") &&
                            responseOk(ClientKind::ControllerA, "thread-start")) {
                            result.expectTrue(client(ClientKind::ControllerA).context->coalescedWriteCount() == 1,
                                              "two command frames in one Unix write are decoded and correlated independently");
                            result.expectTrue(
                                responseError(ClientKind::ObserverB, "observer-thread-start", frontend::ErrorCode::PermissionDenied),
                                "observer mutation is denied while controller thread.start succeeds");
                            phase = Phase::WaitingTurn;
                            frontend::TurnStart turn;
                            turn.threadId = "thread-acceptance";
                            turn.input = {frontend::TextInput{"acceptance text input", Json::object()}};
                            send(ClientKind::ControllerA, command("turn-start", std::move(turn)));
                        }
                        break;
                    case Phase::WaitingTurn:
                        if (responseOk(ClientKind::ControllerA, "turn-start")) {
                            result.expectTrue(
                                sawTypedTurnInput,
                                "turn.start reaches fake App Server as typed text input without raw Codex JSON from frontend");
                            phase = Phase::WaitingAgentBurst;
                            injectAgentBurst();
                        }
                        break;
                    case Phase::WaitingAgentBurst:
                        if (record(ClientKind::ControllerA).agentContent == expectedAgentText &&
                            record(ClientKind::ObserverB).agentContent == expectedAgentText) {
                            result.expectTrue(record(ClientKind::ControllerA).agentContentUpdates < 20 &&
                                                  record(ClientKind::ObserverB).agentContentUpdates < 20,
                                              "1,000 raw token deltas reconstruct exactly with substantially fewer frontend updates");
                            result.expectTrue(record(ClientKind::ControllerA).maximumBatchEvents <= frontend::DefaultBatchMaxEvents &&
                                                  record(ClientKind::ObserverB).maximumBatchEvents <= frontend::DefaultBatchMaxEvents,
                                              "Unix frontend event batches obey the stable event-count bound");
                            phase = Phase::WaitingApproval;
                            injectApprovalRequest();
                        }
                        break;
                    case Phase::WaitingApproval:
                        if (record(ClientKind::ControllerA).pendingRequestId && record(ClientKind::ObserverB).pendingRequestId) {
                            result.expectTrue(record(ClientKind::ControllerA).pendingRequestId ==
                                                  record(ClientKind::ObserverB).pendingRequestId,
                                              "controller and observer promptly see the same frontend pending-request id");
                            phase = Phase::WaitingApprovalResolution;
                            send(ClientKind::ControllerA,
                                 command("approval-decline",
                                         frontend::ApprovalRespond{*record(ClientKind::ControllerA).pendingRequestId, "decline"}));
                        }
                        break;
                    case Phase::WaitingApprovalResolution:
                        if (responseOk(ClientKind::ControllerA, "approval-decline") && record(ClientKind::ControllerA).requestResolved &&
                            record(ClientKind::ObserverB).requestResolved) {
                            result.expectTrue(approvalDeclineSent(),
                                              "controller decline is sent once through typed request ownership and removes pending state");
                            phase = Phase::WaitingTerminal;
                            injectCommandOutputAndCompletion();
                        }
                        break;
                    case Phase::WaitingTerminal:
                        if (record(ClientKind::ControllerA).terminalTurn && record(ClientKind::ObserverB).terminalTurn &&
                            record(ClientKind::ControllerA).commandOutput == expectedCommandOutput &&
                            record(ClientKind::ObserverB).commandOutput == expectedCommandOutput) {
                            result.expectTrue(record(ClientKind::ControllerA).commandOutputSequence <
                                                      record(ClientKind::ControllerA).terminalTurnSequence &&
                                                  record(ClientKind::ObserverB).commandOutputSequence <
                                                      record(ClientKind::ObserverB).terminalTurnSequence,
                                              "pending command output flushes before terminal turn update for both clients");
                            phase = Phase::WaitingADisconnect;
                            client(ClientKind::ControllerA).context->disconnect();
                        }
                        break;
                    case Phase::WaitingADisconnect:
                        if (record(ClientKind::ControllerA).disconnected && !backendCore.snapshot().controller) {
                            result.expectTrue(backendCore.snapshot().pendingRequests.empty() &&
                                                  backendCore.snapshot().lifecycle == backend::BackendLifecycle::Ready,
                                              "controller disconnect releases ownership without stopping the backend or "
                                              "synthesizing request answers");
                            phase = Phase::WaitingBAcquire;
                            send(ClientKind::ObserverB, command("acquire-b", frontend::ControllerAcquire{}));
                        } else if (record(ClientKind::ControllerA).disconnected && !advancePollScheduled) {
                            advancePollScheduled = true;
                            core::EventReceiver::atNextTick([this]() {
                                advancePollScheduled = false;
                                advance();
                            });
                        }
                        break;
                    case Phase::WaitingBAcquire:
                        if (responseOk(ClientKind::ObserverB, "acquire-b")) {
                            resumeAfterB = record(ClientKind::ObserverB).lastSequence;
                            oldResumeSequence = resumeAfterB;
                            phase = Phase::WaitingBDisconnect;
                            client(ClientKind::ObserverB).context->disconnect();
                        }
                        break;
                    case Phase::WaitingBDisconnect:
                        if (record(ClientKind::ObserverB).disconnected) {
                            phase = Phase::WaitingReplay;
                            connect(ClientKind::ReplayB);
                        }
                        break;
                    case Phase::WaitingReplay:
                        if (synchronized(ClientKind::ReplayB)) {
                            const ClientRecord& replay = record(ClientKind::ReplayB);
                            result.expectTrue(replay.welcome && replay.welcome->syncMode == frontend::SyncMode::Replay &&
                                                  replay.snapshotCount == 0 && !replay.events.empty(),
                                              "reconnect after retained sequence receives normalized replay without snapshot");
                            phase = Phase::WaitingReplayAcquire;
                            send(ClientKind::ReplayB, command("acquire-replay", frontend::ControllerAcquire{}));
                        }
                        break;
                    case Phase::WaitingReplayAcquire:
                        if (responseOk(ClientKind::ReplayB, "acquire-replay")) {
                            phase = Phase::EvictingJournal;
                            injectNextEvictionChunk();
                        }
                        break;
                    case Phase::EvictingJournal:
                        if (record(ClientKind::ReplayB).highestEvictionExtension >= evictionInjected) {
                            if (evictionInjected < EvictionEventCount) {
                                injectNextEvictionChunk();
                            } else {
                                result.expectTrue(record(ClientKind::ReplayB).highestEvictionExtension == EvictionEventCount,
                                                  "forced eviction traffic remains ordered through the real Unix adapter");
                                phase = Phase::WaitingOldSnapshot;
                                connect(ClientKind::OldReconnect);
                            }
                        }
                        break;
                    case Phase::WaitingOldSnapshot:
                        if (synchronized(ClientKind::OldReconnect)) {
                            const ClientRecord& old = record(ClientKind::OldReconnect);
                            result.expectTrue(old.welcome && old.welcome->syncMode == frontend::SyncMode::Snapshot &&
                                                  old.snapshotCount == 1,
                                              "resume position older than bounded journal deterministically falls back to snapshot");
                            phase = Phase::WaitingMalformed;
                            connect(ClientKind::Malformed);
                        }
                        break;
                    case Phase::WaitingMalformed:
                        if (record(ClientKind::Malformed).disconnected) {
                            result.expectTrue(record(ClientKind::Malformed).protocolError == frontend::ErrorCode::MalformedJson &&
                                                  record(ClientKind::Malformed).welcomeCount == 0 &&
                                                  record(ClientKind::OldReconnect).connected &&
                                                  !record(ClientKind::OldReconnect).disconnected,
                                              "malformed JSON blocks later coalesced frames, closes only its Unix frontend, and leaves a "
                                              "healthy client connected");
                            phase = Phase::WaitingOversized;
                            connect(ClientKind::Oversized);
                        }
                        break;
                    case Phase::WaitingOversized:
                        if (record(ClientKind::Oversized).disconnected) {
                            result.expectTrue(record(ClientKind::Oversized).protocolError == frontend::ErrorCode::FrameTooLarge &&
                                                  !record(ClientKind::OldReconnect).disconnected,
                                              "oversized unterminated frame is isolated with stable frame_too_large error");
                            phase = Phase::WaitingBackendStop;
                            backendCore.stop();
                            waitForBackendStopped();
                        }
                        break;
                    case Phase::WaitingBackendStop:
                    case Phase::Finished:
                        break;
                }
            }
            advancing = false;
        }

        void handleOutgoing(const Json& message, const TransportCallbacks& callbacks) {
            const auto method = message.find("method");
            if (method == message.end()) {
                return;
            }
            if (!method->is_string()) {
                return;
            }
            const auto id = message.find("id");
            if (id == message.end()) {
                return;
            }
            const std::string methodName = method->get<std::string>();
            if (methodName == "thread/list") {
                tests::codex::inject(
                    callbacks,
                    Json{{"id", *id}, {"result", {{"data", Json::array()}, {"nextCursor", nullptr}, {"backwardsCursor", nullptr}}}});
            } else if (methodName == "thread/start") {
                tests::codex::inject(callbacks, Json{{"id", *id}, {"result", tests::codex::threadOperationResult("thread-acceptance")}});
            } else if (methodName == "turn/start") {
                const Json params = message.value("params", Json::object());
                sawTypedTurnInput = params.value("threadId", "") == "thread-acceptance" && params.contains("input") &&
                                    params["input"].is_array() && params["input"].size() == 1 &&
                                    params["input"][0].value("type", "") == "text" &&
                                    params["input"][0].value("text", "") == "acceptance text input";
                tests::codex::inject(
                    callbacks, Json{{"id", *id}, {"result", tests::codex::turnOperationResult("thread-acceptance", "turn-acceptance")}});
            }
        }

        bool approvalDeclineSent() const noexcept {
            return std::count_if(transport->outgoing.begin(), transport->outgoing.end(), [](const Json& message) {
                       const auto id = message.find("id");
                       const auto resultValue = message.find("result");
                       if (id == message.end() || *id != "approval-server" || resultValue == message.end() || !resultValue->is_object()) {
                           return false;
                       }
                       const auto decision = resultValue->find("decision");
                       return decision != resultValue->end() && decision->is_string() && *decision == "decline";
                   }) == 1;
        }

        void injectAgentBurst() {
            transport->inject({{"method", "item/started"},
                               {"params",
                                {{"threadId", "thread-acceptance"},
                                 {"turnId", "turn-acceptance"},
                                 {"item", agentItem("agent-acceptance")},
                                 {"startedAtMs", 1}}}});
            expectedAgentText.clear();
            expectedAgentText.reserve(2000);
            for (std::size_t index = 0; index < 1000; ++index) {
                const std::string delta = std::to_string(index % 10);
                expectedAgentText += delta;
                transport->inject({{"method", "item/agentMessage/delta"},
                                   {"params",
                                    {{"threadId", "thread-acceptance"},
                                     {"turnId", "turn-acceptance"},
                                     {"itemId", "agent-acceptance"},
                                     {"delta", delta}}}});
            }
        }

        void injectApprovalRequest() {
            transport->inject({{"method", "item/commandExecution/requestApproval"},
                               {"id", "approval-server"},
                               {"params",
                                {{"threadId", "thread-acceptance"},
                                 {"turnId", "turn-acceptance"},
                                 {"itemId", "command-acceptance"},
                                 {"startedAtMs", 2},
                                 {"command", "printf acceptance"},
                                 {"cwd", "/tmp/project"}}}});
        }

        void injectCommandOutputAndCompletion() {
            transport->inject({{"method", "item/completed"},
                               {"params",
                                {{"threadId", "thread-acceptance"},
                                 {"turnId", "turn-acceptance"},
                                 {"item", agentItem("agent-acceptance", expectedAgentText)},
                                 {"completedAtMs", 3}}}});
            transport->inject({{"method", "item/started"},
                               {"params",
                                {{"threadId", "thread-acceptance"},
                                 {"turnId", "turn-acceptance"},
                                 {"item", commandItem("command-acceptance", "inProgress")},
                                 {"startedAtMs", 4}}}});
            expectedCommandOutput = "first line\nsecond line\nfinal line\n";
            for (const std::string& delta : {std::string("first line\n"), std::string("second line\n"), std::string("final line\n")}) {
                transport->inject({{"method", "item/commandExecution/outputDelta"},
                                   {"params",
                                    {{"threadId", "thread-acceptance"},
                                     {"turnId", "turn-acceptance"},
                                     {"itemId", "command-acceptance"},
                                     {"delta", delta}}}});
            }
            transport->inject({{"method", "item/completed"},
                               {"params",
                                {{"threadId", "thread-acceptance"},
                                 {"turnId", "turn-acceptance"},
                                 {"item", commandItem("command-acceptance", "completed", expectedCommandOutput)},
                                 {"completedAtMs", 5}}}});
            transport->inject({{"method", "turn/completed"},
                               {"params",
                                {{"threadId", "thread-acceptance"},
                                 {"turn", tests::codex::turnValue("thread-acceptance", "turn-acceptance", "completed")}}}});
        }

        void injectNextEvictionChunk() {
            const std::size_t end = std::min(EvictionEventCount, evictionInjected + EvictionChunkSize);
            for (std::size_t index = evictionInjected + 1; index <= end; ++index) {
                transport->inject({{"method", "acceptance/evict/" + std::to_string(index)}, {"params", {{"index", index}}}});
            }
            evictionInjected = end;
        }

        void waitForBackendStopped(std::size_t remaining = 2000) {
            core::EventReceiver::atNextTick([this, remaining]() {
                if (finished) {
                    return;
                }
                if (backendCore.snapshot().lifecycle == backend::BackendLifecycle::Stopped) {
                    result.expectTrue(transport->stopCount != 0,
                                      "backend stop reaches the owned fake App Server transport while Unix clients are connected");
                    if (client(ClientKind::OldReconnect).context) {
                        client(ClientKind::OldReconnect).context->disconnect();
                    }
                    if (client(ClientKind::ReplayB).context) {
                        client(ClientKind::ReplayB).context->disconnect();
                    }
                    phase = Phase::Finished;
                    finish();
                } else if (remaining == 0) {
                    result.expectTrue(false, "BackendCore reaches Stopped during Unix acceptance shutdown");
                    fail();
                } else {
                    waitForBackendStopped(remaining - 1);
                }
            });
        }

        void fail() {
            if (finished) {
                return;
            }
            backendCore.stop();
            finish();
        }

        void finish() {
            if (finished) {
                return;
            }
            finished = true;
            core::SNodeC::stop();
        }

    public:
        void installFakeProtocol() {
            tests::codex::installInitializingFake(transport, [this](const Json& message, const TransportCallbacks& callbacks) {
                handleOutgoing(message, callbacks);
            });
        }

    private:
        tests::support::TestResult& result;
        FakeBackendCore& backendCore;
        std::shared_ptr<tests::codex::FakeTransportState> transport;
        ClientHooks hooks;
        std::function<void(ClientKind)> connector;
        std::array<ClientRecord, clientIndex(ClientKind::Count)> clients;
        Phase phase = Phase::WaitingA;
        bool advancing = false;
        bool finished = false;
        std::size_t listenSuccesses = 0;
        std::size_t listenFailures = 0;
        std::size_t connectFailures = 0;
        std::size_t decodeFailures = 0;
        std::uint64_t resumeAfterB = 0;
        std::uint64_t oldResumeSequence = 0;
        std::size_t evictionInjected = 0;
        std::string expectedAgentText;
        std::string expectedCommandOutput;
        bool sawTypedTurnInput = false;
        bool advancePollScheduled = false;
    };

    std::string socketPath() {
        return "/tmp/snodec-codex-backend-acceptance-" + std::to_string(::getpid()) + ".sock";
    }

    bool pathExists(const std::string& path) {
        return ::access(path.c_str(), F_OK) == 0;
    }
} // namespace

int main(int argc, char* argv[]) {
    tests::support::TestResult result;
    int returnCode = tests::support::cTestSkipReturnCode;
    const std::string path = socketPath();

    if (tests::support::shouldSkipRootWithoutSNodeCGroup()) {
        tests::support::printRootWithoutSNodeCGroupSkipMessage("CodexBackendUnixAcceptanceTest");
    } else {
        std::remove(path.c_str());
        result.expectTrue(!pathExists(path), "unique Unix acceptance socket path is absent before listen");
        core::SNodeC::init(argc, argv);

        bool timedOut = false;
        int eventLoopResult = 1;
        bool scenarioFinished = false;
        std::size_t listenSuccesses = 0;
        std::size_t listenFailures = 0;
        std::size_t connectionFailures = 0;
        std::size_t decodingFailures = 0;
        bool typedInput = false;
        bool declinedApproval = false;
        {
            const auto transport = std::make_shared<tests::codex::FakeTransportState>();
            backend::BackendCoreOptions backendOptions;
            backendOptions.initialThreadListLimit = 2;
            backendOptions.maxEventsPerCallback = 512;
            FakeBackendCore backendCore(backendOptions, transport);

            apps::codex_backend::SocketFrontendOptions frontendOptions;
            frontendOptions.maximumFrameSize = MaximumFrameSize;
            frontendOptions.maximumOutboundBytes = 2U * 1024U * 1024U;
            const net::un::stream::legacy::SocketServer<apps::codex_backend::CodexFrontendSocketContextFactory,
                                                        FakeBackendCore&,
                                                        apps::codex_backend::SocketFrontendOptions>
                server("codex-backend-acceptance-server", backendCore, std::move(frontendOptions));
            server.getConfig()->Instance::forceUnrequired();

            AcceptanceScenario scenario(result, backendCore, transport);
            scenario.installFakeProtocol();

            using Client = net::un::stream::legacy::SocketClient<TestFrontendClientFactory, ClientHooks&, ClientKind>;
            Client controllerA("codex-backend-acceptance-controller-a", scenario.clientHooks(), ClientKind::ControllerA);
            Client observerB("codex-backend-acceptance-observer-b", scenario.clientHooks(), ClientKind::ObserverB);
            Client replayB("codex-backend-acceptance-replay-b", scenario.clientHooks(), ClientKind::ReplayB);
            Client oldReconnect("codex-backend-acceptance-old", scenario.clientHooks(), ClientKind::OldReconnect);
            Client malformed("codex-backend-acceptance-malformed", scenario.clientHooks(), ClientKind::Malformed);
            Client oversized("codex-backend-acceptance-oversized", scenario.clientHooks(), ClientKind::Oversized);
            for (Client* client : {&controllerA, &observerB, &replayB, &oldReconnect, &malformed, &oversized}) {
                client->getConfig()->Instance::forceUnrequired();
            }

            scenario.setConnector([&](ClientKind kind) {
                Client* selected = nullptr;
                switch (kind) {
                    case ClientKind::ControllerA:
                        selected = &controllerA;
                        break;
                    case ClientKind::ObserverB:
                        selected = &observerB;
                        break;
                    case ClientKind::ReplayB:
                        selected = &replayB;
                        break;
                    case ClientKind::OldReconnect:
                        selected = &oldReconnect;
                        break;
                    case ClientKind::Malformed:
                        selected = &malformed;
                        break;
                    case ClientKind::Oversized:
                        selected = &oversized;
                        break;
                    case ClientKind::Count:
                        break;
                }
                if (selected) {
                    selected->connect(path, [&scenario, kind](const net::un::SocketAddress&, core::socket::State state) {
                        scenario.connectionResult(kind, state);
                    });
                }
            });

            server.listen(path, [&scenario](const net::un::SocketAddress&, core::socket::State state) {
                if (state == core::socket::State::OK) {
                    scenario.listenReady();
                } else {
                    scenario.listenFailed();
                }
            });
            backendCore.start();

            [[maybe_unused]] core::timer::Timer watchdog = core::timer::Timer::singleshotTimer(
                [&timedOut]() {
                    timedOut = true;
                    core::SNodeC::stop();
                },
                utils::Timeval({15, 0}));
            eventLoopResult = core::SNodeC::start(utils::Timeval({18, 0}));

            scenarioFinished = scenario.isFinished();
            listenSuccesses = scenario.successfulListens();
            listenFailures = scenario.failedListens();
            connectionFailures = scenario.connectionFailures();
            decodingFailures = scenario.decodingFailures();
            typedInput = scenario.typedTurnInputObserved();
            declinedApproval = scenario.approvalDeclineObserved();
        }

        const bool pathCleanedByServer = !pathExists(path);
        result.expectTrue(!timedOut, "full Unix acceptance scenario completes before watchdog");
        result.expectTrue(scenarioFinished, "full fake-App-Server through Unix frontend scenario reaches completion");
        result.expectEqual(0, eventLoopResult, "Unix acceptance event loop stops cleanly");
        result.expectEqual(1, static_cast<int>(listenSuccesses), "real Unix SocketServer listens exactly once");
        result.expectEqual(0, static_cast<int>(listenFailures), "real Unix SocketServer reports no listen failure");
        result.expectEqual(0, static_cast<int>(connectionFailures), "all scheduled Unix frontend clients connect successfully");
        result.expectEqual(0, static_cast<int>(decodingFailures), "all server JSONL frames decode as Frontend Protocol v1");
        result.expectTrue(typedInput && declinedApproval, "typed turn input and explicit decline traversed the complete stack");
        result.expectTrue(pathCleanedByServer, "Unix SocketServer removes its socket path on clean destruction");

        core::SNodeC::free();
        returnCode = result.processResult();
    }

    std::remove(path.c_str());
    return returnCode;
}
