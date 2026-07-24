/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include "ai/openai/codex/AppServerClient.h"
#include "ai/openai/codex/detail/ProtocolSurfaceRegistry.h"
#include "ai/openai/codex/detail/Transport.h"
#include "ai/openai/codex/typed/Client.h"
#include "core/EventReceiver.h"
#include "core/SNodeC.h"
#include "core/timer/Timer.h"
#include "support/TestResult.h"
#include "utils/Timeval.h"

#include <algorithm>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <functional>
#include <iterator>
#include <limits>
#include <map>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <sys/socket.h>
#include <type_traits>
#include <unistd.h>
#include <utility>
#include <variant>
#include <vector>

#ifndef CODEX_A1_FIXTURE_ROOT
#error "CODEX_A1_FIXTURE_ROOT must name the checked-in App Server fixture corpus"
#endif

namespace {
    namespace codex = ai::openai::codex;
    namespace detail = ai::openai::codex::detail;
    namespace typed = ai::openai::codex::typed;

    using Submission = codex::AppServerClient::RawProtocol::Submission;

    constexpr std::string_view ThreadIdValue = "synthetic-bd0d0f38fe";
    constexpr std::string_view TurnIdValue = "synthetic-e026a1a7e9";
    constexpr std::size_t OperationCount = 22;

