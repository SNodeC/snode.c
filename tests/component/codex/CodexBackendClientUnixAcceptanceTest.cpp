/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include "ai/openai/codex/frontend/Codec.h"
#include "ai/openai/codex/frontend/Messages.h"
#include "apps/codex-backend-client/ClientConnection.h"
#include "apps/codex-backend-client/CodexBackendClientSocketContextFactory.h"
#include "apps/codex-backend-client/CommandDrainController.h"
#include "apps/codex-backend-client/CommandParser.h"
#include "apps/codex-backend-client/JsonLineFramer.h"
#include "apps/codex-backend-client/Presenter.h"
#include "apps/codex-backend-client/StdinReader.h"
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
#include <cerrno>
#include <cstddef>
#include <cstdio>
#include <exception>
#include <fcntl.h>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <unistd.h>
#include <utility>
#include <variant>
#include <vector>

namespace {
    namespace client = apps::codex_backend_client;
    namespace frontend = ai::openai::codex::frontend;

    constexpr std::size_t TestMaximumFrameSize = 64 * 1024;
    constexpr frontend::SequenceNumber HandshakeSequence{7};

    std::string socketPath() {
        return "/tmp/snodec-codex-backend-client-acceptance-" + std::to_string(::getpid()) + ".sock";
    }

    bool pathExists(const std::string& path) {
        return ::access(path.c_str(), F_OK) == 0;
    }

    class Pipe final {
    public:
        Pipe() {
            int descriptors[2] = {-1, -1};
            if (::pipe(descriptors) == 0) {
                readFd = descriptors[0];
                writeFd = descriptors[1];
            }
        }

        Pipe(const Pipe&) = delete;
        Pipe(Pipe&&) = delete;

        Pipe& operator=(const Pipe&) = delete;
        Pipe& operator=(Pipe&&) = delete;

        ~Pipe() {
            if (readFd >= 0) {
                static_cast<void>(::close(readFd));
            }
            if (writeFd >= 0) {
                static_cast<void>(::close(writeFd));
            }
        }

        [[nodiscard]] bool valid() const noexcept {
            return readFd >= 0 && writeFd >= 0;
        }

        [[nodiscard]] bool writeAll(std::string_view data) const noexcept {
            std::size_t offset = 0;
            while (offset < data.size()) {
                const ssize_t count = ::write(writeFd, data.data() + offset, data.size() - offset);
                if (count > 0) {
                    offset += static_cast<std::size_t>(count);
                } else if (count < 0 && errno == EINTR) {
                    continue;
                } else {
                    return false;
                }
            }
            return true;
        }

        [[nodiscard]] bool closeWriter() noexcept {
            if (writeFd < 0) {
                return true;
            }
            const int descriptor = std::exchange(writeFd, -1);
            return ::close(descriptor) == 0;
        }

        int readFd = -1;

    private:
        int writeFd = -1;
    };

    struct ScenarioState {
        explicit ScenarioState(tests::support::TestResult& result)
            : result(result)
            , parser("acceptance") {
        }

        void fail(const std::string& message) {
            ++failureCount;
            result.expectTrue(false, message);
            core::SNodeC::stop();
        }

        void stopWhenDisconnected() {
            if (clientDisconnectedCount == 1 && serverDisconnectedCount == 1) {
                completed = true;
                core::SNodeC::stop();
            }
        }

        void inputLine(std::string line, client::CommandDrainController& lifecycle) {
            ++stdinLineCount;
            result.expectTrue(connection == nullptr || !connection->connected(),
                              "piped commands are parsed before the Unix connection is established");

            client::ParsedCommand parsed = parser.parse(line);
            auto* send = std::get_if<client::SendCommand>(&parsed);
            auto* command = send == nullptr ? nullptr : std::get_if<frontend::Command>(&send->message);
            if (command == nullptr) {
                fail("production parser did not produce a typed command for piped input");
                return;
            }

            const bool acquire = std::holds_alternative<frontend::ControllerAcquire>(command->parameters);
            const bool threads = std::holds_alternative<frontend::ThreadList>(command->parameters);
            const bool expected =
                (stdinLineCount == 1 && line == "acquire" && acquire) || (stdinLineCount == 2 && line == "threads" && threads);
            result.expectTrue(expected, "production stdin and parser preserve the acquire/threads input order");
            if (!expected) {
                fail("piped input did not decode to acquire followed by threads");
                return;
            }

            if (acquire) {
                acquireRequestId = command->requestId;
            } else {
                threadsRequestId = command->requestId;
            }

            const bool queued = lifecycle.enqueue(std::move(send->message));
            result.expectTrue(queued, "production drain lifecycle accepts a preconnection piped command");
            if (!queued) {
                fail("production drain lifecycle rejected a preconnection piped command");
            }
        }

