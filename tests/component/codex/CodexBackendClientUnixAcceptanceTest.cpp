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
#include "apps/codex-backend-client/CommandParser.h"
#include "apps/codex-backend-client/JsonLineFramer.h"
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
#include <cstdio>
#include <optional>
#include <string>
#include <string_view>
#include <unistd.h>
#include <utility>
#include <variant>

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

        void clientConnected() {
            ++clientContextConnectedCount;
        }

        void clientDisconnected() {
            ++clientDisconnectedCount;
            stopWhenDisconnected();
        }

        void clientProtocolError(const frontend::CodecError& error) {
            fail("production client rejected deterministic server protocol: " + error.message);
        }

        void clientMessage(const frontend::ServerMessage& message) {
            if (const auto* welcome = std::get_if<frontend::Welcome>(&message)) {
                ++welcomeCount;
                result.expectTrue(welcome->sessionId == "acceptance-session" && welcome->role == frontend::SessionRole::Observer &&
                                      welcome->currentSequence == HandshakeSequence && welcome->syncMode == frontend::SyncMode::Snapshot,
                                  "production client decodes the fragmented welcome exactly");
                return;
            }

            if (const auto* snapshot = std::get_if<frontend::Snapshot>(&message)) {
                ++snapshotCount;
                result.expectTrue(snapshot->sequence == HandshakeSequence && snapshot->state == expectedSnapshot,
                                  "production client decodes the coalesced snapshot exactly");
                return;
            }

            if (const auto* complete = std::get_if<frontend::SyncComplete>(&message)) {
                ++syncCompleteCount;
                result.expectTrue(complete->sequence == HandshakeSequence,
                                  "production client decodes sync.complete after the coalesced snapshot");
                if (connection == nullptr || acquireSent) {
                    fail("sync.complete must submit exactly one controller acquisition through ClientConnection");
                    return;
                }

                const client::ParsedCommand parsed = parser.parse("acquire");
                const auto* send = std::get_if<client::SendCommand>(&parsed);
                const auto* command = send == nullptr ? nullptr : std::get_if<frontend::Command>(&send->message);
                const bool typedAcquire = command != nullptr && std::holds_alternative<frontend::ControllerAcquire>(command->parameters);
                result.expectTrue(typedAcquire, "acceptance flow uses the production parser for controller.acquire");
                if (!typedAcquire) {
                    fail("production parser did not create controller.acquire during acceptance flow");
                    return;
                }

                acquireRequestId = command->requestId;
                acquireSent = connection->send(send->message);
                result.expectTrue(acquireSent, "production ClientConnection accepts controller.acquire after synchronization");
                if (!acquireSent) {
                    fail("production ClientConnection could not send controller.acquire");
                }
                return;
            }

            if (const auto* response = std::get_if<frontend::Response>(&message)) {
                ++responseCount;
                const bool correlated = acquireSent && response->requestId == acquireRequestId && response->ok &&
                                        response->result == std::optional<frontend::Json>{frontend::Json{{"role", "controller"}}};
                result.expectTrue(correlated, "production client delivers the correlated controller acquisition response exactly");
                if (!correlated || connection == nullptr) {
                    fail("controller acquisition response was not correlated");
                    return;
                }
                ++clientDisconnectRequests;
                connection->disconnect();
                return;
            }

            fail("production client received an unexpected deterministic server message");
        }

        tests::support::TestResult& result;
        client::CommandParser parser;
        client::ClientConnection* connection = nullptr;
        frontend::Json expectedSnapshot{{"lifecycle", "ready"}, {"threads", frontend::Json::array()}};
        std::string acquireRequestId;
        bool acquireSent = false;
        bool completed = false;
        std::size_t failureCount = 0;
        std::size_t serverListenOkCount = 0;
        std::size_t clientConnectOkCount = 0;
        std::size_t serverConnectedCount = 0;
        std::size_t serverDisconnectedCount = 0;
        std::size_t clientContextConnectedCount = 0;
        std::size_t clientDisconnectedCount = 0;
        std::size_t helloCount = 0;
        std::size_t acquireCount = 0;
        std::size_t welcomeCount = 0;
        std::size_t snapshotCount = 0;
        std::size_t syncCompleteCount = 0;
        std::size_t responseCount = 0;
        std::size_t fragmentedWrites = 0;
        std::size_t coalescedHandshakeMessages = 0;
        std::size_t clientDisconnectRequests = 0;
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
                state.result.expectTrue(state.helloCount == 1 && !hello->resumeAfter.has_value(),
                                        "production socket context automatically sends one typed hello without a replay position");
                sendHandshake();
                return;
            }

            const auto* command = std::get_if<frontend::Command>(&decoded.value());
            const bool acquire = command != nullptr && std::holds_alternative<frontend::ControllerAcquire>(command->parameters);
            state.result.expectTrue(state.helloCount == 1 && acquire,
                                    "fake frontend server receives controller.acquire only after the automatic hello");
            if (!acquire) {
                state.fail("fake frontend server received an unexpected client command");
                close();
                return;
            }

            ++state.acquireCount;
            const auto response =
                frame(frontend::ServerMessage{frontend::Response::success(command->requestId, frontend::Json{{"role", "controller"}})});
            if (response) {
                sendToPeer(response->data(), response->size());
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

        ScenarioState state(result);
        bool timedOut = false;
        int eventLoopResult = 1;
        bool connectionClosed = false;
        {
            client::ClientConnection connection(client::ClientConnectionCallbacks{.onConnected =
                                                                                      [&state]() {
                                                                                          state.clientConnected();
                                                                                      },
                                                                                  .onMessage =
                                                                                      [&state](const frontend::ServerMessage& message) {
                                                                                          state.clientMessage(message);
                                                                                      },
                                                                                  .onProtocolError =
                                                                                      [&state](const frontend::CodecError& error) {
                                                                                          state.clientProtocolError(error);
                                                                                      },
                                                                                  .onDisconnected =
                                                                                      [&state]() {
                                                                                          state.clientDisconnected();
                                                                                      }});
            state.connection = &connection;

            using Server = net::un::stream::legacy::SocketServer<FakeFrontendServerFactory, ScenarioState&>;
            using Client = net::un::stream::legacy::
                SocketClient<client::CodexBackendClientSocketContextFactory, client::ClientConnection&, std::size_t>;

            Server server("codex-backend-client-acceptance-server", state);
            Client socketClient("codex-backend-client-acceptance-client", connection, std::size_t{TestMaximumFrameSize});
            server.getConfig()->Instance::forceUnrequired();
            socketClient.getConfig()->Instance::forceUnrequired();

            server.listen(path, [&state, &socketClient, &path](const net::un::SocketAddress&, core::socket::State listenState) {
                if (listenState != core::socket::State::OK) {
                    state.fail("deterministic fake Unix frontend server failed to listen");
                    return;
                }
                ++state.serverListenOkCount;
                socketClient.connect(path, [&state](const net::un::SocketAddress&, core::socket::State connectState) {
                    if (connectState == core::socket::State::OK) {
                        ++state.clientConnectOkCount;
                    } else {
                        state.fail("production Unix frontend client failed to connect");
                    }
                });
            });

            [[maybe_unused]] core::timer::Timer watchdog = core::timer::Timer::singleshotTimer(
                [&timedOut]() {
                    timedOut = true;
                    core::SNodeC::stop();
                },
                utils::Timeval({5, 0}));
            eventLoopResult = core::SNodeC::start(utils::Timeval({7, 0}));
            connectionClosed = !connection.connected();
            state.connection = nullptr;
        }

        const bool pathCleaned = !pathExists(path);
        result.expectTrue(!timedOut && state.completed, "client acceptance flow completes before its watchdog");
        result.expectEqual(0, eventLoopResult, "client acceptance event loop stops cleanly");
        result.expectEqual(1, static_cast<int>(state.serverListenOkCount), "fake frontend server listens exactly once");
        result.expectEqual(1, static_cast<int>(state.clientConnectOkCount), "production frontend client connects exactly once");
        result.expectEqual(1, static_cast<int>(state.serverConnectedCount), "fake server context connects exactly once");
        result.expectEqual(1, static_cast<int>(state.clientContextConnectedCount), "production client context connects exactly once");
        result.expectEqual(1, static_cast<int>(state.helloCount), "production client automatically sends hello exactly once");
        result.expectEqual(2, static_cast<int>(state.fragmentedWrites), "fake server deliberately splits welcome across two writes");
        result.expectEqual(3,
                           static_cast<int>(state.coalescedHandshakeMessages),
                           "the second fake-server write coalesces welcome completion, snapshot, and sync.complete");
        result.expectEqual(1, static_cast<int>(state.welcomeCount), "production client receives one welcome");
        result.expectEqual(1, static_cast<int>(state.snapshotCount), "production client receives one snapshot");
        result.expectEqual(1, static_cast<int>(state.syncCompleteCount), "production client receives one sync.complete");
        result.expectEqual(1, static_cast<int>(state.acquireCount), "fake server receives one controller acquisition");
        result.expectEqual(1, static_cast<int>(state.responseCount), "production client receives one correlated response");
        result.expectEqual(1, static_cast<int>(state.clientDisconnectRequests), "client requests one controlled disconnect");
        result.expectEqual(1, static_cast<int>(state.clientDisconnectedCount), "production client disconnect callback runs once");
        result.expectEqual(1, static_cast<int>(state.serverDisconnectedCount), "fake server disconnect callback runs once");
        result.expectEqual(0, static_cast<int>(state.failureCount), "client acceptance flow reports no protocol or socket failures");
        result.expectTrue(connectionClosed, "ClientConnection reports disconnected after controlled close");
        result.expectTrue(pathCleaned, "fake Unix server removes its owned socket path on destruction");

        core::SNodeC::free();
        returnCode = result.processResult();
    }

    std::remove(path.c_str());
    return returnCode;
}