    static_assert(std::is_same_v<decltype(typed::ThreadRollbackParams::numTurns), std::uint32_t>);
    static_assert(std::numeric_limits<decltype(typed::ThreadRollbackParams::numTurns)>::max() == 4'294'967'295U);
    static_assert(std::is_aggregate_v<typed::ThreadListParams>);
    static_assert(std::is_aggregate_v<typed::ThreadStartParams>);

    static_assert(requires(typed::Threads& threads,
                           typed::Threads::ThreadListResultHandler listHandler,
                           typed::Threads::ThreadResultHandler threadHandler) {
        threads.list({}, std::move(listHandler));
        threads.start({}, std::move(threadHandler));
    });

    bool writeFully(int descriptor, std::string_view bytes) {
        std::size_t offset = 0;
        while (offset < bytes.size()) {
            const ssize_t written = ::write(descriptor, bytes.data() + offset, bytes.size() - offset);
            if (written > 0) {
                offset += static_cast<std::size_t>(written);
            } else if (written < 0 && errno == EINTR) {
                continue;
            } else {
                return false;
            }
        }
        return true;
    }

    std::optional<std::string> readLine(int descriptor) {
        std::string line;
        for (;;) {
            char byte = '\0';
            const ssize_t received = ::read(descriptor, &byte, 1);
            if (received == 1) {
                line.push_back(byte);
                if (byte == '\n') {
                    return line;
                }
            } else if (received < 0 && errno == EINTR) {
                continue;
            } else {
                return std::nullopt;
            }
        }
    }

    codex::Json readFixture(std::string_view slug, std::string_view kind) {
        const std::string path =
            std::string(CODEX_A1_FIXTURE_ROOT) + "/cases/operations/client/" + std::string(slug) + "/" + std::string(kind) + ".json";
        std::ifstream input(path, std::ios::binary);
        if (!input) {
            return nullptr;
        }
        try {
            return codex::Json::parse(std::istreambuf_iterator<char>{input}, std::istreambuf_iterator<char>{});
        } catch (...) {
            return nullptr;
        }
    }

    codex::Json initializeResult() {
        return {
            {"codexHome", "/tmp/codex-a1-1-wire"},
            {"platformFamily", "unix"},
            {"platformOs", "linux"},
            {"userAgent", "codex-a1-1-wire/1"},
        };
    }

    struct OutboundRecord {
        std::string line;
        codex::Json envelope;
    };

    struct UnixTransportState {
        enum class ReplyMode { Success, RemoteError, Hold };

        detail::TransportCallbacks callbacks;
        std::vector<detail::TransportCallbacks> callbackGenerations;
        std::vector<OutboundRecord> outbound;
        std::map<std::string, codex::Json> successResults;
        ReplyMode replyMode = ReplyMode::Success;
        bool injectObservers = false;
        bool running = false;
        int clientDescriptor = -1;
        int serverDescriptor = -1;

        ~UnixTransportState() {
            if (clientDescriptor >= 0) {
                ::close(clientDescriptor);
            }
            if (serverDescriptor >= 0) {
                ::close(serverDescriptor);
            }
        }

        bool open() {
            int descriptors[2]{-1, -1};
            if (::socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, descriptors) != 0) {
                return false;
            }
            clientDescriptor = descriptors[0];
            serverDescriptor = descriptors[1];
            return true;
        }

        bool injectWith(const detail::TransportCallbacks& target, const codex::Json& envelope) {
            const std::string outboundLine = envelope.dump() + "\n";
            if (!writeFully(serverDescriptor, outboundLine)) {
                return false;
            }
            const std::optional<std::string> received = readLine(clientDescriptor);
            if (!received || *received != outboundLine || !target.onMessage) {
                return false;
            }
            target.onMessage(received->substr(0, received->size() - 1));
            return true;
        }

        bool injectCurrent(const codex::Json& envelope) {
            return injectWith(callbacks, envelope);
        }

        bool send(std::string message) {
            const std::string clientLine = message + "\n";
            if (!writeFully(clientDescriptor, clientLine)) {
                return false;
            }
            const std::optional<std::string> received = readLine(serverDescriptor);
            if (!received || *received != clientLine) {
                return false;
            }

            codex::Json envelope;
            try {
                envelope = codex::Json::parse(received->begin(), received->end() - 1);
            } catch (...) {
                return false;
            }
            outbound.push_back({*received, envelope});

            const auto method = envelope.find("method");
            if (method == envelope.end() || !method->is_string()) {
                return true;
            }
            if (*method == "initialize") {
                const auto id = envelope.find("id");
                return id != envelope.end() && injectCurrent({{"id", *id}, {"result", initializeResult()}});
            }
            if (*method == "initialized" || envelope.find("id") == envelope.end()) {
                return true;
            }
            if (replyMode == ReplyMode::Hold) {
                return true;
            }

            const std::string operation = *method;
            if (injectObservers && !injectCurrent({{"method", "test/a1-1/raw-observer"}, {"params", {{"operation", operation}}}})) {
                return false;
            }

            if (replyMode == ReplyMode::RemoteError) {
                const codex::Json error = {
                    {"code", -32'222},
                    {"message", "synthetic remote failure"},
                    {"data", {{"operation", operation}, {"retained", true}}},
                    {"futureErrorMetadata", {{"operation", operation}, {"preserve", "exact"}}},
                };
                return injectCurrent({{"id", envelope.at("id")}, {"error", error}});
            }

            const auto result = successResults.find(operation);
            return result != successResults.end() && injectCurrent({{"id", envelope.at("id")}, {"result", result->second}});
        }
    };

    class UnixTranscriptTransport final : public detail::Transport {
    public:
        explicit UnixTranscriptTransport(std::shared_ptr<UnixTransportState> state)
            : state(std::move(state)) {
        }

        void setCallbacks(detail::TransportCallbacks callbacks) override {
            state->callbacks = std::move(callbacks);
            if (state->callbacks.onStarted || state->callbacks.onMessage || state->callbacks.onDiagnostic || state->callbacks.onError ||
                state->callbacks.onExited) {
                state->callbackGenerations.push_back(state->callbacks);
            }
        }

        void start() override {
            state->running = true;
            if (state->callbacks.onStarted) {
                state->callbacks.onStarted();
            }
        }

        bool send(std::string message) override {
            return state->send(std::move(message));
        }

        void stop() override {
            if (!std::exchange(state->running, false)) {
                return;
            }
            if (state->callbacks.onExited) {
                state->callbacks.onExited(detail::ProcessExit{true, 0, false, 0});
            }
        }

    private:
        std::shared_ptr<UnixTransportState> state;
    };

    class TestClient final : public codex::AppServerClient {
    public:
        explicit TestClient(const std::shared_ptr<UnixTransportState>& state)
            : AppServerClient(std::make_unique<UnixTranscriptTransport>(state), {"codex_a1_1_wire_test", "Codex A1.1 Wire Test", "1"}) {
        }
    };

    typed::ThreadId threadId() {
        return {std::string(ThreadIdValue)};
    }

    typed::TurnId turnId() {
        return {std::string(TurnIdValue)};
    }

    typed::UserInput textInput() {
        typed::TextUserInput input;
        input.text = "synthetic-796383c7b4";
        typed::TextElement element;
        element.byteRange = {0, 0};
        element.placeholder = std::string{"synthetic-3cb304aacb"};
        input.textElements = std::vector<typed::TextElement>{std::move(element)};
        return input;
    }

    typed::ThreadForkParams threadForkParams() {
        typed::ThreadForkParams params;
        params.threadId = threadId();
        params.approvalPolicy = typed::AskForApproval{typed::ApprovalPolicy::untrusted()};
        params.approvalsReviewer = typed::ApprovalsReviewer::user();
        params.baseInstructions = std::string{"synthetic-7f79c0118e"};
        params.config = typed::ProtocolConfiguration{};
        params.cwd = std::string{"synthetic-5d9a348799"};
        params.developerInstructions = std::string{"synthetic-d0ce5f0564"};
        params.ephemeral = false;
        params.serviceTier = std::string{"synthetic-df688533b9"};
        params.lastTurnId = typed::TurnId{"synthetic-440f9a3f23"};
        params.model = typed::ModelId{"synthetic-7ec171ea8d"};
        params.modelProvider = std::string{"synthetic-fac534a664"};
        params.threadSource = typed::ThreadSource{"synthetic-2db5dd3788"};
        params.sandbox = typed::SandboxMode::readOnly();
        return params;
    }

    typed::ThreadGoalSetParams threadGoalSetParams() {
        typed::ThreadGoalSetParams params;
        params.threadId = threadId();
        params.objective = std::string{"synthetic-4dfad93d78"};
        params.status = typed::ThreadGoalStatus::active();
        params.tokenBudget = 0;
        return params;
    }

    typed::ThreadListParams threadListParams() {
        typed::ThreadListParams params;
        params.sourceKinds = std::vector<typed::ThreadSourceKind>{typed::ThreadSourceKind::cli()};
        params.archived = false;
        params.cursor = std::string{"synthetic-812497d6a7"};
        params.cwd = typed::ThreadListCwdFilter{std::string{"synthetic-a1e31d316f"}};
        params.limit = 0;
        params.modelProviders = std::vector<std::string>{"synthetic-987a91ec9d"};
        params.useStateDbOnly = false;
        params.searchTerm = std::string{"synthetic-db1c2a4f35"};
        params.sortDirection = typed::SortDirection::ascending();
        params.sortKey = typed::ThreadSortKey::createdAt();
        return params;
    }

    typed::ThreadLoadedListParams threadLoadedListParams() {
        typed::ThreadLoadedListParams params;
        params.cursor = std::string{"synthetic-812497d6a7"};
        params.limit = 0;
        return params;
    }

    typed::ThreadMetadataUpdateParams threadMetadataUpdateParams() {
        typed::ThreadMetadataGitInfoUpdateParams gitInfo;
        gitInfo.branch = std::string{"synthetic-88c5cd91a8"};
        gitInfo.originUrl = std::string{"synthetic-7c5ecb00e8"};
        gitInfo.sha = std::string{"synthetic-c3a5f27a0d"};
        typed::ThreadMetadataUpdateParams params;
        params.threadId = threadId();
        params.gitInfo = std::move(gitInfo);
        return params;
    }

    typed::ThreadResumeParams threadResumeParams() {
        typed::ThreadResumeParams params;
        params.threadId = threadId();
        params.approvalPolicy = typed::AskForApproval{typed::ApprovalPolicy::untrusted()};
        params.approvalsReviewer = typed::ApprovalsReviewer::user();
        params.baseInstructions = std::string{"synthetic-7f79c0118e"};
        params.config = typed::ProtocolConfiguration{};
        params.cwd = std::string{"synthetic-5d9a348799"};
        params.developerInstructions = std::string{"synthetic-d0ce5f0564"};
        params.personality = typed::Personality::none();
        params.serviceTier = std::string{"synthetic-df688533b9"};
        params.model = typed::ModelId{"synthetic-7ec171ea8d"};
        params.modelProvider = std::string{"synthetic-fac534a664"};
        params.sandbox = typed::SandboxMode::readOnly();
        return params;
    }

    typed::ThreadStartParams threadStartParams() {
        typed::ThreadStartParams params;
        params.sessionStartSource = typed::ThreadStartSource::startup();
        params.approvalPolicy = typed::AskForApproval{typed::ApprovalPolicy::untrusted()};
        params.approvalsReviewer = typed::ApprovalsReviewer::user();
        params.baseInstructions = std::string{"synthetic-7f79c0118e"};
        params.config = typed::ProtocolConfiguration{};
        params.cwd = std::string{"synthetic-5d9a348799"};
        params.developerInstructions = std::string{"synthetic-d0ce5f0564"};
        params.serviceName = std::string{"synthetic-2c513307b8"};
        params.personality = typed::Personality::none();
        params.ephemeral = false;
        params.threadSource = typed::ThreadSource{"synthetic-2db5dd3788"};
        params.sandbox = typed::SandboxMode::readOnly();
        params.serviceTier = std::string{"synthetic-df688533b9"};
        params.model = typed::ModelId{"synthetic-7ec171ea8d"};
        params.modelProvider = std::string{"synthetic-fac534a664"};
        return params;
    }

    typed::TurnStartParams turnStartParams() {
        typed::TurnStartParams params;
        params.threadId = threadId();
        params.input = {textInput()};
        params.personality = typed::Personality::none();
        params.approvalPolicy = typed::AskForApproval{typed::ApprovalPolicy::untrusted()};
        params.approvalsReviewer = typed::ApprovalsReviewer::user();
        params.clientUserMessageId = typed::ClientUserMessageId{"synthetic-e93fab63d2"};
        params.serviceTier = std::string{"synthetic-df688533b9"};
        params.cwd = std::string{"synthetic-5d9a348799"};
        params.effort = typed::ReasoningEffort{"synthetic-b43c9594ba"};
        params.model = typed::ModelId{"synthetic-7ec171ea8d"};
        params.summary = typed::ReasoningSummary::automatic();
        params.outputSchema = codex::Json::object();
        params.sandboxPolicy = typed::SandboxPolicy{typed::DangerFullAccessSandboxPolicy{}};
        return params;
    }

    typed::TurnSteerParams turnSteerParams() {
        typed::TurnSteerParams params;
        params.threadId = threadId();
        params.expectedTurnId = typed::TurnId{"synthetic-3f6f42ff96"};
        params.input = {textInput()};
        params.clientUserMessageId = typed::ClientUserMessageId{"synthetic-e93fab63d2"};
        return params;
    }

    bool hasUnknownEnumDiagnostic(const std::vector<typed::DecodeDiagnostic>& diagnostics,
                                  std::string_view surface,
                                  std::string_view fieldPath) {
        return std::any_of(diagnostics.begin(), diagnostics.end(), [&](const typed::DecodeDiagnostic& diagnostic) {
            return diagnostic.kind == typed::DecodeIssueKind::UnknownEnumValue &&
                   diagnostic.severity == typed::DecodeIssueSeverity::ForwardCompatibility && diagnostic.surface == surface &&
                   diagnostic.fieldPath == fieldPath;
        });
    }

    bool threadFieldsMatch(const typed::Thread& thread) {
        const auto* source = std::get_if<typed::SessionSourceKind>(&thread.source);
        const auto* status = std::get_if<typed::NotLoadedThreadStatus>(&thread.status);
        return thread.id.value == "synthetic-2852e52f40" && thread.preview == "synthetic-b23c0e80e9" && !thread.ephemeral &&
               thread.modelProvider == "synthetic-d975f8e1aa" && thread.createdAt == 0 && thread.updatedAt == 0 && status &&
               thread.path.present && thread.path.value && *thread.path.value == "synthetic-98f2b2af35" &&
               thread.cwd.value == "synthetic-75177d933c" && thread.cliVersion == "synthetic-8fd2f0fd37" && source &&
               source->value == "cli" && thread.agentRole.present && thread.agentRole.value &&
               *thread.agentRole.value == "synthetic-6e6c65c126" && thread.agentNickname.present && thread.agentNickname.value &&
               *thread.agentNickname.value == "synthetic-2be1c2010f" && thread.turns.size() == 1 && thread.name.present &&
               thread.name.value && *thread.name.value == "synthetic-dfd2e6e7cd" && thread.gitInfo.present && thread.gitInfo.value &&
               thread.gitInfo.value->branch.value && *thread.gitInfo.value->branch.value == "synthetic-170356d5af" &&
               thread.gitInfo.value->originUrl.value && *thread.gitInfo.value->originUrl.value == "synthetic-4ca2b8d64d" &&
               thread.gitInfo.value->sha.value && *thread.gitInfo.value->sha.value == "synthetic-a15c69bc92" && thread.forkedFromId.value &&
               thread.forkedFromId.value->value == "synthetic-d71f2c3e86" && thread.parentThreadId.value &&
               thread.parentThreadId.value->value == "synthetic-b5f2ddde9c" && thread.sessionId.value == "synthetic-4d32f359ba" &&
               thread.recencyAt.value && *thread.recencyAt.value == 0 && thread.threadSource.value &&
               thread.threadSource.value->value == "synthetic-2db5dd3788";
    }

    bool goalFieldsMatch(const typed::ThreadGoal& goal) {
        return goal.createdAt == 0 && goal.objective == "synthetic-f6211b9cb1" && goal.status == typed::ThreadGoalStatus::active() &&
               goal.threadId.value == "synthetic-5ffdd90d61" && goal.timeUsedSeconds == 0 && goal.tokenBudget.present &&
               goal.tokenBudget.value && *goal.tokenBudget.value == 0 && goal.tokensUsed == 0 && goal.updatedAt == 0;
    }

    bool turnFieldsMatch(const typed::Turn& turn) {
        return turn.id.value == "synthetic-48be160827" && turn.threadId.value == ThreadIdValue &&
               turn.status == typed::TurnStatus::completed() && turn.items.size() == 1 &&
               std::holds_alternative<typed::UserMessageThreadItem>(turn.items.front()) &&
               turn.itemsView == typed::TurnItemsView::notLoaded() && turn.error.present && turn.error.value &&
               turn.error.value->message == "synthetic-65f454ad02" && turn.startedAt.value && *turn.startedAt.value == 0 &&
               turn.completedAt.value && *turn.completedAt.value == 0 && turn.durationMs.value && *turn.durationMs.value == 0;
    }

    template <typename Response>
    bool launchResponseFieldsMatch(const Response& response) {
        const auto* approval = std::get_if<typed::ApprovalPolicy>(&response.approvalPolicy);
        return response.serviceTier.present && response.serviceTier.value && *response.serviceTier.value == "synthetic-df688533b9" &&
               approval && *approval == typed::ApprovalPolicy::untrusted() &&
               response.approvalsReviewer == typed::ApprovalsReviewer::user() && response.cwd.value == "synthetic-75177d933c" &&
               response.instructionSources.size() == 1 && response.instructionSources.front().value == "synthetic-f2afe5ff17" &&
               response.model.value == "synthetic-7ec171ea8d" && response.modelProvider == "synthetic-fac534a664" &&
               std::holds_alternative<typed::DangerFullAccessSandboxPolicy>(response.sandbox) && response.reasoningEffort.present &&
               response.reasoningEffort.value && response.reasoningEffort.value->value == "synthetic-b43c9594ba" &&
               hasUnknownEnumDiagnostic(response.diagnostics, "ReasoningEffort", "$.reasoningEffort") && threadFieldsMatch(response.thread);
    }

    template <typename Result>
    bool resultFieldsMatch(const Result& value) {
        if constexpr (std::is_same_v<Result, typed::Unit>) {
            return true;
        } else if constexpr (std::is_same_v<Result, typed::ThreadForkResponse> || std::is_same_v<Result, typed::ThreadResumeResponse> ||
                             std::is_same_v<Result, typed::ThreadStartResponse>) {
            return launchResponseFieldsMatch(value);
        } else if constexpr (std::is_same_v<Result, typed::ThreadGoalClearResponse>) {
            return !value.cleared;
        } else if constexpr (std::is_same_v<Result, typed::ThreadGoalGetResponse>) {
            return value.goal.present && value.goal.value && goalFieldsMatch(*value.goal.value);
        } else if constexpr (std::is_same_v<Result, typed::ThreadGoalSetResponse>) {
            return goalFieldsMatch(value.goal);
        } else if constexpr (std::is_same_v<Result, typed::ThreadListResponse>) {
            return value.data.size() == 1 && threadFieldsMatch(value.data.front()) && value.nextCursor.value &&
                   *value.nextCursor.value == "synthetic-9e08152d99" && value.backwardsCursor.value &&
                   *value.backwardsCursor.value == "synthetic-7261b2f3fa";
        } else if constexpr (std::is_same_v<Result, typed::ThreadLoadedListResponse>) {
            return value.data == std::vector<typed::ThreadId>{typed::ThreadId{"synthetic-48af340a73"}} && value.nextCursor.value &&
                   *value.nextCursor.value == "synthetic-9e08152d99";
        } else if constexpr (std::is_same_v<Result, typed::ThreadMetadataUpdateResponse> ||
                             std::is_same_v<Result, typed::ThreadReadResponse> || std::is_same_v<Result, typed::ThreadRollbackResponse> ||
                             std::is_same_v<Result, typed::ThreadUnarchiveResponse>) {
            return threadFieldsMatch(value.thread);
        } else if constexpr (std::is_same_v<Result, typed::ThreadUnsubscribeResponse>) {
            return value.status == typed::ThreadUnsubscribeStatus::notLoaded() && value.diagnostics.empty();
        } else if constexpr (std::is_same_v<Result, typed::TurnStartResponse>) {
            return turnFieldsMatch(value.turn);
        } else if constexpr (std::is_same_v<Result, typed::TurnSteerResponse>) {
            return value.turnId.value == TurnIdValue;
        } else {
            return false;
        }
    }

    enum class Phase { LocalRejection, Success, MalformedUnit, RemoteError, Cancellation, Generation };

    enum class LaunchResponseVariant { Omitted, Null, UnknownOpenEnums };

    struct OperationCase {
        std::string method;
        std::string slug;
        detail::ClientRequestTarget target = detail::ClientRequestTarget::Count;
        codex::Json expectedParams;
        codex::Json expectedResult;
        std::function<Submission(Phase)> invoke;
    };

    struct EncodingCase {
        std::string label;
        std::string method;
        codex::Json expectedParams;
        std::function<Submission()> invoke;
    };

    struct LaunchResponseCase {
        std::string label;
        std::string method;
        codex::Json expectedParams;
        codex::Json expectedResult;
        std::function<Submission()> invoke;
    };

    class OperationWireRunner {
    public:
        explicit OperationWireRunner(tests::support::TestResult& testResult)
            : testResult(testResult)
            , state(std::make_shared<UnixTransportState>()) {
            socketReady = state->open();
            if (socketReady) {
                client = std::make_unique<TestClient>(state);
                buildCases();
                buildEncodingCases();
                buildLaunchResponseCases();
            }
        }

        void start() {
            expect(socketReady && client != nullptr, "A1.1 operation harness creates its AF_UNIX socketpair");
            if (!socketReady || !client) {
                finished = true;
                core::SNodeC::stop();
                return;
            }
            expect(cases.size() == OperationCount, "A1.1 operation harness contains the exact 22 stable request methods");

            client->typed().events().setOnEvent([this](const typed::Event& event) {
                const typed::UnknownEvent* unknown = std::get_if<typed::UnknownEvent>(&event);
                if (unknown && unknown->method == "test/a1-1/raw-observer" && unknown->params.is_object() &&
                    unknown->params.contains("operation") && unknown->params["operation"].is_string()) {
                    const std::string operation = unknown->params["operation"];
                    ++typedObserverCounts[operation];
                    observerOrder.push_back("typed:" + operation);
                }
            });
            client->raw().setOnNotification([this](const codex::Notification& notification) {
                if (notification.method == "test/a1-1/raw-observer" && notification.params.is_object() &&
                    notification.params.contains("operation") && notification.params["operation"].is_string()) {
                    const std::string operation = notification.params["operation"];
                    ++rawObserverCounts[operation];
                    observerOrder.push_back("raw:" + operation);
                }
            });

            client->setOnStateChanged([this](const codex::StateChange& change) {
                if (change.current == codex::State::Ready) {
                    ++readyCount;
                    if (readyCount == 1) {
                        beginSuccess();
                    } else if (readyCount == 2) {
                        beginGeneration();
                    }
                } else if (change.current == codex::State::Failed) {
                    expect(false, "A1.1 operation socket transcript connection must not fail");
                    client->stop();
                } else if (change.current == codex::State::Stopped) {
                    if (readyCount == 1) {
                        firstStopObserved = true;
                        maybeRestart();
                    } else if (readyCount == 2) {
                        finished = true;
                        core::SNodeC::stop();
                    }
                }
            });

            invokeLocalRejections();
            client->start();
        }

        bool isFinished() const noexcept {
            return finished;
        }

    private:
        template <typename Result, typename Params, typename Submit>
        OperationCase
        makeOperation(std::string method, std::string slug, detail::ClientRequestTarget target, Params params, Submit submit) {
            const codex::Json expectedParams = readFixture(slug, "params");
            const codex::Json expectedResult = readFixture(slug, "result");
            state->successResults.emplace(method, expectedResult);
            return {
                method,
                slug,
                target,
                expectedParams,
                expectedResult,
                [this, method, expectedResult, params = std::move(params), submit = std::move(submit)](Phase phase) mutable {
                    return submit(params, resultHandler<Result>(method, expectedResult, phase));
                },
            };
        }

        template <typename Result>
        std::function<void(const typed::OperationResult<Result>&)>
        resultHandler(std::string method, codex::Json expectedResult, Phase phase) {
            return [this, method = std::move(method), expectedResult = std::move(expectedResult), phase](
                       const typed::OperationResult<Result>& result) {
                expect(!insideSubmission, method + " completion is asynchronous");
                expect(result.requestId.has_value(), method + " completion retains its allocated request ID");

                if (phase == Phase::LocalRejection) {
                    ++unexpectedLocalCallbacks;
                    expect(false, method + " synchronous local rejection must not invoke its handler");
                    return;
                }
                if (phase == Phase::Success || phase == Phase::Generation) {
                    expect(static_cast<bool>(result) && result.value.has_value(), method + " decodes its exact successful result type");
                    expect(result.value && resultFieldsMatch(*result.value),
                           method + " exposes field-level typed values for its exact successful result contract");
                    expect(result.raw == expectedResult, method + " retains the exact raw successful result");
                    if constexpr (!std::is_same_v<Result, typed::Unit>) {
                        expect(result.value && result.value->raw == expectedResult,
                               method + " retains the exact raw JSON on its typed result aggregate");
                    }
                    std::vector<std::string>& order = phase == Phase::Success ? successOrder : generationOrder;
                    order.push_back(method);
                    std::size_t& count = phase == Phase::Success ? successCallbacks : generationCallbacks;
                    ++count;
                    if (count == OperationCount) {
                        if (phase == Phase::Success) {
                            core::EventReceiver::atNextTick([this]() {
                                completeSuccess();
                            });
                        } else {
                            core::EventReceiver::atNextTick([this]() {
                                completeGeneration();
                            });
                        }
                    }
                    if (phase == Phase::Success) {
                        throw std::runtime_error("intentional " + method + " typed completion failure");
                    }
                    return;
                }
                if (phase == Phase::MalformedUnit) {
                    if constexpr (std::is_same_v<Result, typed::Unit>) {
                        const codex::Json malformedResult = {{"unexpected", true}};
                        expect(result.kind == typed::OperationResult<Result>::Kind::LocalError && !result.value && result.localError &&
                                   result.localError->category == codex::Error::Category::Protocol && result.localError->code == EINVAL &&
                                   result.localError->message == "Unit successful result must be the exact empty object" &&
                                   result.raw == malformedResult,
                               method + " rejects a non-empty successful result against the pinned Unit contract");
                        malformedUnitOrder.push_back(method);
                        if (++malformedUnitCallbacks == 7) {
                            core::EventReceiver::atNextTick([this]() {
                                completeMalformedUnitResults();
                            });
                        }
                    } else {
                        expect(false, method + " must not enter the Unit-result mutation phase");
                    }
                    return;
                }
                if (phase == Phase::RemoteError) {
                    const codex::Json expectedRawError = {
                        {"code", -32'222},
                        {"message", "synthetic remote failure"},
                        {"data", {{"operation", method}, {"retained", true}}},
                        {"futureErrorMetadata", {{"operation", method}, {"preserve", "exact"}}},
                    };
                    expect(result.kind == typed::OperationResult<Result>::Kind::RemoteError && !result.value && result.remoteError &&
                               result.remoteError->code == -32'222 && result.remoteError->message == "synthetic remote failure" &&
                               result.remoteError->data ==
                                   std::optional<codex::Json>(codex::Json{{"operation", method}, {"retained", true}}) &&
                               result.remoteError->raw == expectedRawError && result.raw.is_null(),
                           method + " retains the exact remote error, including unknown error-object fields");
                    remoteOrder.push_back(method);
                    if (++remoteCallbacks == OperationCount) {
                        core::EventReceiver::atNextTick([this]() {
                            completeRemoteErrors();
                        });
                    }
                    return;
                }

                expect(result.kind == typed::OperationResult<Result>::Kind::Cancelled && !result.value && result.localError &&
                           result.localError->category == codex::Error::Category::Cancelled,
                       method + " receives one typed cancellation at the connection boundary");
                cancellationOrder.push_back(method);
                if (++cancellationCallbacks == OperationCount) {
                    maybeRestart();
                }
            };
        }

        template <typename Result, typename Params, typename Submit>
        EncodingCase makeEncodingCase(
            std::string label, std::string method, std::string_view slug, Params params, codex::Json expectedParams, Submit submit) {
            const codex::Json expectedResult = readFixture(slug, "result");
            const std::string callbackLabel = label;
            const std::string callbackMethod = method;
            return {
                std::move(label),
                std::move(method),
                std::move(expectedParams),
                [this,
                 label = callbackLabel,
                 method = callbackMethod,
                 expectedResult,
                 params = std::move(params),
                 submit = std::move(submit)]() mutable {
                    return submit(params, [this, label, method, expectedResult](const typed::OperationResult<Result>& result) {
                        expect(!insideSubmission, label + " completion is asynchronous");
                        const bool typedRawRetained = [&]() {
                            if (!result.value) {
                                return false;
                            }
                            if constexpr (std::is_same_v<Result, typed::Unit>) {
                                return expectedResult == codex::Json::object();
                            } else if constexpr (std::is_same_v<Result, typed::Thread>) {
                                return expectedResult.contains("thread") && result.value->raw == expectedResult.at("thread");
                            } else {
                                return result.value->raw == expectedResult;
                            }
                        }();
                        expect(static_cast<bool>(result) && result.value && result.raw == expectedResult && typedRawRetained,
                               label + " decodes and retains its exact raw result");
                        encodingOrder.push_back(label);
                        if (++encodingCallbacks == encodingCases.size()) {
                            core::EventReceiver::atNextTick([this]() {
                                completeEncoding();
                            });
                        }
                    });
                },
            };
        }

        template <typename Result, typename Params, typename Submit>
        LaunchResponseCase makeLaunchResponseCase(
            std::string label, std::string method, std::string_view slug, LaunchResponseVariant variant, Params params, Submit submit) {
            codex::Json expectedResult = readFixture(slug, "result");
            if (variant == LaunchResponseVariant::Omitted) {
                expectedResult.erase("serviceTier");
                expectedResult.erase("reasoningEffort");
                expectedResult.erase("instructionSources");
            } else if (variant == LaunchResponseVariant::Null) {
                expectedResult["serviceTier"] = nullptr;
                expectedResult["reasoningEffort"] = nullptr;
            } else {
                expectedResult["approvalsReviewer"] = "synthetic-future-reviewer";
            }

            const codex::Json expectedParams = readFixture(slug, "params");
            const std::string callbackLabel = label;
            const std::string callbackMethod = method;
            return {
                std::move(label),
                std::move(method),
                expectedParams,
                expectedResult,
                [this,
                 label = callbackLabel,
                 method = callbackMethod,
                 expectedResult,
                 variant,
                 params = std::move(params),
                 submit = std::move(submit)]() mutable {
                    return submit(params, [this, label, method, expectedResult, variant](const typed::OperationResult<Result>& result) {
                        expect(!insideSubmission, label + " completion is asynchronous");
                        expect(static_cast<bool>(result) && result.value && result.raw == expectedResult &&
                                   result.value->raw == expectedResult && threadFieldsMatch(result.value->thread),
                               label + " decodes and retains its exact launch response");
                        if (result.value) {
                            const Result& response = *result.value;
                            if (variant == LaunchResponseVariant::Omitted) {
                                expect(!response.serviceTier.present && !response.reasoningEffort.present &&
                                           response.instructionSources.empty() && !response.raw.contains("instructionSources") &&
                                           response.diagnostics.empty(),
                                       label + " preserves omitted service-tier/reasoning-effort state and materializes the "
                                               "instructionSources schema default");
                            } else if (variant == LaunchResponseVariant::Null) {
                                expect(response.serviceTier.present && !response.serviceTier.value && response.reasoningEffort.present &&
                                           !response.reasoningEffort.value && response.diagnostics.empty(),
                                       label + " preserves explicit-null service-tier/reasoning-effort response state");
                            } else {
                                const bool exactDiagnostics =
                                    response.diagnostics.size() == 2 &&
                                    hasUnknownEnumDiagnostic(response.diagnostics, "ApprovalsReviewer", "$.approvalsReviewer") &&
                                    hasUnknownEnumDiagnostic(response.diagnostics, "ReasoningEffort", "$.reasoningEffort");
                                const bool diagnosticsRedacted =
                                    std::none_of(response.diagnostics.begin(),
                                                 response.diagnostics.end(),
                                                 [](const typed::DecodeDiagnostic& diagnostic) {
                                                     return diagnostic.message.find("synthetic-future-reviewer") != std::string::npos ||
                                                            diagnostic.message.find("synthetic-b43c9594ba") != std::string::npos;
                                                 });
                                expect(response.serviceTier.present && response.serviceTier.value &&
                                           *response.serviceTier.value == "synthetic-df688533b9" && response.reasoningEffort.present &&
                                           response.reasoningEffort.value &&
                                           response.reasoningEffort.value->value == "synthetic-b43c9594ba" &&
                                           response.approvalsReviewer.value == "synthetic-future-reviewer" &&
                                           !response.approvalsReviewer.isKnown() && !response.reasoningEffort.value->isKnown() &&
                                           exactDiagnostics && diagnosticsRedacted,
                                       label + " retains future open-enum values with the exact redacted diagnostics");
                            }
                        }
                        launchResponseOrder.push_back(label);
                        if (++launchResponseCallbacks == launchResponseCases.size()) {
                            core::EventReceiver::atNextTick([this]() {
                                completeLaunchResponseCases();
                            });
                        }
                    });
                },
            };
        }

        void expect(bool condition, const std::string& message) {
            testResult.expectTrue(condition, message);
        }

        std::vector<std::string> expectedMethods() const {
            std::vector<std::string> methods;
            methods.reserve(cases.size());
            for (const OperationCase& operation : cases) {
                methods.push_back(operation.method);
            }
            return methods;
        }

        std::vector<std::string> expectedUnitMethods() const {
            std::vector<std::string> methods;
            for (const OperationCase& operation : cases) {
                if (detail::entryFor(operation.target).operationContract.resultKind == detail::ResultContractKind::Unit) {
                    methods.push_back(operation.method);
                }
            }
            return methods;
        }

        void validateRegistryAndFixtures() {
            const std::vector<std::string_view> unitMethods = {
                "thread/archive",
                "thread/compact/start",
                "thread/delete",
                "thread/inject_items",
                "thread/name/set",
                "thread/shellCommand",
                "turn/interrupt",
            };
            std::size_t unitCount = 0;
            std::size_t concreteCount = 0;
            for (const OperationCase& operation : cases) {
                const detail::ProtocolSurfaceEntry& entry = detail::entryFor(operation.target);
                const auto* target = std::get_if<detail::ClientRequestTarget>(&entry.runtimeTarget);
                const bool expectedUnit = std::find(unitMethods.begin(), unitMethods.end(), operation.method) != unitMethods.end();
                if (entry.operationContract.resultKind == detail::ResultContractKind::Unit) {
                    ++unitCount;
                } else if (entry.operationContract.resultKind == detail::ResultContractKind::Concrete) {
                    ++concreteCount;
                }
                expect(entry.key.category == detail::SurfaceCategory::ClientRequest && entry.key.domain == "ClientRequest" &&
                           entry.key.field == "method" && entry.key.name == operation.method && target && *target == operation.target &&
                           entry.operationContract.resultKind ==
                               (expectedUnit ? detail::ResultContractKind::Unit : detail::ResultContractKind::Concrete),
                       operation.method + " begins from its exact canonical registry row and operation contract");
                expect(operation.expectedParams.is_object() && operation.expectedResult.is_object(),
                       operation.method + " loads independently generated params/result fixtures");
            }
            expect(unitCount == 7 && concreteCount == 15,
                   "the exact A1.1 operation registry ratchet remains seven Unit and fifteen Concrete results");
        }

        void invokeLocalRejections() {
            validateRegistryAndFixtures();
            const std::size_t before = state->outbound.size();
            for (OperationCase& operation : cases) {
                insideSubmission = true;
                const Submission submission = operation.invoke(Phase::LocalRejection);
                insideSubmission = false;
                expect(!submission && !submission.id && submission.error &&
                           submission.error->category == codex::Error::Category::InvalidState,
                       operation.method + " rejects synchronously before a ready connection");
            }
            expect(state->outbound.size() == before && unexpectedLocalCallbacks == 0,
                   "all 22 synchronous local rejections emit no bytes and invoke no callbacks");
        }

        void invokeBatch(Phase phase) {
            const std::size_t before = state->outbound.size();
            std::vector<std::optional<codex::ClientRequestId>> ids;
            ids.reserve(cases.size());
            for (OperationCase& operation : cases) {
                insideSubmission = true;
                Submission submission = operation.invoke(phase);
                insideSubmission = false;
                expect(static_cast<bool>(submission) && submission.id.has_value() && !submission.error,
                       operation.method + " is accepted by the shared RawProtocol request registry");
                ids.push_back(submission.id);
            }
            expect(state->outbound.size() == before + cases.size(),
                   "the operation batch emits exactly one client request per typed method");

            for (std::size_t index = 0; index < cases.size(); ++index) {
                const OperationCase& operation = cases[index];
                const OutboundRecord& record = state->outbound[before + index];
                const codex::Json expectedEnvelope = {
                    {"id", ids[index] ? codex::Json(ids[index]->value()) : codex::Json(nullptr)},
                    {"method", operation.method},
                    {"params", operation.expectedParams},
                };
                expect(record.envelope == expectedEnvelope && record.line == expectedEnvelope.dump() + "\n",
                       operation.method + " crosses AF_UNIX as exact canonical JSONL method and params bytes");
            }

            if (phase == Phase::Generation) {
                generationIds = std::move(ids);
            }
        }

        void beginSuccess() {
            state->replyMode = UnixTransportState::ReplyMode::Success;
            state->injectObservers = true;
            invokeBatch(Phase::Success);
        }

        void completeSuccess() {
            const std::vector<std::string> methods = expectedMethods();
            expect(successCallbacks == OperationCount && successOrder == methods,
                   "all 22 throwing success callbacks complete asynchronously in exact submission order");
            bool observerParity = observerOrder.size() == OperationCount * 2;
            for (std::size_t index = 0; index < methods.size(); ++index) {
                const std::string& method = methods[index];
                observerParity = observerParity && typedObserverCounts[method] == 1 && rawObserverCounts[method] == 1 &&
                                 observerOrder[index * 2] == "typed:" + method && observerOrder[index * 2 + 1] == "raw:" + method;
            }
            expect(observerParity, "every typed operation coexists with typed-before-raw observers after every throwing callback path");
            state->injectObservers = false;
            invokeEncodingCases();
        }

        void invokeEncodingCases() {
            state->replyMode = UnixTransportState::ReplyMode::Success;
            const std::size_t before = state->outbound.size();
            std::vector<std::optional<codex::ClientRequestId>> ids;
            ids.reserve(encodingCases.size());
            for (EncodingCase& encodingCase : encodingCases) {
                insideSubmission = true;
                Submission submission = encodingCase.invoke();
                insideSubmission = false;
                expect(static_cast<bool>(submission) && submission.id && !submission.error,
                       encodingCase.label + " is accepted by the shared RawProtocol engine");
                ids.push_back(submission.id);
            }
            expect(state->outbound.size() == before + encodingCases.size(),
                   "omitted/null encoding variants each emit exactly one typed request");
            for (std::size_t index = 0; index < encodingCases.size(); ++index) {
                const EncodingCase& encodingCase = encodingCases[index];
                const OutboundRecord& record = state->outbound[before + index];
                const codex::Json expectedEnvelope = {
                    {"id", ids[index] ? codex::Json(ids[index]->value()) : codex::Json(nullptr)},
                    {"method", encodingCase.method},
                    {"params", encodingCase.expectedParams},
                };
                expect(record.envelope == expectedEnvelope && record.line == expectedEnvelope.dump() + "\n",
                       encodingCase.label + " crosses AF_UNIX as exact omitted/null JSONL bytes");
            }
        }

        void completeEncoding() {
            std::vector<std::string> expectedOrder;
            expectedOrder.reserve(encodingCases.size());
            for (const EncodingCase& encodingCase : encodingCases) {
                expectedOrder.push_back(encodingCase.label);
            }
            expect(encodingCallbacks == encodingCases.size() && encodingOrder == expectedOrder,
                   "all schema-derived omitted/null parameter variants complete asynchronously in order");
            invokeMalformedUnitResults();
        }

        void invokeMalformedUnitResults() {
            state->replyMode = UnixTransportState::ReplyMode::Success;
            const codex::Json malformedResult = {{"unexpected", true}};
            const std::size_t before = state->outbound.size();
            std::vector<std::optional<codex::ClientRequestId>> ids;
            for (OperationCase& operation : cases) {
                if (detail::entryFor(operation.target).operationContract.resultKind != detail::ResultContractKind::Unit) {
                    continue;
                }
                state->successResults[operation.method] = malformedResult;
                insideSubmission = true;
                Submission submission = operation.invoke(Phase::MalformedUnit);
                insideSubmission = false;
                expect(static_cast<bool>(submission) && submission.id && !submission.error,
                       operation.method + " accepts the malformed-Unit response mutation through the shared engine");
                ids.push_back(submission.id);
            }

            const std::vector<std::string> methods = expectedUnitMethods();
            expect(methods.size() == 7 && ids.size() == methods.size() && state->outbound.size() == before + methods.size(),
                   "the Unit-response mutation reaches exactly the seven reviewed Unit contracts");
            for (std::size_t index = 0; index < methods.size(); ++index) {
                const auto operation = std::find_if(cases.begin(), cases.end(), [&](const OperationCase& candidate) {
                    return candidate.method == methods[index];
                });
                const codex::Json expectedEnvelope = {
                    {"id", ids[index] ? codex::Json(ids[index]->value()) : codex::Json(nullptr)},
                    {"method", methods[index]},
                    {"params", operation != cases.end() ? operation->expectedParams : codex::Json(nullptr)},
                };
                const OutboundRecord& record = state->outbound[before + index];
                expect(record.envelope == expectedEnvelope && record.line == expectedEnvelope.dump() + "\n",
                       methods[index] + " Unit mutation crosses AF_UNIX with unchanged exact request bytes");
            }
        }

        void completeMalformedUnitResults() {
            expect(malformedUnitCallbacks == 7 && malformedUnitOrder == expectedUnitMethods(),
                   "all seven non-empty Unit responses fail locally in exact submission order");
            for (const OperationCase& operation : cases) {
                state->successResults[operation.method] = operation.expectedResult;
            }
            invokeLaunchResponseCases();
        }

        void invokeLaunchResponseCases() {
            state->replyMode = UnixTransportState::ReplyMode::Success;
            const std::size_t before = state->outbound.size();
            std::vector<std::optional<codex::ClientRequestId>> ids;
            ids.reserve(launchResponseCases.size());
            for (LaunchResponseCase& responseCase : launchResponseCases) {
                state->successResults[responseCase.method] = responseCase.expectedResult;
                insideSubmission = true;
                Submission submission = responseCase.invoke();
                insideSubmission = false;
                expect(static_cast<bool>(submission) && submission.id && !submission.error,
                       responseCase.label + " is accepted by the shared RawProtocol engine");
                ids.push_back(submission.id);
            }
            expect(launchResponseCases.size() == 9 && state->outbound.size() == before + launchResponseCases.size(),
                   "all three launch roots exercise omitted, null, and open-enum value responses");
            for (std::size_t index = 0; index < launchResponseCases.size(); ++index) {
                const LaunchResponseCase& responseCase = launchResponseCases[index];
                const codex::Json expectedEnvelope = {
                    {"id", ids[index] ? codex::Json(ids[index]->value()) : codex::Json(nullptr)},
                    {"method", responseCase.method},
                    {"params", responseCase.expectedParams},
                };
                const OutboundRecord& record = state->outbound[before + index];
                expect(record.envelope == expectedEnvelope && record.line == expectedEnvelope.dump() + "\n",
                       responseCase.label + " crosses AF_UNIX with its unchanged exact request bytes");
            }
        }

        void completeLaunchResponseCases() {
            std::vector<std::string> expectedOrder;
            expectedOrder.reserve(launchResponseCases.size());
            for (const LaunchResponseCase& responseCase : launchResponseCases) {
                expectedOrder.push_back(responseCase.label);
            }
            expect(launchResponseCallbacks == launchResponseCases.size() && launchResponseOrder == expectedOrder,
                   "all launch-root response variants complete asynchronously in exact submission order");
            for (const OperationCase& operation : cases) {
                state->successResults[operation.method] = operation.expectedResult;
            }
            state->replyMode = UnixTransportState::ReplyMode::RemoteError;
            invokeBatch(Phase::RemoteError);
        }

        void completeRemoteErrors() {
            expect(remoteCallbacks == OperationCount && remoteOrder == expectedMethods(),
                   "all 22 raw-retaining remote-error callbacks complete asynchronously in submission order");
            state->replyMode = UnixTransportState::ReplyMode::Hold;
            invokeBatch(Phase::Cancellation);
            client->stop();
        }

        void maybeRestart() {
            if (!restartScheduled && firstStopObserved && cancellationCallbacks == OperationCount) {
                restartScheduled = true;
                expect(cancellationOrder == expectedMethods(), "all 22 held operations cancel once in exact submission order");
                core::EventReceiver::atNextTick([this]() {
                    client->start();
                });
            }
        }

        void beginGeneration() {
            state->replyMode = UnixTransportState::ReplyMode::Hold;
            state->injectObservers = false;
            invokeBatch(Phase::Generation);
            expect(state->callbackGenerations.size() >= 2, "restart installs a distinct transport callback generation");
            if (state->callbackGenerations.size() < 2) {
                client->stop();
                return;
            }

            const detail::TransportCallbacks stale = state->callbackGenerations.front();
            for (std::size_t index = 0; index < cases.size(); ++index) {
                if (generationIds[index]) {
                    expect(state->injectWith(stale, {{"id", generationIds[index]->value()}, {"result", cases[index].expectedResult}}),
                           cases[index].method + " stale-generation response crosses the Unix socket");
                }
            }
            core::EventReceiver::atNextTick([this]() {
                expect(generationCallbacks == 0, "all 22 stale-generation responses are ignored before current-generation delivery");
                const detail::TransportCallbacks current = state->callbackGenerations.back();
                for (std::size_t index = 0; index < cases.size(); ++index) {
                    if (generationIds[index]) {
                        expect(state->injectWith(current, {{"id", generationIds[index]->value()}, {"result", cases[index].expectedResult}}),
                               cases[index].method + " current-generation response crosses the Unix socket");
                    }
                }
            });
        }

        void completeGeneration() {
            expect(generationCallbacks == OperationCount && generationOrder == expectedMethods(),
                   "all 22 current-generation responses complete once and in callback order");
            client->stop();
        }

        void buildCases() {
            auto& threads = client->typed().threads();
            auto& turns = client->typed().turns();

            cases.push_back(makeOperation<typed::Unit>("thread/archive",
                                                       "thread-archive",
                                                       detail::ClientRequestTarget::ThreadArchive,
                                                       typed::ThreadArchiveParams{threadId()},
                                                       [&threads](auto params, auto handler) {
                                                           return threads.archive(std::move(params), std::move(handler));
                                                       }));
            cases.push_back(makeOperation<typed::Unit>("thread/compact/start",
                                                       "thread-compact-start",
                                                       detail::ClientRequestTarget::ThreadCompactStart,
                                                       typed::ThreadCompactStartParams{threadId()},
                                                       [&threads](auto params, auto handler) {
                                                           return threads.startCompaction(std::move(params), std::move(handler));
                                                       }));
            cases.push_back(makeOperation<typed::Unit>("thread/delete",
                                                       "thread-delete",
                                                       detail::ClientRequestTarget::ThreadDelete,
                                                       typed::ThreadDeleteParams{threadId()},
                                                       [&threads](auto params, auto handler) {
                                                           return threads.remove(std::move(params), std::move(handler));
                                                       }));
            cases.push_back(makeOperation<typed::ThreadForkResponse>("thread/fork",
                                                                     "thread-fork",
                                                                     detail::ClientRequestTarget::ThreadFork,
                                                                     threadForkParams(),
                                                                     [&threads](auto params, auto handler) {
                                                                         return threads.fork(std::move(params), std::move(handler));
                                                                     }));
            cases.push_back(makeOperation<typed::ThreadGoalClearResponse>("thread/goal/clear",
                                                                          "thread-goal-clear",
                                                                          detail::ClientRequestTarget::ThreadGoalClear,
                                                                          typed::ThreadGoalClearParams{threadId()},
                                                                          [&threads](auto params, auto handler) {
                                                                              return threads.clearGoal(std::move(params),
                                                                                                       std::move(handler));
                                                                          }));
            cases.push_back(makeOperation<typed::ThreadGoalGetResponse>("thread/goal/get",
                                                                        "thread-goal-get",
                                                                        detail::ClientRequestTarget::ThreadGoalGet,
                                                                        typed::ThreadGoalGetParams{threadId()},
                                                                        [&threads](auto params, auto handler) {
                                                                            return threads.getGoal(std::move(params), std::move(handler));
                                                                        }));
            cases.push_back(makeOperation<typed::ThreadGoalSetResponse>("thread/goal/set",
                                                                        "thread-goal-set",
                                                                        detail::ClientRequestTarget::ThreadGoalSet,
                                                                        threadGoalSetParams(),
                                                                        [&threads](auto params, auto handler) {
                                                                            return threads.setGoal(std::move(params), std::move(handler));
                                                                        }));
            cases.push_back(makeOperation<typed::Unit>("thread/inject_items",
                                                       "thread-inject_items",
                                                       detail::ClientRequestTarget::ThreadInjectItems,
                                                       typed::ThreadInjectItemsParams{threadId(), {codex::Json::object()}},
                                                       [&threads](auto params, auto handler) {
                                                           return threads.injectItems(std::move(params), std::move(handler));
                                                       }));
            cases.push_back(makeOperation<typed::ThreadListResponse>("thread/list",
                                                                     "thread-list",
                                                                     detail::ClientRequestTarget::ThreadList,
                                                                     threadListParams(),
                                                                     [&threads](auto params, auto handler) {
                                                                         return threads.list(std::move(params), std::move(handler));
                                                                     }));
            cases.push_back(makeOperation<typed::ThreadLoadedListResponse>("thread/loaded/list",
                                                                           "thread-loaded-list",
                                                                           detail::ClientRequestTarget::ThreadLoadedList,
                                                                           threadLoadedListParams(),
                                                                           [&threads](auto params, auto handler) {
                                                                               return threads.listLoaded(std::move(params),
                                                                                                         std::move(handler));
                                                                           }));
            cases.push_back(makeOperation<typed::ThreadMetadataUpdateResponse>("thread/metadata/update",
                                                                               "thread-metadata-update",
                                                                               detail::ClientRequestTarget::ThreadMetadataUpdate,
                                                                               threadMetadataUpdateParams(),
                                                                               [&threads](auto params, auto handler) {
                                                                                   return threads.updateMetadata(std::move(params),
                                                                                                                 std::move(handler));
                                                                               }));
            cases.push_back(makeOperation<typed::Unit>("thread/name/set",
                                                       "thread-name-set",
                                                       detail::ClientRequestTarget::ThreadSetName,
                                                       typed::ThreadSetNameParams{threadId(), "synthetic-17626d1028"},
                                                       [&threads](auto params, auto handler) {
                                                           return threads.setName(std::move(params), std::move(handler));
                                                       }));
            cases.push_back(makeOperation<typed::ThreadReadResponse>("thread/read",
                                                                     "thread-read",
                                                                     detail::ClientRequestTarget::ThreadRead,
                                                                     typed::ThreadReadParams{threadId(), false},
                                                                     [&threads](auto params, auto handler) {
                                                                         return threads.read(std::move(params), std::move(handler));
                                                                     }));
            cases.push_back(makeOperation<typed::ThreadResumeResponse>("thread/resume",
                                                                       "thread-resume",
                                                                       detail::ClientRequestTarget::ThreadResume,
                                                                       threadResumeParams(),
                                                                       [&threads](auto params, auto handler) {
                                                                           return threads.resume(std::move(params), std::move(handler));
                                                                       }));
            cases.push_back(makeOperation<typed::ThreadRollbackResponse>("thread/rollback",
                                                                         "thread-rollback",
                                                                         detail::ClientRequestTarget::ThreadRollback,
                                                                         typed::ThreadRollbackParams{threadId(), 0},
                                                                         [&threads](auto params, auto handler) {
                                                                             return threads.rollback(std::move(params), std::move(handler));
                                                                         }));
            cases.push_back(makeOperation<typed::Unit>("thread/shellCommand",
                                                       "thread-shellcommand",
                                                       detail::ClientRequestTarget::ThreadShellCommand,
                                                       typed::ThreadShellCommandParams{threadId(), "synthetic-1d4a1a5061"},
                                                       [&threads](auto params, auto handler) {
                                                           return threads.shellCommand(std::move(params), std::move(handler));
                                                       }));
            cases.push_back(makeOperation<typed::ThreadStartResponse>("thread/start",
                                                                      "thread-start",
                                                                      detail::ClientRequestTarget::ThreadStart,
                                                                      threadStartParams(),
                                                                      [&threads](auto params, auto handler) {
                                                                          return threads.start(std::move(params), std::move(handler));
                                                                      }));
            cases.push_back(makeOperation<typed::ThreadUnarchiveResponse>("thread/unarchive",
                                                                          "thread-unarchive",
                                                                          detail::ClientRequestTarget::ThreadUnarchive,
                                                                          typed::ThreadUnarchiveParams{threadId()},
                                                                          [&threads](auto params, auto handler) {
                                                                              return threads.unarchive(std::move(params),
                                                                                                       std::move(handler));
                                                                          }));
            cases.push_back(makeOperation<typed::ThreadUnsubscribeResponse>("thread/unsubscribe",
                                                                            "thread-unsubscribe",
                                                                            detail::ClientRequestTarget::ThreadUnsubscribe,
                                                                            typed::ThreadUnsubscribeParams{threadId()},
                                                                            [&threads](auto params, auto handler) {
                                                                                return threads.unsubscribe(std::move(params),
                                                                                                           std::move(handler));
                                                                            }));
            cases.push_back(makeOperation<typed::Unit>("turn/interrupt",
                                                       "turn-interrupt",
                                                       detail::ClientRequestTarget::TurnInterrupt,
                                                       typed::TurnInterruptParams{threadId(), turnId()},
                                                       [&turns](auto params, auto handler) {
                                                           return turns.interrupt(std::move(params), std::move(handler));
                                                       }));
            cases.push_back(makeOperation<typed::TurnStartResponse>(
                "turn/start", "turn-start", detail::ClientRequestTarget::TurnStart, turnStartParams(), [&turns](auto params, auto handler) {
                    return turns.start(std::move(params), std::move(handler));
                }));
            cases.push_back(makeOperation<typed::TurnSteerResponse>(
                "turn/steer", "turn-steer", detail::ClientRequestTarget::TurnSteer, turnSteerParams(), [&turns](auto params, auto handler) {
                    return turns.steer(std::move(params), std::move(handler));
                }));
        }

        void buildEncodingCases() {
            auto& threads = client->typed().threads();
            auto& turns = client->typed().turns();
            const codex::Json nontrivialOpaque = {
                {"nested", {{"enabled", true}, {"values", codex::Json::array({1, "two", nullptr, codex::Json{{"leaf", 3}}})}}},
            };

            typed::ThreadForkParams forkOmitted;
            forkOmitted.threadId = threadId();
            encodingCases.push_back(makeEncodingCase<typed::ThreadForkResponse>("thread/fork:omitted",
                                                                                "thread/fork",
                                                                                "thread-fork",
                                                                                std::move(forkOmitted),
                                                                                {{"threadId", ThreadIdValue}},
                                                                                [&threads](auto params, auto handler) {
                                                                                    return threads.fork(std::move(params),
                                                                                                        std::move(handler));
                                                                                }));
            typed::ThreadForkParams forkNull;
            forkNull.threadId = threadId();
            forkNull.approvalPolicy = decltype(forkNull.approvalPolicy)::explicitNull();
            forkNull.approvalsReviewer = decltype(forkNull.approvalsReviewer)::explicitNull();
            forkNull.baseInstructions = decltype(forkNull.baseInstructions)::explicitNull();
            forkNull.config = decltype(forkNull.config)::explicitNull();
            forkNull.cwd = decltype(forkNull.cwd)::explicitNull();
            forkNull.developerInstructions = decltype(forkNull.developerInstructions)::explicitNull();
            forkNull.serviceTier = decltype(forkNull.serviceTier)::explicitNull();
            forkNull.lastTurnId = decltype(forkNull.lastTurnId)::explicitNull();
            forkNull.model = decltype(forkNull.model)::explicitNull();
            forkNull.modelProvider = decltype(forkNull.modelProvider)::explicitNull();
            forkNull.threadSource = decltype(forkNull.threadSource)::explicitNull();
            forkNull.sandbox = decltype(forkNull.sandbox)::explicitNull();
            encodingCases.push_back(makeEncodingCase<typed::ThreadForkResponse>("thread/fork:null",
                                                                                "thread/fork",
                                                                                "thread-fork",
                                                                                std::move(forkNull),
                                                                                {{"approvalPolicy", nullptr},
                                                                                 {"approvalsReviewer", nullptr},
                                                                                 {"baseInstructions", nullptr},
                                                                                 {"config", nullptr},
                                                                                 {"cwd", nullptr},
                                                                                 {"developerInstructions", nullptr},
                                                                                 {"lastTurnId", nullptr},
                                                                                 {"model", nullptr},
                                                                                 {"modelProvider", nullptr},
                                                                                 {"sandbox", nullptr},
                                                                                 {"serviceTier", nullptr},
                                                                                 {"threadId", ThreadIdValue},
                                                                                 {"threadSource", nullptr}},
                                                                                [&threads](auto params, auto handler) {
                                                                                    return threads.fork(std::move(params),
                                                                                                        std::move(handler));
                                                                                }));
            typed::ThreadForkParams forkOpaque;
            forkOpaque.threadId = threadId();
            forkOpaque.config = typed::ProtocolConfiguration{{"opaque", nontrivialOpaque}};
            encodingCases.push_back(
                makeEncodingCase<typed::ThreadForkResponse>("thread/fork:opaque-config",
                                                            "thread/fork",
                                                            "thread-fork",
                                                            std::move(forkOpaque),
                                                            {{"config", {{"opaque", nontrivialOpaque}}}, {"threadId", ThreadIdValue}},
                                                            [&threads](auto params, auto handler) {
                                                                return threads.fork(std::move(params), std::move(handler));
                                                            }));

            encodingCases.push_back(
                makeEncodingCase<typed::Unit>("thread/inject_items:opaque-nested-item",
                                              "thread/inject_items",
                                              "thread-inject_items",
                                              typed::ThreadInjectItemsParams{threadId(), {nontrivialOpaque}},
                                              {{"items", codex::Json::array({nontrivialOpaque})}, {"threadId", ThreadIdValue}},
                                              [&threads](auto params, auto handler) {
                                                  return threads.injectItems(std::move(params), std::move(handler));
                                              }));

            encodingCases.push_back(makeEncodingCase<typed::ThreadGoalSetResponse>("thread/goal/set:omitted",
                                                                                   "thread/goal/set",
                                                                                   "thread-goal-set",
                                                                                   typed::ThreadGoalSetParams{threadId()},
                                                                                   {{"threadId", ThreadIdValue}},
                                                                                   [&threads](auto params, auto handler) {
                                                                                       return threads.setGoal(std::move(params),
                                                                                                              std::move(handler));
                                                                                   }));
            typed::ThreadGoalSetParams goalNull{threadId()};
            goalNull.objective = decltype(goalNull.objective)::explicitNull();
            goalNull.status = decltype(goalNull.status)::explicitNull();
            goalNull.tokenBudget = decltype(goalNull.tokenBudget)::explicitNull();
            encodingCases.push_back(makeEncodingCase<typed::ThreadGoalSetResponse>(
                "thread/goal/set:null",
                "thread/goal/set",
                "thread-goal-set",
                std::move(goalNull),
                {{"objective", nullptr}, {"status", nullptr}, {"threadId", ThreadIdValue}, {"tokenBudget", nullptr}},
                [&threads](auto params, auto handler) {
                    return threads.setGoal(std::move(params), std::move(handler));
                }));

            encodingCases.push_back(makeEncodingCase<typed::ThreadListResponse>("thread/list:omitted",
                                                                                "thread/list",
                                                                                "thread-list",
                                                                                typed::ThreadListParams{},
                                                                                codex::Json::object(),
                                                                                [&threads](auto params, auto handler) {
                                                                                    return threads.list(std::move(params),
                                                                                                        std::move(handler));
                                                                                }));
            typed::ThreadListParams listNull;
            listNull.sourceKinds = decltype(listNull.sourceKinds)::explicitNull();
            listNull.archived = decltype(listNull.archived)::explicitNull();
            listNull.cursor = decltype(listNull.cursor)::explicitNull();
            listNull.cwd = decltype(listNull.cwd)::explicitNull();
            listNull.limit = decltype(listNull.limit)::explicitNull();
            listNull.modelProviders = decltype(listNull.modelProviders)::explicitNull();
            listNull.searchTerm = decltype(listNull.searchTerm)::explicitNull();
            listNull.sortDirection = decltype(listNull.sortDirection)::explicitNull();
            listNull.sortKey = decltype(listNull.sortKey)::explicitNull();
            encodingCases.push_back(makeEncodingCase<typed::ThreadListResponse>("thread/list:null",
                                                                                "thread/list",
                                                                                "thread-list",
                                                                                std::move(listNull),
                                                                                {{"archived", nullptr},
                                                                                 {"cursor", nullptr},
                                                                                 {"cwd", nullptr},
                                                                                 {"limit", nullptr},
                                                                                 {"modelProviders", nullptr},
                                                                                 {"searchTerm", nullptr},
                                                                                 {"sortDirection", nullptr},
                                                                                 {"sortKey", nullptr},
                                                                                 {"sourceKinds", nullptr}},
                                                                                [&threads](auto params, auto handler) {
                                                                                    return threads.list(std::move(params),
                                                                                                        std::move(handler));
                                                                                }));
            typed::ThreadListParams listCwdVector;
            listCwdVector.cwd = typed::ThreadListCwdFilter{std::vector<std::string>{"/workspace/a", "/workspace/b"}};
            encodingCases.push_back(
                makeEncodingCase<typed::ThreadListResponse>("thread/list:cwd-vector-nonempty",
                                                            "thread/list",
                                                            "thread-list",
                                                            std::move(listCwdVector),
                                                            {{"cwd", codex::Json::array({"/workspace/a", "/workspace/b"})}},
                                                            [&threads](auto params, auto handler) {
                                                                return threads.list(std::move(params), std::move(handler));
                                                            }));
            typed::ThreadListParams listEmptyCwdVector;
            listEmptyCwdVector.cwd = typed::ThreadListCwdFilter{std::vector<std::string>{}};
            encodingCases.push_back(makeEncodingCase<typed::ThreadListResponse>("thread/list:cwd-vector-empty",
                                                                                "thread/list",
                                                                                "thread-list",
                                                                                std::move(listEmptyCwdVector),
                                                                                {{"cwd", codex::Json::array()}},
                                                                                [&threads](auto params, auto handler) {
                                                                                    return threads.list(std::move(params),
                                                                                                        std::move(handler));
                                                                                }));

            encodingCases.push_back(makeEncodingCase<typed::ThreadLoadedListResponse>("thread/loaded/list:omitted",
                                                                                      "thread/loaded/list",
                                                                                      "thread-loaded-list",
                                                                                      typed::ThreadLoadedListParams{},
                                                                                      codex::Json::object(),
                                                                                      [&threads](auto params, auto handler) {
                                                                                          return threads.listLoaded(std::move(params),
                                                                                                                    std::move(handler));
                                                                                      }));
            typed::ThreadLoadedListParams loadedNull;
            loadedNull.cursor = decltype(loadedNull.cursor)::explicitNull();
            loadedNull.limit = decltype(loadedNull.limit)::explicitNull();
            encodingCases.push_back(makeEncodingCase<typed::ThreadLoadedListResponse>("thread/loaded/list:null",
                                                                                      "thread/loaded/list",
                                                                                      "thread-loaded-list",
                                                                                      std::move(loadedNull),
                                                                                      {{"cursor", nullptr}, {"limit", nullptr}},
                                                                                      [&threads](auto params, auto handler) {
                                                                                          return threads.listLoaded(std::move(params),
                                                                                                                    std::move(handler));
                                                                                      }));

            encodingCases.push_back(makeEncodingCase<typed::ThreadMetadataUpdateResponse>("thread/metadata/update:omitted",
                                                                                          "thread/metadata/update",
                                                                                          "thread-metadata-update",
                                                                                          typed::ThreadMetadataUpdateParams{threadId()},
                                                                                          {{"threadId", ThreadIdValue}},
                                                                                          [&threads](auto params, auto handler) {
                                                                                              return threads.updateMetadata(
                                                                                                  std::move(params), std::move(handler));
                                                                                          }));
            typed::ThreadMetadataUpdateParams metadataNull{threadId()};
            metadataNull.gitInfo = decltype(metadataNull.gitInfo)::explicitNull();
            encodingCases.push_back(makeEncodingCase<typed::ThreadMetadataUpdateResponse>(
                "thread/metadata/update:null",
                "thread/metadata/update",
                "thread-metadata-update",
                std::move(metadataNull),
                {{"gitInfo", nullptr}, {"threadId", ThreadIdValue}},
                [&threads](auto params, auto handler) {
                    return threads.updateMetadata(std::move(params), std::move(handler));
                }));
            typed::ThreadMetadataUpdateParams metadataNestedOmitted{threadId()};
            metadataNestedOmitted.gitInfo = typed::ThreadMetadataGitInfoUpdateParams{};
            encodingCases.push_back(makeEncodingCase<typed::ThreadMetadataUpdateResponse>(
                "thread/metadata/update:gitinfo-members-omitted",
                "thread/metadata/update",
                "thread-metadata-update",
                std::move(metadataNestedOmitted),
                {{"gitInfo", codex::Json::object()}, {"threadId", ThreadIdValue}},
                [&threads](auto params, auto handler) {
                    return threads.updateMetadata(std::move(params), std::move(handler));
                }));
            typed::ThreadMetadataGitInfoUpdateParams nullGitInfo;
            nullGitInfo.branch = decltype(nullGitInfo.branch)::explicitNull();
            nullGitInfo.originUrl = decltype(nullGitInfo.originUrl)::explicitNull();
            nullGitInfo.sha = decltype(nullGitInfo.sha)::explicitNull();
            typed::ThreadMetadataUpdateParams metadataNestedNull{threadId()};
            metadataNestedNull.gitInfo = std::move(nullGitInfo);
            encodingCases.push_back(makeEncodingCase<typed::ThreadMetadataUpdateResponse>(
                "thread/metadata/update:gitinfo-members-null",
                "thread/metadata/update",
                "thread-metadata-update",
                std::move(metadataNestedNull),
                {{"gitInfo", {{"branch", nullptr}, {"originUrl", nullptr}, {"sha", nullptr}}}, {"threadId", ThreadIdValue}},
                [&threads](auto params, auto handler) {
                    return threads.updateMetadata(std::move(params), std::move(handler));
                }));
            encodingCases.push_back(makeEncodingCase<typed::ThreadMetadataUpdateResponse>("thread/metadata/update:gitinfo-members-value",
                                                                                          "thread/metadata/update",
                                                                                          "thread-metadata-update",
                                                                                          threadMetadataUpdateParams(),
                                                                                          readFixture("thread-metadata-update", "params"),
                                                                                          [&threads](auto params, auto handler) {
                                                                                              return threads.updateMetadata(
                                                                                                  std::move(params), std::move(handler));
                                                                                          }));

            encodingCases.push_back(makeEncodingCase<typed::ThreadReadResponse>("thread/read:omitted",
                                                                                "thread/read",
                                                                                "thread-read",
                                                                                typed::ThreadReadParams{threadId(), std::nullopt},
                                                                                {{"threadId", ThreadIdValue}},
                                                                                [&threads](auto params, auto handler) {
                                                                                    return threads.read(std::move(params),
                                                                                                        std::move(handler));
                                                                                }));

            typed::ThreadResumeParams resumeOmitted;
            resumeOmitted.threadId = threadId();
            encodingCases.push_back(makeEncodingCase<typed::ThreadResumeResponse>("thread/resume:omitted",
                                                                                  "thread/resume",
                                                                                  "thread-resume",
                                                                                  std::move(resumeOmitted),
                                                                                  {{"threadId", ThreadIdValue}},
                                                                                  [&threads](auto params, auto handler) {
                                                                                      return threads.resume(std::move(params),
                                                                                                            std::move(handler));
                                                                                  }));
            typed::ThreadResumeParams resumeNull;
            resumeNull.threadId = threadId();
            resumeNull.approvalPolicy = decltype(resumeNull.approvalPolicy)::explicitNull();
            resumeNull.approvalsReviewer = decltype(resumeNull.approvalsReviewer)::explicitNull();
            resumeNull.baseInstructions = decltype(resumeNull.baseInstructions)::explicitNull();
            resumeNull.config = decltype(resumeNull.config)::explicitNull();
            resumeNull.cwd = decltype(resumeNull.cwd)::explicitNull();
            resumeNull.developerInstructions = decltype(resumeNull.developerInstructions)::explicitNull();
            resumeNull.personality = decltype(resumeNull.personality)::explicitNull();
            resumeNull.serviceTier = decltype(resumeNull.serviceTier)::explicitNull();
            resumeNull.model = decltype(resumeNull.model)::explicitNull();
            resumeNull.modelProvider = decltype(resumeNull.modelProvider)::explicitNull();
            resumeNull.sandbox = decltype(resumeNull.sandbox)::explicitNull();
            encodingCases.push_back(makeEncodingCase<typed::ThreadResumeResponse>("thread/resume:null",
                                                                                  "thread/resume",
                                                                                  "thread-resume",
                                                                                  std::move(resumeNull),
                                                                                  {{"approvalPolicy", nullptr},
                                                                                   {"approvalsReviewer", nullptr},
                                                                                   {"baseInstructions", nullptr},
                                                                                   {"config", nullptr},
                                                                                   {"cwd", nullptr},
                                                                                   {"developerInstructions", nullptr},
                                                                                   {"model", nullptr},
                                                                                   {"modelProvider", nullptr},
                                                                                   {"personality", nullptr},
                                                                                   {"sandbox", nullptr},
                                                                                   {"serviceTier", nullptr},
                                                                                   {"threadId", ThreadIdValue}},
                                                                                  [&threads](auto params, auto handler) {
                                                                                      return threads.resume(std::move(params),
                                                                                                            std::move(handler));
                                                                                  }));
            typed::ThreadResumeParams resumeOpaque;
            resumeOpaque.threadId = threadId();
            resumeOpaque.config = typed::ProtocolConfiguration{{"opaque", nontrivialOpaque}};
            encodingCases.push_back(
                makeEncodingCase<typed::ThreadResumeResponse>("thread/resume:opaque-config",
                                                              "thread/resume",
                                                              "thread-resume",
                                                              std::move(resumeOpaque),
                                                              {{"config", {{"opaque", nontrivialOpaque}}}, {"threadId", ThreadIdValue}},
                                                              [&threads](auto params, auto handler) {
                                                                  return threads.resume(std::move(params), std::move(handler));
                                                              }));

            encodingCases.push_back(makeEncodingCase<typed::ThreadStartResponse>("thread/start:omitted",
                                                                                 "thread/start",
                                                                                 "thread-start",
                                                                                 typed::ThreadStartParams{},
                                                                                 codex::Json::object(),
                                                                                 [&threads](auto params, auto handler) {
                                                                                     return threads.start(std::move(params),
                                                                                                          std::move(handler));
                                                                                 }));
            typed::ThreadStartOptions compatibilityStart;
            compatibilityStart.cwd = "/compatibility-start";
            compatibilityStart.sandboxMode = typed::SandboxMode::workspaceWrite();
            compatibilityStart.ephemeral = true;
            encodingCases.push_back(
                makeEncodingCase<typed::Thread>("thread/start:compatibility-options",
                                                "thread/start",
                                                "thread-start",
                                                std::move(compatibilityStart),
                                                {{"cwd", "/compatibility-start"}, {"ephemeral", true}, {"sandbox", "workspace-write"}},
                                                [&threads](auto options, auto handler) {
                                                    return threads.start(std::move(options), std::move(handler));
                                                }));
            typed::ThreadStartParams startNull;
            startNull.sessionStartSource = decltype(startNull.sessionStartSource)::explicitNull();
            startNull.approvalPolicy = decltype(startNull.approvalPolicy)::explicitNull();
            startNull.approvalsReviewer = decltype(startNull.approvalsReviewer)::explicitNull();
            startNull.baseInstructions = decltype(startNull.baseInstructions)::explicitNull();
            startNull.config = decltype(startNull.config)::explicitNull();
            startNull.cwd = decltype(startNull.cwd)::explicitNull();
            startNull.developerInstructions = decltype(startNull.developerInstructions)::explicitNull();
            startNull.serviceName = decltype(startNull.serviceName)::explicitNull();
            startNull.personality = decltype(startNull.personality)::explicitNull();
            startNull.ephemeral = decltype(startNull.ephemeral)::explicitNull();
            startNull.threadSource = decltype(startNull.threadSource)::explicitNull();
            startNull.sandbox = decltype(startNull.sandbox)::explicitNull();
            startNull.serviceTier = decltype(startNull.serviceTier)::explicitNull();
            startNull.model = decltype(startNull.model)::explicitNull();
            startNull.modelProvider = decltype(startNull.modelProvider)::explicitNull();
            encodingCases.push_back(makeEncodingCase<typed::ThreadStartResponse>("thread/start:null",
                                                                                 "thread/start",
                                                                                 "thread-start",
                                                                                 std::move(startNull),
                                                                                 {{"approvalPolicy", nullptr},
                                                                                  {"approvalsReviewer", nullptr},
                                                                                  {"baseInstructions", nullptr},
                                                                                  {"config", nullptr},
                                                                                  {"cwd", nullptr},
                                                                                  {"developerInstructions", nullptr},
                                                                                  {"ephemeral", nullptr},
                                                                                  {"model", nullptr},
                                                                                  {"modelProvider", nullptr},
                                                                                  {"personality", nullptr},
                                                                                  {"sandbox", nullptr},
                                                                                  {"serviceName", nullptr},
                                                                                  {"serviceTier", nullptr},
                                                                                  {"sessionStartSource", nullptr},
                                                                                  {"threadSource", nullptr}},
                                                                                 [&threads](auto params, auto handler) {
                                                                                     return threads.start(std::move(params),
                                                                                                          std::move(handler));
                                                                                 }));
            typed::ThreadStartParams startOpaque;
            startOpaque.config = typed::ProtocolConfiguration{{"opaque", nontrivialOpaque}};
            encodingCases.push_back(makeEncodingCase<typed::ThreadStartResponse>("thread/start:opaque-config",
                                                                                 "thread/start",
                                                                                 "thread-start",
                                                                                 std::move(startOpaque),
                                                                                 {{"config", {{"opaque", nontrivialOpaque}}}},
                                                                                 [&threads](auto params, auto handler) {
                                                                                     return threads.start(std::move(params),
                                                                                                          std::move(handler));
                                                                                 }));

            encodingCases.push_back(makeEncodingCase<typed::ThreadRollbackResponse>(
                "thread/rollback:numturns-uint32-max",
                "thread/rollback",
                "thread-rollback",
                typed::ThreadRollbackParams{threadId(), std::numeric_limits<std::uint32_t>::max()},
                {{"numTurns", std::numeric_limits<std::uint32_t>::max()}, {"threadId", ThreadIdValue}},
                [&threads](auto params, auto handler) {
                    return threads.rollback(std::move(params), std::move(handler));
                }));

            typed::TurnStartParams turnOmitted;
            turnOmitted.threadId = threadId();
            turnOmitted.input = {textInput()};
            encodingCases.push_back(makeEncodingCase<typed::TurnStartResponse>(
                "turn/start:omitted",
                "turn/start",
                "turn-start",
                std::move(turnOmitted),
                {{"input",
                  codex::Json::array({codex::Json{{"text", "synthetic-796383c7b4"},
                                                  {"text_elements",
                                                   codex::Json::array({codex::Json{{"byteRange", {{"end", 0}, {"start", 0}}},
                                                                                   {"placeholder", "synthetic-3cb304aacb"}}})},
                                                  {"type", "text"}}})},
                 {"threadId", ThreadIdValue}},
                [&turns](auto params, auto handler) {
                    return turns.start(std::move(params), std::move(handler));
                }));
            typed::TurnStartParams turnNull;
            turnNull.threadId = threadId();
            turnNull.input = {textInput()};
            turnNull.personality = decltype(turnNull.personality)::explicitNull();
            turnNull.approvalPolicy = decltype(turnNull.approvalPolicy)::explicitNull();
            turnNull.approvalsReviewer = decltype(turnNull.approvalsReviewer)::explicitNull();
            turnNull.clientUserMessageId = decltype(turnNull.clientUserMessageId)::explicitNull();
            turnNull.serviceTier = decltype(turnNull.serviceTier)::explicitNull();
            turnNull.cwd = decltype(turnNull.cwd)::explicitNull();
            turnNull.effort = decltype(turnNull.effort)::explicitNull();
            turnNull.model = decltype(turnNull.model)::explicitNull();
            turnNull.summary = decltype(turnNull.summary)::explicitNull();
            turnNull.outputSchema = decltype(turnNull.outputSchema)::explicitNull();
            turnNull.sandboxPolicy = decltype(turnNull.sandboxPolicy)::explicitNull();
            const codex::Json turnRequiredExpected = encodingCases.back().expectedParams;
            codex::Json turnNullExpected = turnRequiredExpected;
            for (std::string_view field : {"approvalPolicy",
                                           "approvalsReviewer",
                                           "clientUserMessageId",
                                           "cwd",
                                           "effort",
                                           "model",
                                           "outputSchema",
                                           "personality",
                                           "sandboxPolicy",
                                           "serviceTier",
                                           "summary"}) {
                turnNullExpected[std::string(field)] = nullptr;
            }
            encodingCases.push_back(makeEncodingCase<typed::TurnStartResponse>("turn/start:null",
                                                                               "turn/start",
                                                                               "turn-start",
                                                                               std::move(turnNull),
                                                                               std::move(turnNullExpected),
                                                                               [&turns](auto params, auto handler) {
                                                                                   return turns.start(std::move(params),
                                                                                                      std::move(handler));
                                                                               }));
            for (const auto& [label, summary] : std::vector<std::pair<std::string, typed::ReasoningSummary>>{
                     {"concise", typed::ReasoningSummary::concise()},
                     {"detailed", typed::ReasoningSummary::detailed()},
                     {"none", typed::ReasoningSummary::none()},
                 }) {
                typed::TurnStartParams turnSummary;
                turnSummary.threadId = threadId();
                turnSummary.input = {textInput()};
                turnSummary.summary = summary;
                codex::Json expected = turnRequiredExpected;
                expected["summary"] = label;
                encodingCases.push_back(makeEncodingCase<typed::TurnStartResponse>("turn/start:summary-" + label,
                                                                                   "turn/start",
                                                                                   "turn-start",
                                                                                   std::move(turnSummary),
                                                                                   std::move(expected),
                                                                                   [&turns](auto params, auto handler) {
                                                                                       return turns.start(std::move(params),
                                                                                                          std::move(handler));
                                                                                   }));
            }
            typed::TurnStartParams turnOpaque;
            turnOpaque.threadId = threadId();
            turnOpaque.input = {textInput()};
            turnOpaque.outputSchema = nontrivialOpaque;
            codex::Json turnOpaqueExpected = turnRequiredExpected;
            turnOpaqueExpected["outputSchema"] = nontrivialOpaque;
            encodingCases.push_back(makeEncodingCase<typed::TurnStartResponse>("turn/start:opaque-output-schema",
                                                                               "turn/start",
                                                                               "turn-start",
                                                                               std::move(turnOpaque),
                                                                               std::move(turnOpaqueExpected),
                                                                               [&turns](auto params, auto handler) {
                                                                                   return turns.start(std::move(params),
                                                                                                      std::move(handler));
                                                                               }));

            typed::TurnSteerParams steerOmitted;
            steerOmitted.threadId = threadId();
            steerOmitted.expectedTurnId = typed::TurnId{"synthetic-3f6f42ff96"};
            steerOmitted.input = {textInput()};
            codex::Json steerOmittedExpected = readFixture("turn-steer", "params");
            steerOmittedExpected.erase("clientUserMessageId");
            encodingCases.push_back(makeEncodingCase<typed::TurnSteerResponse>(
                "turn/steer:omitted", "turn/steer", "turn-steer", steerOmitted, steerOmittedExpected, [&turns](auto params, auto handler) {
                    return turns.steer(std::move(params), std::move(handler));
                }));
            steerOmitted.clientUserMessageId = decltype(steerOmitted.clientUserMessageId)::explicitNull();
            steerOmittedExpected["clientUserMessageId"] = nullptr;
            encodingCases.push_back(makeEncodingCase<typed::TurnSteerResponse>("turn/steer:null",
                                                                               "turn/steer",
                                                                               "turn-steer",
                                                                               std::move(steerOmitted),
                                                                               std::move(steerOmittedExpected),
                                                                               [&turns](auto params, auto handler) {
                                                                                   return turns.steer(std::move(params),
                                                                                                      std::move(handler));
                                                                               }));
        }

        void buildLaunchResponseCases() {
            auto& threads = client->typed().threads();

            for (LaunchResponseVariant variant :
                 {LaunchResponseVariant::Omitted, LaunchResponseVariant::Null, LaunchResponseVariant::UnknownOpenEnums}) {
                const std::string suffix = variant == LaunchResponseVariant::Omitted
                                               ? "omitted"
                                               : (variant == LaunchResponseVariant::Null ? "null" : "value-open-enums");
                launchResponseCases.push_back(makeLaunchResponseCase<typed::ThreadForkResponse>("thread/fork:response-" + suffix,
                                                                                                "thread/fork",
                                                                                                "thread-fork",
                                                                                                variant,
                                                                                                threadForkParams(),
                                                                                                [&threads](auto params, auto handler) {
                                                                                                    return threads.fork(std::move(params),
                                                                                                                        std::move(handler));
                                                                                                }));
                launchResponseCases.push_back(
                    makeLaunchResponseCase<typed::ThreadResumeResponse>("thread/resume:response-" + suffix,
                                                                        "thread/resume",
                                                                        "thread-resume",
                                                                        variant,
                                                                        threadResumeParams(),
                                                                        [&threads](auto params, auto handler) {
                                                                            return threads.resume(std::move(params), std::move(handler));
                                                                        }));
                launchResponseCases.push_back(
                    makeLaunchResponseCase<typed::ThreadStartResponse>("thread/start:response-" + suffix,
                                                                       "thread/start",
                                                                       "thread-start",
                                                                       variant,
                                                                       threadStartParams(),
                                                                       [&threads](auto params, auto handler) {
                                                                           return threads.start(std::move(params), std::move(handler));
                                                                       }));
            }
        }

        tests::support::TestResult& testResult;
        std::shared_ptr<UnixTransportState> state;
        std::unique_ptr<TestClient> client;
        std::vector<OperationCase> cases;
        std::vector<EncodingCase> encodingCases;
        std::vector<LaunchResponseCase> launchResponseCases;
        std::map<std::string, std::size_t> typedObserverCounts;
        std::map<std::string, std::size_t> rawObserverCounts;
        std::vector<std::string> observerOrder;
        std::vector<std::string> successOrder;
        std::vector<std::string> remoteOrder;
        std::vector<std::string> cancellationOrder;
        std::vector<std::string> generationOrder;
        std::vector<std::string> encodingOrder;
        std::vector<std::string> malformedUnitOrder;
        std::vector<std::string> launchResponseOrder;
        std::vector<std::optional<codex::ClientRequestId>> generationIds;
        std::size_t successCallbacks = 0;
        std::size_t remoteCallbacks = 0;
        std::size_t cancellationCallbacks = 0;
        std::size_t generationCallbacks = 0;
        std::size_t encodingCallbacks = 0;
        std::size_t malformedUnitCallbacks = 0;
        std::size_t launchResponseCallbacks = 0;
        std::size_t unexpectedLocalCallbacks = 0;
        int readyCount = 0;
        bool socketReady = false;
        bool insideSubmission = false;
        bool firstStopObserved = false;
        bool restartScheduled = false;
        bool finished = false;
    };
} // namespace

int main(int argc, char* argv[]) {
    tests::support::TestResult result;
    core::SNodeC::init(argc, argv);

    OperationWireRunner runner(result);
    runner.start();

    bool timedOut = false;
    [[maybe_unused]] core::timer::Timer watchdog = core::timer::Timer::singleshotTimer(
        [&]() {
            timedOut = true;
            core::SNodeC::stop();
        },
        utils::Timeval({12, 0}));
    const int startResult = core::SNodeC::start(utils::Timeval({13, 0}));

    result.expectTrue(!timedOut && runner.isFinished(), "A1.1 all-operation AF_UNIX lifecycle matrix completes before the watchdog");
    result.expectEqual(0, startResult, "A1.1 all-operation AF_UNIX lifecycle matrix stops the event loop cleanly");
    core::SNodeC::free();
    return result.processResult();
}
