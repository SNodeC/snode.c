/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include "CodexBackendTestSupport.h"
#include "ai/openai/codex/backend/BackendCore.h"
#include "ai/openai/codex/frontend/Codec.h"
#include "ai/openai/codex/frontend/Messages.h"
#include "apps/codex-backend-client/ClientConnection.h"
#include "apps/codex-backend-client/CodexBackendClientSocketContextFactory.h"
#include "apps/codex-backend-client/CommandDrainController.h"
#include "apps/codex-backend-client/CommandParser.h"
#include "apps/codex-backend-client/Presenter.h"
#include "apps/codex-backend-client/StdinReader.h"
#include "apps/codex-backend/CodexFrontendSocketContextFactory.h"
#include "apps/codex-backend/Configuration.h"
#include "core/EventReceiver.h"
#include "core/SNodeC.h"
#include "core/socket/State.h"
#include "core/timer/Timer.h"
#include "net/un/SocketAddress.h"
#include "net/un/stream/legacy/SocketClient.h"
#include "net/un/stream/legacy/SocketServer.h"
#include "support/TestResult.h"
#include "utils/Timeval.h"

#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <functional>
#include <memory>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <unistd.h>
#include <utility>
#include <variant>
#include <vector>

namespace {
    namespace backend = ai::openai::codex::backend;
    namespace client = apps::codex_backend_client;
    namespace frontend = ai::openai::codex::frontend;

    using FakeBackendCore = backend::BackendCore<tests::codex::FakeAppServerClient>;
    using ai::openai::codex::Json;
    using ai::openai::codex::detail::TransportCallbacks;

    class Pipe {
    public:
        Pipe() {
            int descriptors[2] = {-1, -1};
            if (::pipe(descriptors) == 0) {
                readFd = descriptors[0];
                writeFd = descriptors[1];
            }
        }

        Pipe(const Pipe&) = delete;
        Pipe& operator=(const Pipe&) = delete;

        ~Pipe() {
            if (readFd >= 0) {
                ::close(readFd);
            }
            if (writeFd >= 0) {
                ::close(writeFd);
            }
        }

        [[nodiscard]] bool valid() const noexcept {
            return readFd >= 0 && writeFd >= 0;
        }

        [[nodiscard]] bool writeAll(std::string_view bytes) const noexcept {
            std::size_t offset = 0;
            while (offset < bytes.size()) {
                const ssize_t count = ::write(writeFd, bytes.data() + offset, bytes.size() - offset);
                if (count > 0) {
                    offset += static_cast<std::size_t>(count);
                    continue;
                }
                if (count < 0 && errno == EINTR) {
                    continue;
                }
                return false;
            }
            return true;
        }

        int readFd = -1;

    private:
        int writeFd = -1;
    };

    std::string socketPath() {
        return "/tmp/snodec-codex-client-real-backend-" + std::to_string(::getpid()) + ".sock";
    }

    bool pathExists(const std::string& path) {
        return ::access(path.c_str(), F_OK) == 0;
    }

    bool contains(std::string_view haystack, std::string_view needle) {
        return haystack.find(needle) != std::string_view::npos;
    }

} // namespace