        void clientConnected(client::CommandDrainController& lifecycle) {
            ++clientContextConnectedCount;
            result.expectTrue(stdinEofCount == 1 && stdinLineCount == 2 && lifecycle.queuedCount() == 2 && commandCount == 0,
                              "both piped commands and EOF occur before the production client connects");
            lifecycle.connected();
        }

        void clientDisconnected(client::CommandDrainController& lifecycle) {
            lifecycle.disconnected();
            ++clientDisconnectedCount;
            lifecycleDrainedCleanly = lifecycle.outcome() == client::CommandDrainController::Outcome::Success &&
                                      lifecycle.sessionState() == client::CommandDrainController::SessionState::Closed &&
                                      lifecycle.queuedCount() == 0 && lifecycle.pendingResponseCount() == 0 &&
                                      lifecycle.pendingSyncCount() == 0;
            result.expectTrue(lifecycleDrainedCleanly, "the controlled post-drain disconnect preserves a successful lifecycle outcome");
            stopWhenDisconnected();
        }

        void clientProtocolError(client::CommandDrainController& lifecycle, const frontend::CodecError& error) {
            lifecycle.protocolFailed(std::string(frontend::toString(error.code)) + ": " + error.message);
        }

        void clientMessage(const frontend::ServerMessage& message) {
            if (const auto* welcome = std::get_if<frontend::Welcome>(&message)) {
                ++welcomeCount;
                result.expectTrue(welcome->sessionId == "acceptance-session" && welcome->role == frontend::SessionRole::Observer &&
                                      welcome->currentSequence == HandshakeSequence && welcome->syncMode == frontend::SyncMode::Snapshot,
                                  "production client decodes and presents the fragmented welcome exactly");
                return;
            }

            if (const auto* snapshot = std::get_if<frontend::Snapshot>(&message)) {
                ++snapshotCount;
                result.expectTrue(snapshot->sequence == HandshakeSequence && snapshot->state == expectedSnapshot,
                                  "production client decodes and presents the coalesced snapshot exactly");
                return;
            }

            if (const auto* complete = std::get_if<frontend::SyncComplete>(&message)) {
                ++syncCompleteCount;
                result.expectTrue(complete->sequence == HandshakeSequence && commandCount == 0,
                                  "initial sync.complete is accepted and presented before the server receives released queued commands");
                return;
            }

            if (const auto* response = std::get_if<frontend::Response>(&message)) {
                ++responseCount;
                const bool acquireResponse = responseCount == 1 && response->requestId == acquireRequestId && response->ok &&
                                             response->result == std::optional<frontend::Json>{frontend::Json{{"role", "controller"}}};
                const bool threadsResponse =
                    responseCount == 2 && response->requestId == threadsRequestId && response->ok &&
                    response->result == std::optional<frontend::Json>{frontend::Json{{"threads", frontend::Json::array()}}};
                result.expectTrue(acquireResponse || threadsResponse,
                                  "production client presents each ordered correlated response exactly");
                if (!acquireResponse && !threadsResponse) {
                    fail("production client received an unexpected or uncorrelated response");
                }
                return;
            }

            if (const auto* events = std::get_if<frontend::EventBatch>(&message)) {
                ++trailingEventBatchCount;
                result.expectTrue(events->fromSequence == frontend::SequenceNumber{8} &&
                                      events->toSequence == frontend::SequenceNumber{8} && events->events.size() == 1 &&
                                      events->events.front().sequence == frontend::SequenceNumber{8} &&
                                      events->events.front().type == "acceptance.trailing" &&
                                      events->events.front().data == frontend::Json{{"presented", true}},
                                  "a frame coalesced after the final response is decoded and presented before disconnect");
                return;
            }

            fail("production client received an unexpected deterministic server message");
        }

