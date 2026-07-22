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

#include <algorithm>
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <limits>
#include <memory>
#include <optional>
#include <ostream>
#include <sstream>
#include <streambuf>
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

    constexpr std::size_t LargeThreadCount = 24;
    constexpr std::size_t LargePreviewBytes = 4096;
    constexpr std::size_t LargeProtocolThreshold = 64U * 1024U;
    constexpr std::size_t HumanPresentationWindowBytes = 8U * 1024U;

    std::string largeThreadId(std::size_t index) {
        return "large-thread-" + std::to_string(index);
    }

    std::string largePreview(std::size_t index) {
        std::string preview = "complete-preview-" + largeThreadId(index) + ':';
        preview.append(LargePreviewBytes - preview.size(), static_cast<char>('a' + index % 26));
        return preview;
    }

    Json largeThreadPage() {
        Json threads = Json::array();
        for (std::size_t index = 0; index < LargeThreadCount; ++index) {
            Json thread = tests::codex::threadValue(largeThreadId(index));
            thread["preview"] = largePreview(index);
            threads.push_back(std::move(thread));
        }
        return Json{{"data", std::move(threads)}, {"nextCursor", nullptr}, {"backwardsCursor", nullptr}};
    }

    bool isLargeSnapshotHydrated(const backend::Snapshot& snapshot) {
        if (snapshot.lifecycle != backend::BackendLifecycle::Ready || snapshot.threads.size() != LargeThreadCount) {
            return false;
        }
        for (std::size_t index = 0; index < LargeThreadCount; ++index) {
            const auto thread =
                std::find_if(snapshot.threads.begin(), snapshot.threads.end(), [&](const backend::ThreadSnapshot& candidate) {
                    return candidate.id == largeThreadId(index);
                });
            if (thread == snapshot.threads.end() || !thread->preview.has_value() || *thread->preview != largePreview(index)) {
                return false;
            }
        }
        return true;
    }

    class WindowedStreamBuffer final : public std::streambuf {
    public:
        explicit WindowedStreamBuffer(std::size_t windowBytes)
            : windowBytes(windowBytes) {
        }

        [[nodiscard]] const std::string& str() const noexcept {
            return bytes;
        }

        [[nodiscard]] std::size_t size() const noexcept {
            return bytes.size();
        }

        [[nodiscard]] std::size_t maximumBurstBytes() const noexcept {
            return maximumBurst;
        }

        [[nodiscard]] bool overflowed() const noexcept {
            return didOverflow;
        }

    protected:
        std::streamsize xsputn(const char* source, std::streamsize count) override {
            if (count <= 0) {
                return 0;
            }

            const std::size_t requested = static_cast<std::size_t>(count);
            if (requested > windowBytes - currentBurst) {
                didOverflow = true;
                return 0;
            }

            bytes.append(source, requested);
            currentBurst += requested;
            maximumBurst = std::max(maximumBurst, currentBurst);
            return count;
        }

        int_type overflow(int_type character) override {
            if (traits_type::eq_int_type(character, traits_type::eof())) {
                return traits_type::not_eof(character);
            }

            const char value = traits_type::to_char_type(character);
            return xsputn(&value, 1) == 1 ? character : traits_type::eof();
        }

        int sync() override {
            currentBurst = 0;
            return 0;
        }

    private:
        std::size_t windowBytes;
        std::size_t currentBurst = 0;
        std::size_t maximumBurst = 0;
        bool didOverflow = false;
        std::string bytes;
    };

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

    std::string socketPath() {
        return "/tmp/snodec-codex-client-real-backend-" + std::to_string(::getpid()) + ".sock";
    }

    bool pathExists(const std::string& path) {
        return ::access(path.c_str(), F_OK) == 0;
    }

    bool contains(std::string_view haystack, std::string_view needle) {
        return haystack.find(needle) != std::string_view::npos;
    }

    std::string threadWorkflowSocketPath() {
        return "/tmp/snodec-codex-client-thread-workflow-" + std::to_string(::getpid()) + ".sock";
    }

    void runThreadWorkflowAcceptance(tests::support::TestResult& result, int argc, char* argv[]) {
        const std::string path = threadWorkflowSocketPath();
        std::remove(path.c_str());
        result.expectTrue(!pathExists(path), "the thread-workflow Unix socket path is absent before listen");
        core::SNodeC::init(argc, argv);

        constexpr std::string_view PersistedThreadId = "thread-persisted-acceptance";
        constexpr std::string_view ExplicitThreadId = "thread-started-acceptance";
        constexpr std::string_view NewThreadId = "thread-new-acceptance";
        constexpr std::string_view ExplicitPrompt = "Continue the previous task.";
        constexpr std::string_view NewPrompt = "Review  the current implementation. --literal";

        bool timedOut = false;
        bool serverListening = false;
        bool backendHydratedBeforeInput = false;
        bool inputClosedBeforeConnection = false;
        bool eofEnteredDrain = false;
        bool newStartObservedDuringEofDrain = false;
        bool newTurnSentFromMatchingStart = false;
        bool exitWaitedForNewTurnResponse = false;
        bool lifecycleClosedCleanly = false;
        bool eventLoopRunning = false;
        bool exitScheduled = false;
        bool completed = false;
        int eventLoopResult = 1;
        std::size_t listenSuccessCount = 0;
        std::size_t connectSuccessCount = 0;
        std::size_t connectionCallbackCount = 0;
        std::size_t disconnectCallbackCount = 0;
        std::size_t stdinLineCount = 0;
        std::size_t stdinEofCount = 0;
        std::size_t syncCompleteCount = 0;
        std::size_t exitRequestCount = 0;
        std::size_t appServerThreadListCount = 0;
        std::size_t acquireResponseCount = 0;
        std::size_t protocolErrorCount = 0;
        std::size_t queuedCountAtEof = 0;
        std::string explicitStartRequestId;
        std::string resumeRequestId;
        std::string explicitTurnRequestId;
        std::string newStartRequestId;
        std::string newTurnRequestId;
        std::vector<std::string> acquireRequestIds;
        std::vector<std::string> sentCommandIds;
        std::vector<std::string> sentCommandMethods;
        std::vector<std::size_t> sentCommandSyncCounts;
        std::vector<std::string> responseIds;
        std::vector<std::pair<std::string, Json>> appServerOperations;
        std::optional<frontend::Response> explicitStartResponse;
        std::optional<frontend::Response> resumeResponse;
        std::optional<frontend::Response> explicitTurnResponse;
        std::optional<frontend::Response> newStartResponse;
        std::optional<frontend::Response> newTurnResponse;
        std::vector<std::string> lifecycleFailures;
        std::ostringstream humanOutput;
        std::ostringstream diagnostics;

        {
            const auto transport = std::make_shared<tests::codex::FakeTransportState>();
            tests::codex::installInitializingFake(transport, [&](const Json& message, const TransportCallbacks& callbacks) {
                const auto method = message.find("method");
                const auto id = message.find("id");
                if (method == message.end() || !method->is_string() || id == message.end()) {
                    return;
                }

                const std::string methodName = method->get<std::string>();
                const Json params = message.value("params", Json::object());
                if (methodName == "thread/list") {
                    ++appServerThreadListCount;
                    tests::codex::inject(callbacks,
                                         Json{{"id", *id},
                                              {"result",
                                               {{"data", Json::array({tests::codex::threadValue(std::string(PersistedThreadId))})},
                                                {"nextCursor", nullptr},
                                                {"backwardsCursor", nullptr}}}});
                    return;
                }

                appServerOperations.emplace_back(methodName, params);
                if (methodName == "thread/start") {
                    const std::string threadId = params.value("cwd", std::string{}) == "/tmp/acceptance-new"
                                                     ? std::string(NewThreadId)
                                                     : std::string(ExplicitThreadId);
                    tests::codex::inject(callbacks, Json{{"id", *id}, {"result", tests::codex::threadOperationResult(threadId)}});
                } else if (methodName == "thread/resume") {
                    tests::codex::inject(
                        callbacks, Json{{"id", *id}, {"result", tests::codex::threadOperationResult(std::string(PersistedThreadId))}});
                } else if (methodName == "turn/start") {
                    const std::string threadId = params.value("threadId", std::string{});
                    const std::string turnId = threadId == NewThreadId ? "turn-new-acceptance" : "turn-resumed-acceptance";
                    tests::codex::inject(callbacks, Json{{"id", *id}, {"result", tests::codex::turnOperationResult(threadId, turnId)}});
                }
            });

            backend::BackendCoreOptions backendOptions;
            backendOptions.initialThreadListLimit = 1;
            FakeBackendCore backendCore(backendOptions, transport);

            apps::codex_backend::SocketFrontendOptions frontendOptions;
            using Server = net::un::stream::legacy::SocketServer<apps::codex_backend::CodexFrontendSocketContextFactory,
                                                                 FakeBackendCore&,
                                                                 apps::codex_backend::SocketFrontendOptions>;
            Server server("codex-client-thread-workflow-server", backendCore, std::move(frontendOptions));
            server.getConfig()->Instance::forceUnrequired();
            backendCore.start();

            Pipe inputPipe;
            const std::string input = "acquire\n"
                                      "start --cwd /tmp/acceptance-start --model start-model --model-provider start-provider "
                                      "--approval-policy on-request --sandbox-mode workspace-write --ephemeral\n"
                                      "acquire\n"
                                      "resume thread-persisted-acceptance --cwd /tmp/acceptance-resume --model resume-model "
                                      "--model-provider resume-provider --approval-policy never --sandbox-mode read-only\n"
                                      "turn thread-persisted-acceptance Continue the previous task.\n"
                                      "acquire\n"
                                      "new --cwd /tmp/acceptance-new --model new-model --model-provider new-provider "
                                      "--approval-policy on-request --sandbox-mode workspace-write --ephemeral -- "
                                      "Review  the current implementation. --literal\n";
            result.expectTrue(inputPipe.valid() && inputPipe.writeAll(input) && inputPipe.closeWriter(),
                              "the deterministic thread-workflow pipe contains every command and closes before connection");

            client::Presenter presenter(client::OutputMode::Human, humanOutput, diagnostics);
            client::CommandParser parser("thread-workflow");
            client::ClientConnection* connectionHandle = nullptr;
            client::StdinReader* stdinReaderHandle = nullptr;
            client::CommandDrainController* lifecycleHandle = nullptr;

            client::CommandDrainController lifecycle(client::CommandDrainCallbacks{
                .send =
                    [&](const frontend::ClientMessage& message) {
                        if (const auto* command = std::get_if<frontend::Command>(&message)) {
                            sentCommandIds.push_back(command->requestId);
                            sentCommandMethods.emplace_back(frontend::toString(frontend::commandMethod(command->parameters)));
                            sentCommandSyncCounts.push_back(syncCompleteCount);
                        }
                        return connectionHandle != nullptr && connectionHandle->send(message);
                    },
                .requestExit =
                    [&]() {
                        ++exitRequestCount;
                        exitWaitedForNewTurnResponse = newTurnResponse.has_value() && responseIds.size() == 8;
                        if (stdinReaderHandle != nullptr) {
                            stdinReaderHandle->stop();
                        }
                        if (!eventLoopRunning || exitScheduled) {
                            return;
                        }
                        exitScheduled = true;
                        core::EventReceiver::atNextTick([&]() {
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
                    [&](std::string message) {
                        lifecycleFailures.push_back(message);
                        presenter.error(std::move(message));
                    }});
            lifecycleHandle = &lifecycle;

            client::ClientConnection connection(client::ClientConnectionCallbacks{
                .onConnected =
                    [&]() {
                        ++connectionCallbackCount;
                        lifecycle.connected();
                        presenter.connected(path);
                    },
                .onMessage =
                    [&](const frontend::ServerMessage& message) {
                        const bool initialSync = std::holds_alternative<frontend::SyncComplete>(message) && syncCompleteCount == 0;
                        const auto* response = std::get_if<frontend::Response>(&message);
                        if (initialSync) {
                            ++syncCompleteCount;
                        } else if (response != nullptr) {
                            responseIds.push_back(response->requestId);
                            if (std::find(acquireRequestIds.begin(), acquireRequestIds.end(), response->requestId) !=
                                acquireRequestIds.end()) {
                                ++acquireResponseCount;
                            } else if (response->requestId == explicitStartRequestId) {
                                explicitStartResponse = *response;
                            } else if (response->requestId == resumeRequestId) {
                                resumeResponse = *response;
                            } else if (response->requestId == explicitTurnRequestId) {
                                explicitTurnResponse = *response;
                            } else if (response->requestId == newStartRequestId) {
                                newStartResponse = *response;
                                newStartObservedDuringEofDrain =
                                    lifecycle.inputState() == client::CommandDrainController::InputState::DrainOnEof &&
                                    lifecycle.outcome() == client::CommandDrainController::Outcome::Running;
                            } else if (response->requestId == newTurnRequestId) {
                                newTurnResponse = *response;
                            }
                        } else if (std::holds_alternative<frontend::ProtocolErrorMessage>(message)) {
                            ++protocolErrorCount;
                        }

                        const std::size_t sentBefore = sentCommandIds.size();
                        const std::optional<client::ResponsePresentation> presentation = lifecycle.receive(message);
                        if (presentation.has_value()) {
                            presenter.present(message, *presentation);
                        } else {
                            presenter.present(message);
                        }
                        if (response != nullptr && response->requestId == newStartRequestId) {
                            newTurnSentFromMatchingStart = sentCommandIds.size() == sentBefore + 1 &&
                                                           sentCommandIds.back() == newTurnRequestId &&
                                                           sentCommandMethods.back() == frontend::method::TurnStart;
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
                        lifecycleClosedCleanly = lifecycle.sessionState() == client::CommandDrainController::SessionState::Closed &&
                                                 lifecycle.outcome() == client::CommandDrainController::Outcome::Success &&
                                                 lifecycle.queuedCount() == 0 && lifecycle.pendingResponseCount() == 0 &&
                                                 lifecycle.pendingSyncCount() == 0;
                        completed = true;
                        if (eventLoopRunning) {
                            core::SNodeC::stop();
                        }
                    }});
            connectionHandle = &connection;

            using Client = net::un::stream::legacy::SocketClient<client::CodexBackendClientSocketContextFactory, client::ClientConnection&>;
            Client socketClient("codex-client-thread-workflow-client", connection);
            socketClient.getConfig()->Instance::forceUnrequired();

            std::unique_ptr<client::StdinReader> stdinReader;
            const auto handleInput = [&](std::string line) {
                ++stdinLineCount;
                client::ParsedCommand parsed = parser.parse(line);
                std::visit(
                    [&]<typename Parsed>(Parsed&& command) {
                        using T = std::remove_cvref_t<Parsed>;
                        if constexpr (std::is_same_v<T, client::SendCommand>) {
                            const auto* wireCommand = std::get_if<frontend::Command>(&command.message);
                            if (wireCommand == nullptr) {
                                result.expectTrue(false, "thread-workflow parser produces a typed frontend command");
                                lifecycle.inputFailed("thread-workflow parser produced a non-command message");
                                return;
                            }
                            if (std::holds_alternative<frontend::ControllerAcquire>(wireCommand->parameters)) {
                                acquireRequestIds.push_back(wireCommand->requestId);
                            } else if (std::holds_alternative<frontend::ThreadStart>(wireCommand->parameters)) {
                                explicitStartRequestId = wireCommand->requestId;
                            } else if (std::holds_alternative<frontend::ThreadResume>(wireCommand->parameters)) {
                                resumeRequestId = wireCommand->requestId;
                            } else if (std::holds_alternative<frontend::TurnStart>(wireCommand->parameters)) {
                                explicitTurnRequestId = wireCommand->requestId;
                            } else {
                                result.expectTrue(false, "thread-workflow input contains only acquire, start, resume, turn, and new");
                            }
                            result.expectTrue(lifecycle.enqueue(std::move(command.message)),
                                              "the lifecycle queues each ordinary thread-workflow command before connection");
                        } else if constexpr (std::is_same_v<T, client::NewCommand>) {
                            newStartRequestId = command.threadStartRequestId;
                            newTurnRequestId = command.turnStartRequestId;
                            result.expectTrue(lifecycle.enqueue(std::move(command)),
                                              "the lifecycle queues the client-side new operation before connection");
                        } else {
                            result.expectTrue(false, "thread-workflow pipe parses without local or usage commands");
                            lifecycle.inputFailed("thread-workflow pipe contained an unexpected parsed command");
                        }
                    },
                    std::move(parsed));
            };

            const auto startInputWhenReady = [&]() {
                const backend::Snapshot snapshot = backendCore.snapshot();
                const bool persistedThreadHydrated = snapshot.threads.size() == 1 && snapshot.threads.front().id == PersistedThreadId;
                if (!serverListening || stdinReader != nullptr || snapshot.lifecycle != backend::BackendLifecycle::Ready ||
                    !persistedThreadHydrated) {
                    return;
                }
                backendHydratedBeforeInput = sentCommandIds.empty();
                try {
                    stdinReader = std::make_unique<client::StdinReader>(
                        handleInput,
                        [&]() {
                            ++stdinEofCount;
                            queuedCountAtEof = lifecycle.queuedCount();
                            inputClosedBeforeConnection = !connection.connected() && sentCommandIds.empty();
                            lifecycle.inputEof();
                            eofEnteredDrain = lifecycle.inputState() == client::CommandDrainController::InputState::DrainOnEof &&
                                              lifecycle.outcome() == client::CommandDrainController::Outcome::Running;
                            socketClient.connect(path, [&](const net::un::SocketAddress&, core::socket::State state) {
                                if (state == core::socket::State::OK) {
                                    ++connectSuccessCount;
                                } else {
                                    lifecycle.connectionFailed("thread-workflow production client failed to connect");
                                }
                            });
                        },
                        [&](std::string message) {
                            lifecycle.inputFailed(std::move(message));
                        },
                        inputPipe.readFd);
                    stdinReaderHandle = stdinReader.get();
                } catch (const std::exception& exception) {
                    lifecycle.inputFailed("failed to construct thread-workflow stdin reader: " + std::string(exception.what()));
                }
            };

            backend::BackendObserverSubscription hydrationObserver =
                backendCore.subscribe(backend::BackendObserverCallbacks{.onEvents =
                                                                            [&](const std::vector<backend::SequencedBackendEvent>&) {
                                                                                startInputWhenReady();
                                                                            },
                                                                        .onResynchronize =
                                                                            [&](const backend::Snapshot&) {
                                                                                startInputWhenReady();
                                                                            }});
            result.expectTrue(hydrationObserver.isOpen(), "the thread-workflow scenario observes deterministic backend hydration");

            server.listen(path, [&](const net::un::SocketAddress&, core::socket::State state) {
                if (state != core::socket::State::OK) {
                    lifecycle.connectionFailed("thread-workflow real BackendAdapter Unix server failed to listen");
                    return;
                }
                ++listenSuccessCount;
                serverListening = true;
                startInputWhenReady();
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

        const std::vector<std::string> expectedCommandIds{acquireRequestIds.size() > 0 ? acquireRequestIds[0] : std::string{},
                                                          explicitStartRequestId,
                                                          acquireRequestIds.size() > 1 ? acquireRequestIds[1] : std::string{},
                                                          resumeRequestId,
                                                          explicitTurnRequestId,
                                                          acquireRequestIds.size() > 2 ? acquireRequestIds[2] : std::string{},
                                                          newStartRequestId,
                                                          newTurnRequestId};
        const std::vector<std::string> expectedCommandMethods{std::string(frontend::method::ControllerAcquire),
                                                              std::string(frontend::method::ThreadStart),
                                                              std::string(frontend::method::ControllerAcquire),
                                                              std::string(frontend::method::ThreadResume),
                                                              std::string(frontend::method::TurnStart),
                                                              std::string(frontend::method::ControllerAcquire),
                                                              std::string(frontend::method::ThreadStart),
                                                              std::string(frontend::method::TurnStart)};
        const std::vector<std::pair<std::string, Json>> expectedAppServerOperations{
            {"thread/start",
             Json{{"cwd", "/tmp/acceptance-start"},
                  {"model", "start-model"},
                  {"modelProvider", "start-provider"},
                  {"approvalPolicy", "on-request"},
                  {"sandbox", "workspace-write"},
                  {"ephemeral", true}}},
            {"thread/resume",
             Json{{"threadId", PersistedThreadId},
                  {"cwd", "/tmp/acceptance-resume"},
                  {"model", "resume-model"},
                  {"modelProvider", "resume-provider"},
                  {"approvalPolicy", "never"},
                  {"sandbox", "read-only"}}},
            {"turn/start",
             Json{{"threadId", PersistedThreadId},
                  {"input", Json::array({Json{{"type", "text"}, {"text", ExplicitPrompt}, {"text_elements", Json::array()}}})}}},
            {"thread/start",
             Json{{"cwd", "/tmp/acceptance-new"},
                  {"model", "new-model"},
                  {"modelProvider", "new-provider"},
                  {"approvalPolicy", "on-request"},
                  {"sandbox", "workspace-write"},
                  {"ephemeral", true}}},
            {"turn/start",
             Json{{"threadId", NewThreadId},
                  {"input", Json::array({Json{{"type", "text"}, {"text", NewPrompt}, {"text_elements", Json::array()}}})}}}};

        const auto responseHasThread = [](const std::optional<frontend::Response>& response, std::string_view threadId) {
            if (!response.has_value() || !response->ok || !response->result.has_value()) {
                return false;
            }
            const auto thread = response->result->find("thread");
            return thread != response->result->end() && thread->is_object() && thread->value("id", std::string{}) == threadId;
        };
        const auto responseHasTurn = [](const std::optional<frontend::Response>& response, std::string_view threadId) {
            if (!response.has_value() || !response->ok || !response->result.has_value()) {
                return false;
            }
            const auto turn = response->result->find("turn");
            return turn != response->result->end() && turn->is_object() && turn->value("threadId", std::string{}) == threadId;
        };

        std::vector<std::string> uniqueRequestIds = sentCommandIds;
        std::sort(uniqueRequestIds.begin(), uniqueRequestIds.end());
        const bool requestIdsAreUnique = std::adjacent_find(uniqueRequestIds.begin(), uniqueRequestIds.end()) == uniqueRequestIds.end();
        result.expectTrue(!timedOut && completed, "the EOF-driven thread-workflow acceptance completes before its watchdog");
        result.expectEqual(0, eventLoopResult, "the EOF-driven thread-workflow event loop stops cleanly");
        result.expectEqual(1, static_cast<int>(listenSuccessCount), "the thread-workflow real frontend server listens exactly once");
        result.expectEqual(1, static_cast<int>(connectSuccessCount), "the thread-workflow production client connects exactly once");
        result.expectEqual(
            1, static_cast<int>(connectionCallbackCount), "the thread-workflow production connection callback runs exactly once");
        result.expectTrue(backendHydratedBeforeInput && appServerThreadListCount == 1,
                          "the persisted thread is list-hydrated before the workflow pipe is consumed");
        result.expectTrue(stdinLineCount == 7 && stdinEofCount == 1 && queuedCountAtEof == 7 && inputClosedBeforeConnection &&
                              eofEnteredDrain,
                          "all seven commands and EOF are queued deterministically before the Unix connection");
        result.expectTrue(sentCommandIds == expectedCommandIds && sentCommandMethods == expectedCommandMethods,
                          "acquire/start, acquire/resume/turn, and acquire/new cross the frontend in exact request order");
        result.expectTrue(requestIdsAreUnique && newStartRequestId != newTurnRequestId,
                          "every workflow request ID is unique and new allocates distinct IDs for its two stages");
        result.expectTrue(sentCommandSyncCounts == std::vector<std::size_t>(8, 1),
                          "every workflow command, including new's turn, is sent only after initial synchronization");
        result.expectTrue(appServerOperations == expectedAppServerOperations,
                          "the real BackendAdapter maps every start/resume option and both exact TextInput prompts in order");
        result.expectTrue(newStartObservedDuringEofDrain && newTurnSentFromMatchingStart && exitWaitedForNewTurnResponse,
                          "EOF remains draining while new waits for its matching start response and subsequent turn response");
        result.expectTrue(responseHasThread(explicitStartResponse, ExplicitThreadId) &&
                              responseHasThread(resumeResponse, PersistedThreadId) &&
                              responseHasTurn(explicitTurnResponse, PersistedThreadId) &&
                              responseHasThread(newStartResponse, NewThreadId) && responseHasTurn(newTurnResponse, NewThreadId),
                          "real BackendAdapter responses preserve explicit, resumed, and generated thread IDs through both workflows");
        result.expectTrue(acquireRequestIds.size() == 3 && acquireResponseCount == 3 && responseIds.size() == 8,
                          "controller ownership is explicitly acquired for each workflow and all real responses are correlated");
        result.expectEqual(0, static_cast<int>(protocolErrorCount), "the thread-workflow scenario reports no frontend protocol error");
        result.expectEqual(1, static_cast<int>(exitRequestCount), "EOF completion requests exactly one controlled exit");
        result.expectEqual(1, static_cast<int>(disconnectCallbackCount), "the thread-workflow client disconnects exactly once");
        result.expectTrue(lifecycleFailures.empty() && lifecycleClosedCleanly,
                          "the compound new workflow closes with no queued or pending lifecycle work");
        result.expectTrue(std::none_of(sentCommandMethods.begin(),
                                       sentCommandMethods.end(),
                                       [](const std::string& method) {
                                           return method.find("new") != std::string::npos;
                                       }) &&
                              std::none_of(appServerOperations.begin(),
                                           appServerOperations.end(),
                                           [](const auto& operation) {
                                               return operation.first.find("new") != std::string::npos;
                                           }),
                          "new emits only real thread.start and turn.start operations with no synthetic protocol method");
        result.expectTrue(contains(humanOutput.str(), "thread started id=" + std::string(ExplicitThreadId)) &&
                              contains(humanOutput.str(), "thread resumed id=" + std::string(PersistedThreadId)) &&
                              contains(humanOutput.str(), "thread created id=" + std::string(NewThreadId)) &&
                              contains(humanOutput.str(), "initial turn submitted thread=" + std::string(NewThreadId)) &&
                              diagnostics.str().empty(),
                          "the production presenter emits bounded lifecycle summaries without diagnostics on the successful workflow");
        result.expectTrue(!pathExists(path), "the thread-workflow Unix server removes its owned socket path");

        core::SNodeC::free();
        std::remove(path.c_str());
    }

} // namespace

int main(int argc, char* argv[]) {
    tests::support::TestResult result;
    int returnCode = tests::support::cTestSkipReturnCode;
    const std::string path = socketPath();
    const char* configuredScenario = std::getenv("SNODEC_CODEX_CLIENT_ACCEPTANCE_SCENARIO");
    const std::string_view scenario = configuredScenario == nullptr ? "interactive" : configuredScenario;
    const std::string testName =
        scenario == "thread-workflow" ? "CodexBackendClientThreadWorkflowAcceptanceTest" : "CodexBackendClientRealBackendAcceptanceTest";

    if (scenario != "interactive" && scenario != "thread-workflow") {
        result.expectTrue(false,
                          "unknown SNODEC_CODEX_CLIENT_ACCEPTANCE_SCENARIO '" + std::string(scenario) +
                              "'; expected 'interactive' or 'thread-workflow'");
        returnCode = result.processResult();
    } else if (tests::support::shouldSkipRootWithoutSNodeCGroup()) {
        tests::support::printRootWithoutSNodeCGroupSkipMessage(testName);
    } else if (scenario == "thread-workflow") {
        runThreadWorkflowAcceptance(result, argc, argv);
        returnCode = result.processResult();
    } else {
        std::remove(path.c_str());
        result.expectTrue(!pathExists(path), "the unique real-backend client socket path is absent before listen");
        core::SNodeC::init(argc, argv);

        bool timedOut = false;
        int eventLoopResult = 1;
        bool backendReadyForHandshake = false;
        bool backendHydratedBeforeConnect = false;
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
        std::optional<frontend::Snapshot> initialSnapshot;
        std::optional<frontend::Response> threadListResponse;
        std::size_t snapshotHumanBytes = 0;
        std::size_t threadsResponseHumanBytes = 0;
        WindowedStreamBuffer humanBuffer(HumanPresentationWindowBytes);
        std::ostream humanOutput(&humanBuffer);
        std::ostringstream diagnostics;

        {
            const auto transport = std::make_shared<tests::codex::FakeTransportState>();
            tests::codex::installInitializingFake(transport,
                                                  [&appServerThreadListCount](const Json& message, const TransportCallbacks& callbacks) {
                                                      if (message.value("method", std::string{}) != "thread/list") {
                                                          return;
                                                      }
                                                      const auto id = message.find("id");
                                                      if (id == message.end()) {
                                                          return;
                                                      }
                                                      ++appServerThreadListCount;
                                                      tests::codex::inject(callbacks, Json{{"id", *id}, {"result", largeThreadPage()}});
                                                  });

            backend::BackendCoreOptions backendOptions;
            backendOptions.initialThreadListLimit = LargeThreadCount;
            FakeBackendCore backendCore(backendOptions, transport);

            apps::codex_backend::SocketFrontendOptions frontendOptions;
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
                        const bool initialSync = std::holds_alternative<frontend::SyncComplete>(message) && syncCompleteCount == 0;
                        const auto* snapshot = std::get_if<frontend::Snapshot>(&message);
                        const auto* response = std::get_if<frontend::Response>(&message);
                        const bool threadsResponse = response != nullptr && response->requestId == threadsRequestId;
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
                        } else if (snapshot != nullptr) {
                            ++snapshotCount;
                            if (!initialSnapshot.has_value()) {
                                initialSnapshot = *snapshot;
                            }
                        } else if (initialSync) {
                            ++syncCompleteCount;
                            syncObservedWhileSynchronizing =
                                lifecycle.sessionState() == client::CommandDrainController::SessionState::Synchronizing &&
                                lifecycle.queuedCount() == 1;
                        } else if (response != nullptr) {
                            if (response->requestId == acquireRequestId) {
                                ++acquireResponseCount;
                                acquireResponseSucceeded = acquireResponseSucceeded || response->ok;
                            } else if (response->requestId == threadsRequestId) {
                                ++threadsResponseCount;
                                threadsResponseSucceeded = threadsResponseSucceeded || response->ok;
                                threadListResponse = *response;
                            }
                        } else if (std::holds_alternative<frontend::ProtocolErrorMessage>(message)) {
                            ++protocolErrorCount;
                        }

                        const std::size_t outputBytesBeforePresentation = humanBuffer.size();
                        lifecycle.receive(message);
                        presenter.present(message);
                        const std::size_t presentedBytes = humanBuffer.size() - outputBytesBeforePresentation;
                        if (snapshot != nullptr) {
                            snapshotHumanBytes = presentedBytes;
                        } else if (threadsResponse) {
                            threadsResponseHumanBytes = presentedBytes;
                        }

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

            bool serverListening = false;
            std::function<void()> startInteractiveInputWhenHydrated = [&]() {
                if (!serverListening || stdinReader != nullptr) {
                    return;
                }
                if (!isLargeSnapshotHydrated(backendCore.snapshot())) {
                    return;
                }

                backendHydratedBeforeConnect = !connection.connected() &&
                                               lifecycle.sessionState() == client::CommandDrainController::SessionState::Connecting &&
                                               sentCommandIds.empty();
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
            };

            backend::BackendObserverSubscription hydrationObserver =
                backendCore.subscribe(backend::BackendObserverCallbacks{.onEvents =
                                                                            [&](const std::vector<backend::SequencedBackendEvent>&) {
                                                                                startInteractiveInputWhenHydrated();
                                                                            },
                                                                        .onResynchronize =
                                                                            [&](const backend::Snapshot&) {
                                                                                startInteractiveInputWhenHydrated();
                                                                            }});
            result.expectTrue(hydrationObserver.isOpen(), "a real BackendCore observer gates frontend connection on initial hydration");

            server.listen(path, [&](const net::un::SocketAddress&, core::socket::State state) {
                if (state != core::socket::State::OK) {
                    lifecycle.connectionFailed("real BackendAdapter Unix server failed to listen");
                    return;
                }
                ++listenSuccessCount;
                serverListening = true;
                startInteractiveInputWhenHydrated();
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
        const std::string output = humanBuffer.str();
        const std::string errors = diagnostics.str();
        std::string snapshotWire;
        std::string jsonSnapshotOutput;
        std::string jsonSnapshotDiagnostics;
        bool snapshotRoundTrips = false;
        bool snapshotContainsCompletePreview = false;
        if (initialSnapshot.has_value()) {
            const frontend::ServerMessage snapshotMessage{*initialSnapshot};
            const auto encoded = frontend::Codec::serializeServer(snapshotMessage);
            if (encoded) {
                snapshotWire = encoded.value();
                const auto decoded = frontend::Codec::decodeServer(std::string_view(snapshotWire));
                snapshotRoundTrips = decoded && decoded.value() == snapshotMessage;

                std::ostringstream jsonOutput;
                std::ostringstream jsonDiagnostics;
                client::Presenter jsonPresenter(client::OutputMode::Json, jsonOutput, jsonDiagnostics);
                jsonPresenter.present(snapshotMessage);
                jsonSnapshotOutput = jsonOutput.str();
                jsonSnapshotDiagnostics = jsonDiagnostics.str();
            }

            const auto threads = initialSnapshot->state.find("threads");
            if (threads != initialSnapshot->state.end() && threads->is_array()) {
                const auto expectedPreview = largePreview(0);
                snapshotContainsCompletePreview = std::any_of(threads->begin(), threads->end(), [&](const Json& thread) {
                    return thread.value("id", std::string{}) == largeThreadId(0) &&
                           thread.value("preview", std::string{}) == expectedPreview;
                });
            }
        }

        std::size_t threadListResponseWireBytes = 0;
        if (threadListResponse.has_value()) {
            const auto encoded = frontend::Codec::serializeServer(frontend::ServerMessage{*threadListResponse});
            if (encoded) {
                threadListResponseWireBytes = encoded.value().size();
            }
        }

        std::ostringstream maximumUnsignedOutput;
        std::ostringstream maximumUnsignedDiagnostics;
        client::Presenter maximumUnsignedPresenter(client::OutputMode::Human, maximumUnsignedOutput, maximumUnsignedDiagnostics);
        constexpr std::uint64_t MaximumUnsigned = std::numeric_limits<std::uint64_t>::max();
        maximumUnsignedPresenter.present(frontend::ServerMessage{frontend::Snapshot{frontend::SequenceNumber{MaximumUnsigned},
                                                                                    Json{{"backendRevision", MaximumUnsigned},
                                                                                         {"lifecycle", "ready"},
                                                                                         {"threads", Json::array()},
                                                                                         {"pendingRequests", Json::array()},
                                                                                         {"sessions", Json::array()},
                                                                                         {"threadList", {{"complete", true}}}},
                                                                                    Json::object()}});

        result.expectTrue(backendReadyForHandshake, "fake AppServerClient brings the real BackendCore to Ready for the frontend handshake");
        result.expectTrue(backendHydratedBeforeConnect,
                          "large fake thread data is hydrated before the production frontend starts its hello handshake");
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
        result.expectTrue(initialSnapshot.has_value() && snapshotContainsCompletePreview,
                          "the typed initial Snapshot contains the complete seeded long preview");
        result.expectTrue(snapshotWire.size() > LargeProtocolThreshold, "the initial real-adapter Snapshot serializes to more than 64 KiB");
        result.expectTrue(snapshotRoundTrips, "the large initial Snapshot round-trips through the production frontend Codec");
        result.expectTrue(jsonSnapshotOutput == snapshotWire + '\n' &&
                              std::count(jsonSnapshotOutput.begin(), jsonSnapshotOutput.end(), '\n') == 1,
                          "JSON Presenter emits exactly the complete compact protocol frame and one record delimiter");
        result.expectTrue(contains(jsonSnapshotOutput, largePreview(0)) && jsonSnapshotDiagnostics.empty(),
                          "JSON Presenter preserves the complete long preview without diagnostics");
        result.expectTrue(threadListResponseWireBytes > LargeProtocolThreshold,
                          "the interactive thread-list Response also carries more than 64 KiB of typed protocol data");
        result.expectTrue(!humanBuffer.overflowed() && humanOutput.good() &&
                              humanBuffer.maximumBurstBytes() <= HumanPresentationWindowBytes,
                          "every flushed human presentation burst fits the deterministic downstream window");
        result.expectTrue(snapshotHumanBytes > 0 && snapshotHumanBytes < HumanPresentationWindowBytes,
                          "human Snapshot presentation remains compact despite its large protocol payload");
        result.expectTrue(threadsResponseHumanBytes > 0 && threadsResponseHumanBytes < HumanPresentationWindowBytes,
                          "human thread-list Response presentation remains compact despite its large protocol payload");
        result.expectTrue(contains(output, "lifecycle=ready") && contains(output, "threads=24") && contains(output, "pending-requests=0") &&
                              contains(output, "controller=none") && contains(output, "sessions=1") &&
                              contains(output, "thread-list-complete=true"),
                          "human Snapshot presents the useful top-level state summary");
        result.expectTrue(contains(output, "result=threads=24 ids=large-thread-0") && !contains(output, "complete-preview-large-thread-"),
                          "human thread-list presentation keeps usable identifiers without expanding previews");
        result.expectTrue(contains(maximumUnsignedOutput.str(), "snapshot sequence=18446744073709551615") &&
                              contains(maximumUnsignedOutput.str(), "backend-revision=18446744073709551615") &&
                              maximumUnsignedDiagnostics.str().empty(),
                          "human Snapshot summary preserves the full unsigned protocol range");
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