int main(int argc, char* argv[]) {
    tests::support::TestResult result;
    int returnCode = tests::support::cTestSkipReturnCode;
    const std::string path = socketPath();

    if (tests::support::shouldSkipRootWithoutSNodeCGroup()) {
        tests::support::printRootWithoutSNodeCGroupSkipMessage("CodexBackendClientRealBackendAcceptanceTest");
    } else {
        std::remove(path.c_str());
        result.expectTrue(!pathExists(path), "the unique real-backend client socket path is absent before listen");
        core::SNodeC::init(argc, argv);

        bool timedOut = false;
        int eventLoopResult = 1;
        bool backendReadyForHandshake = false;
        bool pathCleanedByServer = false;
        std::size_t appServerThreadListCount = 0;
        std::size_t listenSuccessCount = 0;
        std::size_t connectSuccessCount = 0;
        std::size_t connectionCallbackCount = 0;
        std::size_t disconnectCallbackCount = 0;
        std::size_t protocolErrorCount = 0;
        std::size_t welcomeCount = 0;
        std::size_t snapshotCount = 0;
        std::size_t syncCompleteCount = 0;
        std::size_t acquireResponseCount = 0;
        std::size_t threadsResponseCount = 0;
        std::size_t invalidCommandCount = 0;
        std::size_t stdinEofCount = 0;
        std::size_t quitCount = 0;
        std::size_t exitRequestCount = 0;
        std::size_t backendSessionCountWhileInteractive = 0;
        bool acquireQueuedBeforeConnection = false;
        bool syncObservedWhileSynchronizing = false;
        bool reachedReady = false;
        bool queuedAcquireSentAfterSync = false;
        bool postReadyCommandSentImmediately = false;
        bool invalidReportedBeforeThreadsResponse = false;
        bool interactiveStayedConnected = false;
        bool acquireResponseSucceeded = false;
        bool threadsResponseSucceeded = false;
        bool completionScheduled = false;
        bool eventLoopRunning = false;
        bool exitScheduled = false;
        std::string acquireRequestId;
        std::string threadsRequestId;
        std::vector<std::string> initialMessageKinds;
        std::vector<std::string> sentCommandIds;
        std::vector<client::CommandDrainController::SessionState> sentCommandStates;
        std::vector<std::size_t> sentCommandSyncCounts;
        std::ostringstream humanOutput;
        std::ostringstream diagnostics;

        {
            const auto transport = std::make_shared<tests::codex::FakeTransportState>();
            tests::codex::installInitializingFake(
                transport, [&appServerThreadListCount](const Json& message, const TransportCallbacks& callbacks) {
                    if (message.value("method", std::string{}) != "thread/list") {
                        return;
                    }
                    const auto id = message.find("id");
                    if (id == message.end()) {
                        return;
                    }
                    ++appServerThreadListCount;
                    tests::codex::inject(
                        callbacks,
                        Json{{"id", *id}, {"result", {{"data", Json::array()}, {"nextCursor", nullptr}, {"backwardsCursor", nullptr}}}});
                });

            backend::BackendCoreOptions backendOptions;
            backendOptions.initialThreadListLimit = 2;
            FakeBackendCore backendCore(backendOptions, transport);

            apps::codex_backend::SocketFrontendOptions frontendOptions;
            frontendOptions.maximumOutboundBytes = 1024U * 1024U;
            using Server = net::un::stream::legacy::SocketServer<apps::codex_backend::CodexFrontendSocketContextFactory,
                                                                 FakeBackendCore&,
                                                                 apps::codex_backend::SocketFrontendOptions>;
            Server server("codex-client-real-backend-server", backendCore, std::move(frontendOptions));
            server.getConfig()->Instance::forceUnrequired();

            backendCore.start();

            Pipe inputPipe;
            result.expectTrue(inputPipe.valid() && inputPipe.writeAll("acquire\n"),
                              "an open deterministic input pipe contains the preconnection acquire command");

            client::Presenter presenter(client::OutputMode::Human, humanOutput, diagnostics);
            client::CommandParser parser("interactive");
            client::ClientConnection* connectionHandle = nullptr;
            client::StdinReader* stdinReaderHandle = nullptr;
            client::CommandDrainController* lifecycleHandle = nullptr;

            client::CommandDrainController lifecycle(client::CommandDrainCallbacks{
                .send =
                    [&](const frontend::ClientMessage& message) {
                        if (const auto* command = std::get_if<frontend::Command>(&message)) {
                            sentCommandIds.push_back(command->requestId);
                            sentCommandStates.push_back(lifecycleHandle != nullptr
                                                            ? lifecycleHandle->sessionState()
                                                            : client::CommandDrainController::SessionState::Connecting);
                            sentCommandSyncCounts.push_back(syncCompleteCount);
                        }
                        return connectionHandle != nullptr && connectionHandle->send(message);
                    },
                .requestExit =
                    [&connectionHandle, &stdinReaderHandle, &eventLoopRunning, &exitScheduled, &exitRequestCount]() {
                        ++exitRequestCount;
                        if (stdinReaderHandle != nullptr) {
                            stdinReaderHandle->stop();
                        }
                        if (!eventLoopRunning || exitScheduled) {
                            return;
                        }
                        exitScheduled = true;
                        core::EventReceiver::atNextTick([&connectionHandle, &eventLoopRunning]() {
                            if (!eventLoopRunning) {
                                return;
                            }
                            if (connectionHandle != nullptr && connectionHandle->connected()) {
                                connectionHandle->disconnect();
                            } else {
                                core::SNodeC::stop();
                            }
                        });
                    },
                .reportFailure =
                    [&presenter](std::string message) {
                        presenter.error(message);
                    }});
            lifecycleHandle = &lifecycle;

            std::function<void()> maybeFinish;
            client::ClientConnection connection(client::ClientConnectionCallbacks{
                .onConnected =
                    [&]() {
                        ++connectionCallbackCount;
                        result.expectTrue(lifecycle.sessionState() == client::CommandDrainController::SessionState::Connecting,
                                          "production connection callback runs while the lifecycle is Connecting");
                        lifecycle.connected();
                        presenter.connected(path);
                        presenter.localMessage("enter 'help' for commands");
                        result.expectTrue(lifecycle.sessionState() == client::CommandDrainController::SessionState::Synchronizing,
                                          "production connection callback establishes Synchronizing before hello is accepted");
                    },
                .onMessage =
                    [&](const frontend::ServerMessage& message) {
                        presenter.present(message);

                        const bool initialSync = std::holds_alternative<frontend::SyncComplete>(message) && syncCompleteCount == 0;
                        if (syncCompleteCount == 0) {
                            if (std::holds_alternative<frontend::Welcome>(message)) {
                                initialMessageKinds.emplace_back("welcome");
                            } else if (std::holds_alternative<frontend::Snapshot>(message)) {
                                initialMessageKinds.emplace_back("snapshot");
                            } else if (std::holds_alternative<frontend::EventBatch>(message)) {
                                initialMessageKinds.emplace_back("events");
                            } else if (std::holds_alternative<frontend::SyncComplete>(message)) {
                                initialMessageKinds.emplace_back("sync.complete");
                            } else if (std::holds_alternative<frontend::Response>(message)) {
                                initialMessageKinds.emplace_back("response");
                            } else {
                                initialMessageKinds.emplace_back("protocol.error");
                            }
                        }
                        if (std::holds_alternative<frontend::Welcome>(message)) {
                            ++welcomeCount;
                            backendReadyForHandshake = backendCore.snapshot().lifecycle == backend::BackendLifecycle::Ready;
                        } else if (std::holds_alternative<frontend::Snapshot>(message)) {
                            ++snapshotCount;
                        } else if (initialSync) {
                            ++syncCompleteCount;
                            syncObservedWhileSynchronizing =
                                lifecycle.sessionState() == client::CommandDrainController::SessionState::Synchronizing &&
                                lifecycle.queuedCount() == 1;
                        } else if (const auto* response = std::get_if<frontend::Response>(&message)) {
                            if (response->requestId == acquireRequestId) {
                                ++acquireResponseCount;
                                acquireResponseSucceeded = acquireResponseSucceeded || response->ok;
                            } else if (response->requestId == threadsRequestId) {
                                ++threadsResponseCount;
                                threadsResponseSucceeded = threadsResponseSucceeded || response->ok;
                            }
                        } else if (std::holds_alternative<frontend::ProtocolErrorMessage>(message)) {
                            ++protocolErrorCount;
                        }

                        lifecycle.receive(message);

                        if (initialSync) {
                            reachedReady = lifecycle.sessionState() == client::CommandDrainController::SessionState::Ready;
                            queuedAcquireSentAfterSync =
                                reachedReady && lifecycle.queuedCount() == 0 && lifecycle.pendingResponseCount() == 1;
                            presenter.localMessage("synchronized; commands are ready");
                            if (!inputPipe.writeAll("threads\nfoobar\n")) {
                                lifecycle.inputFailed("failed to write deterministic interactive commands");
                            }
                        }
                        if (maybeFinish) {
                            maybeFinish();
                        }
                    },
                .onProtocolError =
                    [&](const frontend::CodecError& error) {
                        ++protocolErrorCount;
                        lifecycle.protocolFailed(std::string(frontend::toString(error.code)) + ": " + error.message);
                    },
                .onDisconnected =
                    [&]() {
                        ++disconnectCallbackCount;
                        presenter.disconnected();
                        lifecycle.disconnected();
                        if (eventLoopRunning) {
                            core::SNodeC::stop();
                        }
                    }});
            connectionHandle = &connection;

            using Client = net::un::stream::legacy::SocketClient<client::CodexBackendClientSocketContextFactory, client::ClientConnection&>;
            Client socketClient("codex-client-real-backend-client", connection);
            socketClient.getConfig()->Instance::forceUnrequired();

            std::unique_ptr<client::StdinReader> stdinReader;
            std::function<void(std::string)> handleInput;
            handleInput = [&](std::string line) {
                client::ParsedCommand parsed = parser.parse(line);
                std::visit(
                    [&]<typename Parsed>(Parsed&& command) {
                        using T = std::remove_cvref_t<Parsed>;
                        if constexpr (std::is_same_v<T, client::SendCommand>) {
                            auto* wireCommand = std::get_if<frontend::Command>(&command.message);
                            if (wireCommand == nullptr) {
                                result.expectTrue(false, "interactive parser produces a typed frontend Command");
                                return;
                            }
                            const bool waiting = lifecycle.sessionState() == client::CommandDrainController::SessionState::Connecting ||
                                                 lifecycle.sessionState() == client::CommandDrainController::SessionState::Synchronizing;
                            const bool acquire = std::holds_alternative<frontend::ControllerAcquire>(wireCommand->parameters);
                            const bool threads = std::holds_alternative<frontend::ThreadList>(wireCommand->parameters);
                            if (acquire) {
                                acquireRequestId = wireCommand->requestId;
                            } else if (threads) {
                                threadsRequestId = wireCommand->requestId;
                            }
                            const bool accepted = lifecycle.enqueue(std::move(command.message));
                            result.expectTrue(accepted, "interactive lifecycle accepts each valid parsed command");
                            if (accepted && waiting) {
                                presenter.localMessage("command queued; waiting for initial synchronization");
                            }
                            if (acquire) {
                                acquireQueuedBeforeConnection =
                                    accepted && waiting && !connection.connected() && lifecycle.queuedCount() == 1;
                                socketClient.connect(path, [&](const net::un::SocketAddress&, core::socket::State state) {
                                    if (state == core::socket::State::OK) {
                                        ++connectSuccessCount;
                                    } else {
                                        lifecycle.connectionFailed("real-backend production client failed to connect");
                                    }
                                });
                            } else if (threads) {
                                postReadyCommandSentImmediately =
                                    accepted && !waiting &&
                                    lifecycle.sessionState() == client::CommandDrainController::SessionState::Ready &&
                                    lifecycle.queuedCount() == 0;
                            } else {
                                result.expectTrue(false, "only acquire and threads are sent in the interactive acceptance scenario");
                            }
                        } else if constexpr (std::is_same_v<T, client::QuitCommand>) {
                            ++quitCount;
                            lifecycle.quit();
                        } else if constexpr (std::is_same_v<T, client::CommandParseError>) {
                            ++invalidCommandCount;
                            invalidReportedBeforeThreadsResponse = threadsResponseCount == 0;
                            presenter.error(command.message);
                            if (maybeFinish) {
                                maybeFinish();
                            }
                        } else if constexpr (!std::is_same_v<T, client::NoopCommand>) {
                            result.expectTrue(false, "interactive acceptance input contains no unexpected local command");
                        }
                    },
                    std::move(parsed));
            };

            maybeFinish = [&]() {
                if (completionScheduled || acquireResponseCount != 1 || threadsResponseCount != 1 || !acquireResponseSucceeded ||
                    !threadsResponseSucceeded || invalidCommandCount != 1 || lifecycle.pendingResponseCount() != 0) {
                    return;
                }
                completionScheduled = true;
                core::EventReceiver::atNextTick([&]() {
                    interactiveStayedConnected = connection.connected() &&
                                                 lifecycle.sessionState() == client::CommandDrainController::SessionState::Ready &&
                                                 lifecycle.inputState() == client::CommandDrainController::InputState::Reading &&
                                                 lifecycle.outcome() == client::CommandDrainController::Outcome::Running;
                    backendSessionCountWhileInteractive = backendCore.snapshot().sessions.size();
                    if (!inputPipe.writeAll("quit\n")) {
                        lifecycle.inputFailed("failed to write deterministic interactive quit");
                    }
                });
            };

            server.listen(path, [&](const net::un::SocketAddress&, core::socket::State state) {
                if (state != core::socket::State::OK) {
                    lifecycle.connectionFailed("real BackendAdapter Unix server failed to listen");
                    return;
                }
                ++listenSuccessCount;
                try {
                    stdinReader = std::make_unique<client::StdinReader>(
                        handleInput,
                        [&]() {
                            ++stdinEofCount;
                            lifecycle.inputEof();
                        },
                        [&](std::string message) {
                            lifecycle.inputFailed(std::move(message));
                        },
                        inputPipe.readFd);
                    stdinReaderHandle = stdinReader.get();
                } catch (const std::exception& exception) {
                    lifecycle.inputFailed("failed to construct interactive stdin reader: " + std::string(exception.what()));
                }
            });

            [[maybe_unused]] core::timer::Timer watchdog = core::timer::Timer::singleshotTimer(
                [&]() {
                    timedOut = true;
                    core::SNodeC::stop();
                },
                utils::Timeval({10, 0}));
            eventLoopRunning = true;
            eventLoopResult = core::SNodeC::start(utils::Timeval({12, 0}));
            eventLoopRunning = false;
            if (stdinReader != nullptr) {
                stdinReader->stop();
            }
            connection.disconnect();
            backendCore.stop();
        }

        pathCleanedByServer = !pathExists(path);
        const std::string output = humanOutput.str();
        const std::string errors = diagnostics.str();
        result.expectTrue(backendReadyForHandshake, "fake AppServerClient brings the real BackendCore to Ready for the frontend handshake");
        result.expectTrue(!timedOut, "interactive real-backend acceptance completes before its safety watchdog");
        result.expectEqual(0, eventLoopResult, "interactive production client event loop stops cleanly");
        result.expectEqual(1, static_cast<int>(listenSuccessCount), "real frontend adapter Unix server listens exactly once");
        result.expectEqual(1, static_cast<int>(connectSuccessCount), "production Unix client connects exactly once");
        result.expectEqual(1, static_cast<int>(connectionCallbackCount), "production client connection callback runs exactly once");
        result.expectTrue(acquireQueuedBeforeConnection, "acquire entered before readiness is retained behind the synchronization barrier");
        result.expectEqual(1, static_cast<int>(welcomeCount), "exactly one automatic hello produces exactly one real-adapter welcome");
        result.expectEqual(1, static_cast<int>(snapshotCount), "real BackendAdapter emits one initial snapshot");
        result.expectEqual(1, static_cast<int>(syncCompleteCount), "real BackendAdapter emits one initial sync.complete");
        result.expectTrue(initialMessageKinds == std::vector<std::string>{"welcome", "snapshot", "sync.complete"},
                          "real adapter handshake is decoded in welcome, snapshot, sync.complete order");
        result.expectTrue(syncObservedWhileSynchronizing && reachedReady,
                          "production lifecycle sees initial sync.complete in Synchronizing and transitions to Ready");
        result.expectTrue(queuedAcquireSentAfterSync, "the pre-ready acquire is sent exactly once only after sync.complete");
        result.expectTrue(postReadyCommandSentImmediately, "threads entered in Ready is sent immediately without entering the queue");
        result.expectTrue(sentCommandIds == std::vector<std::string>{acquireRequestId, threadsRequestId},
                          "queued acquire and ready-state threads are sent exactly once and in input order");
        result.expectTrue(sentCommandStates ==
                              std::vector<client::CommandDrainController::SessionState>{
                                  client::CommandDrainController::SessionState::Ready, client::CommandDrainController::SessionState::Ready},
                          "both ordinary commands cross the Unix connection only after the lifecycle is Ready");
        result.expectTrue(sentCommandSyncCounts == std::vector<std::size_t>{1, 1},
                          "both ordinary commands are sent after the initial sync.complete is decoded");
        result.expectTrue(acquireResponseCount == 1 && acquireResponseSucceeded,
                          "acquire receives exactly one correlated successful response");
        result.expectTrue(threadsResponseCount == 1 && threadsResponseSucceeded,
                          "threads receives exactly one correlated successful response");
        result.expectEqual(
            2, static_cast<int>(appServerThreadListCount), "fake AppServerClient sees one initial and one interactive thread/list request");
        result.expectEqual(0, static_cast<int>(protocolErrorCount), "the real adapter reports no handshake or command protocol error");
        result.expectEqual(1, static_cast<int>(invalidCommandCount), "invalid interactive input reports one immediate local parser error");
        result.expectTrue(invalidReportedBeforeThreadsResponse, "the invalid local command error is emitted before the backend response");
        result.expectEqual(0, static_cast<int>(stdinEofCount), "the open interactive pipe never enters EOF draining");
        result.expectTrue(interactiveStayedConnected, "interactive mode remains connected and Running after both command responses");
        result.expectEqual(1,
                           static_cast<int>(backendSessionCountWhileInteractive),
                           "the real adapter owns one frontend session, consistent with exactly one accepted hello");
        result.expectEqual(1, static_cast<int>(quitCount), "interactive shutdown is initiated by one parsed quit command");
        result.expectEqual(1, static_cast<int>(exitRequestCount), "explicit interactive quit requests one controlled exit");
        result.expectEqual(1, static_cast<int>(disconnectCallbackCount), "production client disconnect callback runs exactly once");
        result.expectTrue(contains(output, "connected to " + path + "; waiting for initial synchronization") &&
                              contains(output, "command queued; waiting for initial synchronization") &&
                              contains(output, "synchronized; commands are ready"),
                          "human mode explains connection, queued-command, and synchronization readiness states");
        result.expectTrue(contains(output, "welcome session=") && contains(output, "snapshot sequence=") &&
                              contains(output, "sync.complete sequence=") &&
                              contains(output, "response request-id=" + acquireRequestId + " ok=true") &&
                              contains(output, "response request-id=" + threadsRequestId + " ok=true"),
                          "human presentation includes the real handshake and both correlated responses");
        result.expectTrue(contains(errors, "unknown command 'foobar'"), "invalid input remains an immediate diagnostic");
        result.expectTrue(pathCleanedByServer, "real frontend Unix server removes its owned socket path");

        core::SNodeC::free();
        returnCode = result.processResult();
    }

    return returnCode;
}