        tests::support::TestResult& result;
        client::CommandParser parser;
        client::ClientConnection* connection = nullptr;
        frontend::Json expectedSnapshot{{"lifecycle", "ready"}, {"threads", frontend::Json::array()}};
        std::string acquireRequestId;
        std::string threadsRequestId;
        bool completed = false;
        bool initialSyncSent = false;
        bool pipeFlagsRestoredAtEof = false;
        bool lifecycleDrainedCleanly = false;
        std::size_t failureCount = 0;
        std::size_t serverListenOkCount = 0;
        std::size_t stdinLineCount = 0;
        std::size_t stdinEofCount = 0;
        std::size_t connectRequestsFromEof = 0;
        std::size_t clientConnectOkCount = 0;
        std::size_t serverConnectedCount = 0;
        std::size_t serverDisconnectedCount = 0;
        std::size_t clientContextConnectedCount = 0;
        std::size_t clientDisconnectedCount = 0;
        std::size_t helloCount = 0;
        std::size_t commandCount = 0;
        std::size_t acquireCount = 0;
        std::size_t threadsCount = 0;
        std::size_t welcomeCount = 0;
        std::size_t snapshotCount = 0;
        std::size_t syncCompleteCount = 0;
        std::size_t responseCount = 0;
        std::size_t trailingEventBatchCount = 0;
        std::size_t fragmentedWrites = 0;
        std::size_t coalescedHandshakeMessages = 0;
        std::size_t coalescedDrainMessages = 0;
        std::size_t lifecycleExitRequests = 0;
        std::size_t controlledDisconnects = 0;
        std::vector<std::string> serverRequestOrder;
        std::vector<std::string> lifecycleFailures;
    };

    class FakeFrontendServerContext final : public core::socket::stream::SocketContext {
    public:
        FakeFrontendServerContext(core::socket::stream::SocketConnection* socketConnection, ScenarioState& state)
            : core::socket::stream::SocketContext(socketConnection)
            , state(state)
            , framer(TestMaximumFrameSize) {
        }

    private:
        std::optional<std::string> frame(const frontend::ServerMessage& message) {
            const auto encoded = frontend::Codec::serializeServer(message);
            if (!encoded) {
                state.fail("fake frontend server could not serialize a typed server message: " + encoded.error().message);
                return std::nullopt;
            }
            std::string bytes = encoded.value();
            bytes.push_back('\n');
            return bytes;
        }

        void sendHandshake() {
            const auto welcome = frame(frontend::ServerMessage{frontend::Welcome{"acceptance-session",
                                                                                 frontend::SessionRole::Observer,
                                                                                 HandshakeSequence,
                                                                                 frontend::SyncMode::Snapshot,
                                                                                 frontend::Json::object()}});
            const auto snapshot =
                frame(frontend::ServerMessage{frontend::Snapshot{HandshakeSequence, state.expectedSnapshot, frontend::Json::object()}});
            const auto complete = frame(frontend::ServerMessage{frontend::SyncComplete{HandshakeSequence, frontend::Json::object()}});
            if (!welcome || !snapshot || !complete) {
                return;
            }

            fragmentedWelcome = *welcome;
            const std::size_t split = std::max<std::size_t>(1, fragmentedWelcome.size() / 2);
            sendToPeer(fragmentedWelcome.data(), split);
            ++state.fragmentedWrites;

            coalescedHandshake = fragmentedWelcome.substr(split) + *snapshot + *complete;
            fragmentTimer = core::timer::Timer::singleshotTimer(
                [this]() {
                    sendToPeer(coalescedHandshake.data(), coalescedHandshake.size());
                    ++state.fragmentedWrites;
                    state.coalescedHandshakeMessages = 3;
                    state.initialSyncSent = true;
                },
                utils::Timeval({0, 1000}));
        }

        void clientMessage(std::string encoded) {
            const auto decoded = frontend::Codec::decodeClient(std::string_view(encoded));
            if (!decoded) {
                state.fail("fake frontend server could not decode production client message: " + decoded.error().message);
                close();
                return;
            }

            if (const auto* hello = std::get_if<frontend::Hello>(&decoded.value())) {
                ++state.helloCount;
                state.result.expectTrue(state.helloCount == 1 && !hello->resumeAfter.has_value() && state.commandCount == 0,
                                        "production socket context automatically sends exactly one hello before commands");
                sendHandshake();
                return;
            }

            const auto* command = std::get_if<frontend::Command>(&decoded.value());
            if (command == nullptr) {
                state.fail("fake frontend server received an unexpected non-command after hello");
                close();
                return;
            }

            ++state.commandCount;
            state.serverRequestOrder.push_back(command->requestId);
            const bool acquire = state.commandCount == 1 && command->requestId == state.acquireRequestId &&
                                 std::holds_alternative<frontend::ControllerAcquire>(command->parameters);
            const bool threads = state.commandCount == 2 && command->requestId == state.threadsRequestId &&
                                 std::holds_alternative<frontend::ThreadList>(command->parameters);
            state.result.expectTrue(state.initialSyncSent && state.syncCompleteCount == 1 && (acquire || threads),
                                    "fake server receives acquire then threads only after initial sync.complete");
            if (!acquire && !threads) {
                state.fail("fake frontend server did not receive acquire followed by threads exactly once");
                close();
                return;
            }

            if (acquire) {
                ++state.acquireCount;
            } else {
                ++state.threadsCount;
            }

            if (state.commandCount == 2) {
                const auto acquireResponse = frame(
                    frontend::ServerMessage{frontend::Response::success(state.acquireRequestId, frontend::Json{{"role", "controller"}})});
                const auto threadsResponse = frame(frontend::ServerMessage{
                    frontend::Response::success(state.threadsRequestId, frontend::Json{{"threads", frontend::Json::array()}})});
                const auto trailingEvents = frame(frontend::ServerMessage{frontend::EventBatch{
                    frontend::SequenceNumber{8},
                    frontend::SequenceNumber{8},
                    {frontend::FrontendEvent{
                        frontend::SequenceNumber{8}, "acceptance.trailing", frontend::Json{{"presented", true}}, frontend::Json::object()}},
                    frontend::Json::object()}});
                if (acquireResponse && threadsResponse && trailingEvents) {
                    coalescedDrain = *acquireResponse + *threadsResponse + *trailingEvents;
                    sendToPeer(coalescedDrain.data(), coalescedDrain.size());
                    state.coalescedDrainMessages = 3;
                }
            }
        }

        void onConnected() override {
            ++state.serverConnectedCount;
        }

        void onDisconnected() override {
            ++state.serverDisconnectedCount;
            state.stopWhenDisconnected();
        }

        std::size_t onReceivedFromPeer() override {
            std::array<char, 16 * 1024> chunk{};
            const std::size_t size = readFromPeer(chunk.data(), chunk.size());
            if (size == 0) {
                return size;
            }

            const auto framing = framer.push(std::string_view(chunk.data(), size), [this](std::string encoded) {
                clientMessage(std::move(encoded));
            });
            if (framing == client::JsonLineFramer::Result::FrameTooLarge) {
                state.fail("production client emitted a frame beyond the deterministic server bound");
                close();
            }
            return size;
        }

        bool onSignal([[maybe_unused]] int signum) override {
            return true;
        }

        ScenarioState& state;
        client::JsonLineFramer framer;
        std::string fragmentedWelcome;
        std::string coalescedHandshake;
        std::string coalescedDrain;
        core::timer::Timer fragmentTimer;
    };

    class FakeFrontendServerFactory final : public core::socket::stream::SocketContextFactory {
    public:
        explicit FakeFrontendServerFactory(ScenarioState& state)
            : state(state) {
        }

        core::socket::stream::SocketContext* create(core::socket::stream::SocketConnection* socketConnection) override {
            return new FakeFrontendServerContext(socketConnection, state);
        }

    private:
        ScenarioState& state;
    };

    std::optional<std::string> jsonOutput(const std::vector<frontend::ServerMessage>& messages) {
        std::string output;
        for (const frontend::ServerMessage& message : messages) {
            const auto encoded = frontend::Codec::serializeServer(message);
            if (!encoded) {
                return std::nullopt;
            }
            output += encoded.value();
            output.push_back('\n');
        }
        return output;
    }
} // namespace

int main(int argc, char* argv[]) {
    tests::support::TestResult result;
    int returnCode = tests::support::cTestSkipReturnCode;
    const std::string path = socketPath();

    if (tests::support::shouldSkipRootWithoutSNodeCGroup()) {
        tests::support::printRootWithoutSNodeCGroupSkipMessage("CodexBackendClientUnixAcceptanceTest");
    } else {
        std::remove(path.c_str());
        result.expectTrue(!pathExists(path), "the unique client acceptance socket path is absent before listen");
        core::SNodeC::init(argc, argv);

        Pipe stdinPipe;
        const int stdinOriginalFlags = stdinPipe.valid() ? ::fcntl(stdinPipe.readFd, F_GETFL) : -1;
        const bool pipeReady =
            stdinPipe.valid() && stdinOriginalFlags >= 0 && stdinPipe.writeAll("acquire\nthreads\n") && stdinPipe.closeWriter();
        result.expectTrue(pipeReady, "the deterministic acquire/threads pipe and EOF are prepared");
        if (!pipeReady) {
            core::SNodeC::free();
            std::remove(path.c_str());
            return result.processResult();
        }

        ScenarioState state(result);
        std::ostringstream protocolOutput;
        std::ostringstream diagnostics;
        client::Presenter presenter(client::OutputMode::Json, protocolOutput, diagnostics);
        bool timedOut = false;
        int eventLoopResult = 1;
        bool connectionClosed = false;
        int stdinFlagsWhileActive = -1;
        int stdinFlagsAfterEventLoop = -1;
        int stdinFlagsAfterReaderDestruction = -1;
        client::ClientConnection* connectionHandle = nullptr;
        std::unique_ptr<client::StdinReader> stdinReader;

        {
            client::CommandDrainController lifecycle(client::CommandDrainCallbacks{
                .send =
                    [&connectionHandle](const frontend::ClientMessage& message) {
                        return connectionHandle != nullptr && connectionHandle->send(message);
                    },
                .requestExit =
                    [&state, &connectionHandle]() {
                        ++state.lifecycleExitRequests;
                        core::EventReceiver::atNextTick([&state, &connectionHandle]() {
                            if (connectionHandle == nullptr || !connectionHandle->connected()) {
                                state.fail("drain completion could not perform its controlled disconnect");
                                return;
                            }
                            ++state.controlledDisconnects;
                            connectionHandle->disconnect();
                        });
                    },
                .reportFailure =
                    [&state, &presenter](std::string message) {
                        state.lifecycleFailures.push_back(message);
                        presenter.error(message);
                        state.fail("production drain lifecycle failed during successful pipe acceptance: " + message);
                    }});

            client::ClientConnection connection(
                client::ClientConnectionCallbacks{.onConnected =
                                                      [&state, &lifecycle, &presenter, &path]() {
                                                          presenter.connected(path);
                                                          state.clientConnected(lifecycle);
                                                      },
                                                  .onMessage =
                                                      [&state, &lifecycle, &presenter](const frontend::ServerMessage& message) {
                                                          lifecycle.receive(message);
                                                          presenter.present(message);
                                                          state.clientMessage(message);
                                                      },
                                                  .onProtocolError =
                                                      [&state, &lifecycle](const frontend::CodecError& error) {
                                                          state.clientProtocolError(lifecycle, error);
                                                      },
                                                  .onDisconnected =
                                                      [&state, &lifecycle, &presenter]() {
                                                          presenter.disconnected();
                                                          state.clientDisconnected(lifecycle);
                                                      }});
            connectionHandle = &connection;
            state.connection = &connection;

            using Server = net::un::stream::legacy::SocketServer<FakeFrontendServerFactory, ScenarioState&>;
            using Client = net::un::stream::legacy::
                SocketClient<client::CodexBackendClientSocketContextFactory, client::ClientConnection&, std::size_t>;

            Server server("codex-backend-client-acceptance-server", state);
            Client socketClient("codex-backend-client-acceptance-client", connection, std::size_t{TestMaximumFrameSize});
            server.getConfig()->Instance::forceUnrequired();
            socketClient.getConfig()->Instance::forceUnrequired();

            server.listen(
                path,
                [&state, &lifecycle, &socketClient, &path, &stdinPipe, stdinOriginalFlags, &stdinFlagsWhileActive, &stdinReader](
                    const net::un::SocketAddress&, core::socket::State listenState) {
                    if (listenState != core::socket::State::OK) {
                        state.fail("deterministic fake Unix frontend server failed to listen");
                        return;
                    }
                    ++state.serverListenOkCount;

                    try {
                        stdinReader = std::make_unique<client::StdinReader>(
                            [&state, &lifecycle](std::string line) {
                                state.inputLine(std::move(line), lifecycle);
                            },
                            [&state, &lifecycle, &socketClient, &path, &stdinPipe, stdinOriginalFlags]() {
                                ++state.stdinEofCount;
                                state.pipeFlagsRestoredAtEof = ::fcntl(stdinPipe.readFd, F_GETFL) == stdinOriginalFlags;
                                state.result.expectTrue(state.stdinLineCount == 2 && lifecycle.queuedCount() == 2 &&
                                                            state.pipeFlagsRestoredAtEof,
                                                        "EOF follows both queued commands and restores stdin flags");
                                lifecycle.inputEof();
                                state.result.expectTrue(lifecycle.inputState() == client::CommandDrainController::InputState::DrainOnEof &&
                                                            lifecycle.outcome() == client::CommandDrainController::Outcome::Running,
                                                        "pipe EOF enters drain-and-exit without initiating immediate shutdown");
                                ++state.connectRequestsFromEof;
                                socketClient.connect(path, [&state](const net::un::SocketAddress&, core::socket::State connectState) {
                                    if (connectState == core::socket::State::OK) {
                                        ++state.clientConnectOkCount;
                                    } else {
                                        state.fail("production Unix frontend client failed to connect after pipe EOF");
                                    }
                                });
                            },
                            [&state, &lifecycle](std::string message) {
                                lifecycle.inputFailed(message);
                                state.fail("production stdin reader failed during pipe acceptance: " + message);
                            },
                            stdinPipe.readFd);
                        stdinFlagsWhileActive = ::fcntl(stdinPipe.readFd, F_GETFL);
                    } catch (const std::exception& exception) {
                        state.fail("production stdin reader could not observe the deterministic pipe: " + std::string(exception.what()));
                    }
                });

            [[maybe_unused]] core::timer::Timer watchdog = core::timer::Timer::singleshotTimer(
                [&timedOut]() {
                    timedOut = true;
                    core::SNodeC::stop();
                },
                utils::Timeval({5, 0}));
            eventLoopResult = core::SNodeC::start(utils::Timeval({7, 0}));
            connectionClosed = !connection.connected();
            stdinFlagsAfterEventLoop = ::fcntl(stdinPipe.readFd, F_GETFL);
            stdinReader.reset();
            stdinFlagsAfterReaderDestruction = ::fcntl(stdinPipe.readFd, F_GETFL);
            state.connection = nullptr;
            connectionHandle = nullptr;
        }

        const auto expectedOutput = jsonOutput(
            {frontend::ServerMessage{frontend::Welcome{"acceptance-session",
                                                       frontend::SessionRole::Observer,
                                                       HandshakeSequence,
                                                       frontend::SyncMode::Snapshot,
                                                       frontend::Json::object()}},
             frontend::ServerMessage{frontend::Snapshot{HandshakeSequence, state.expectedSnapshot, frontend::Json::object()}},
             frontend::ServerMessage{frontend::SyncComplete{HandshakeSequence, frontend::Json::object()}},
             frontend::ServerMessage{frontend::Response::success(state.acquireRequestId, frontend::Json{{"role", "controller"}})},
             frontend::ServerMessage{
                 frontend::Response::success(state.threadsRequestId, frontend::Json{{"threads", frontend::Json::array()}})},
             frontend::ServerMessage{frontend::EventBatch{
                 frontend::SequenceNumber{8},
                 frontend::SequenceNumber{8},
                 {frontend::FrontendEvent{
                     frontend::SequenceNumber{8}, "acceptance.trailing", frontend::Json{{"presented", true}}, frontend::Json::object()}},
                 frontend::Json::object()}}});

        const bool pathCleaned = !pathExists(path);
        result.expectTrue(!timedOut && state.completed, "client pipe acceptance flow completes before its watchdog");
        result.expectEqual(0, eventLoopResult, "client pipe acceptance event loop stops cleanly");
        result.expectEqual(1, static_cast<int>(state.serverListenOkCount), "fake frontend server listens exactly once");
        result.expectEqual(2, static_cast<int>(state.stdinLineCount), "production stdin reader emits both piped commands exactly once");
        result.expectEqual(1, static_cast<int>(state.stdinEofCount), "production stdin reader emits EOF exactly once");
        result.expectEqual(
            1, static_cast<int>(state.connectRequestsFromEof), "the sole Unix connection attempt is initiated by the EOF callback");
        result.expectEqual(1, static_cast<int>(state.clientConnectOkCount), "production frontend client connects exactly once");
        result.expectEqual(1, static_cast<int>(state.serverConnectedCount), "fake server context connects exactly once");
        result.expectEqual(1, static_cast<int>(state.clientContextConnectedCount), "production client context connects exactly once");
        result.expectEqual(1, static_cast<int>(state.helloCount), "production client automatically sends hello exactly once");
        result.expectEqual(2, static_cast<int>(state.fragmentedWrites), "fake server deliberately splits welcome across two writes");
        result.expectEqual(3,
                           static_cast<int>(state.coalescedHandshakeMessages),
                           "the second fake-server write coalesces welcome completion, snapshot, and sync.complete");
        result.expectEqual(1, static_cast<int>(state.welcomeCount), "production client presents one welcome");
        result.expectEqual(1, static_cast<int>(state.snapshotCount), "production client presents one initial snapshot");
        result.expectEqual(1, static_cast<int>(state.syncCompleteCount), "production client presents one initial sync.complete");
        result.expectEqual(2, static_cast<int>(state.commandCount), "fake server receives both drained commands");
        result.expectEqual(1, static_cast<int>(state.acquireCount), "fake server receives controller.acquire exactly once");
        result.expectEqual(1, static_cast<int>(state.threadsCount), "fake server receives thread.list exactly once");
        result.expectTrue(state.serverRequestOrder == std::vector<std::string>{state.acquireRequestId, state.threadsRequestId},
                          "queued command request IDs reach the server exactly once and in input order");
        result.expectEqual(2, static_cast<int>(state.responseCount), "production client presents both correlated responses");
        result.expectEqual(
            3, static_cast<int>(state.coalescedDrainMessages), "fake server coalesces both responses and one trailing protocol frame");
        result.expectEqual(
            1, static_cast<int>(state.trailingEventBatchCount), "production client presents the frame decoded after the final response");
        result.expectEqual(1, static_cast<int>(state.lifecycleExitRequests), "completed drain requests one controlled exit");
        result.expectEqual(1, static_cast<int>(state.controlledDisconnects), "completed drain performs one next-tick disconnect");
        result.expectEqual(1, static_cast<int>(state.clientDisconnectedCount), "production client disconnect callback runs once");
        result.expectEqual(1, static_cast<int>(state.serverDisconnectedCount), "fake server disconnect callback runs once");
        result.expectEqual(0, static_cast<int>(state.failureCount), "client pipe acceptance flow reports no failures");
        result.expectTrue(state.lifecycleFailures.empty(), "production drain lifecycle reports no failure");
        result.expectTrue(state.lifecycleDrainedCleanly,
                          "production lifecycle reaches closed success with no queued, response, or sync work remaining");
        result.expectTrue(expectedOutput.has_value() && protocolOutput.str() == *expectedOutput,
                          "JSON presentation contains the handshake, both responses, and trailing decoded output in exact order");
        result.expectTrue(diagnostics.str() == "connected to " + path + "\ndisconnected\n",
                          "JSON mode keeps connection lifecycle diagnostics out of protocol output");
        result.expectTrue(stdinFlagsWhileActive == (stdinOriginalFlags | O_NONBLOCK),
                          "production stdin reader makes the pipe nonblocking only while active");
        result.expectTrue(state.pipeFlagsRestoredAtEof && stdinFlagsAfterEventLoop == stdinOriginalFlags &&
                              stdinFlagsAfterReaderDestruction == stdinOriginalFlags,
                          "pipe read descriptor flags are restored at EOF and remain restored through shutdown");
        result.expectTrue(connectionClosed, "ClientConnection reports disconnected after the drained close");
        result.expectTrue(pathCleaned, "fake Unix server removes its owned socket path on destruction");

        core::SNodeC::free();
        returnCode = result.processResult();
    }

    std::remove(path.c_str());
    return returnCode;
}
