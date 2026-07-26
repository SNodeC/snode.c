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
#include "ai/openai/codex/typed/Events.h"
#include "ai/openai/codex/typed/Reviews.h"
#include "ai/openai/codex/typed/Threads.h"
#include "core/EventReceiver.h"
#include "core/SNodeC.h"
#include "core/timer/Timer.h"
#include "support/TestResult.h"
#include "utils/Timeval.h"

#include <cerrno>
#include <cstddef>
#include <cstdint>
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

namespace {
    namespace codex = ai::openai::codex;
    namespace detail = ai::openai::codex::detail;
    namespace typed = ai::openai::codex::typed;

    using Submission = codex::AppServerClient::RawProtocol::Submission;

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

    codex::Json initializeResult() {
        return {
            {"codexHome", "/tmp/codex-a1-3-review-wire"},
            {"platformFamily", "unix"},
            {"platformOs", "linux"},
            {"userAgent", "codex-a1-3-review-wire/1"},
        };
    }

    codex::Json turnResult(std::string reviewThreadId, std::string turnId) {
        return {
            {"reviewThreadId", reviewThreadId},
            {"turn",
             {
                 {"id", std::move(turnId)},
                 {"items", codex::Json::array()},
                 {"status", "inProgress"},
                 {"futureTurnField", true},
             }},
            {"futureResponseField", true},
        };
    }

    codex::Json guardianReview(std::string status = "inProgress") {
        return {
            {"rationale", nullptr},
            {"riskLevel", nullptr},
            {"status", std::move(status)},
            {"userAuthorization", nullptr},
        };
    }

    std::vector<codex::Json> guardianActions() {
        return {
            {
                {"command", "synthetic command"},
                {"cwd", "/synthetic/command-cwd"},
                {"source", "shell"},
                {"type", "command"},
            },
            {
                {"argv", codex::Json::array({"synthetic-program", "arg with spaces"})},
                {"cwd", "/synthetic/execve-cwd"},
                {"program", "synthetic-program"},
                {"source", "unifiedExec"},
                {"type", "execve"},
            },
            {
                {"cwd", "/synthetic/patch-cwd"},
                {"files", codex::Json::array({"/synthetic/a", "/synthetic/b"})},
                {"type", "applyPatch"},
            },
            {
                {"host", "synthetic.invalid"},
                {"port", 443},
                {"protocol", "https"},
                {"target", "https://synthetic.invalid"},
                {"type", "networkAccess"},
            },
            {
                {"connectorId", nullptr},
                {"connectorName", nullptr},
                {"server", "synthetic-server"},
                {"toolName", "synthetic_tool"},
                {"toolTitle", "Synthetic tool"},
                {"type", "mcpToolCall"},
            },
            {
                {"permissions", {{"fileSystem", nullptr}, {"network", {{"enabled", false}}}}},
                {"reason", "synthetic reason"},
                {"type", "requestPermissions"},
            },
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
        ReplyMode replyMode = ReplyMode::Success;
        int clientDescriptor = -1;
        int serverDescriptor = -1;
        bool duplicateFirstReviewResult = false;
        bool firstReviewResultSent = false;
        bool running = false;

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
            const std::string serverLine = envelope.dump() + "\n";
            if (!writeFully(serverDescriptor, serverLine)) {
                return false;
            }
            const std::optional<std::string> received = readLine(clientDescriptor);
            if (!received || *received != serverLine || !target.onMessage) {
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
            codex::Json envelope = codex::Json::parse(received->begin(), received->end() - 1, nullptr, false);
            if (envelope.is_discarded()) {
                return false;
            }
            outbound.push_back({*received, envelope});

            const auto method = envelope.find("method");
            if (method == envelope.end() || !method->is_string()) {
                return true;
            }
            if (*method == "initialize") {
                return envelope.contains("id") && injectCurrent({{"id", envelope.at("id")}, {"result", initializeResult()}});
            }
            if (*method == "initialized" || !envelope.contains("id") || replyMode == ReplyMode::Hold) {
                return true;
            }
            if (replyMode == ReplyMode::RemoteError) {
                return injectCurrent({{"id", envelope.at("id")},
                                      {"error",
                                       {
                                           {"code", -32'415},
                                           {"message", "synthetic review remote failure"},
                                           {"data", {{"operation", *method}}},
                                           {"futureErrorField", true},
                                       }}});
            }
            if (*method == "thread/approveGuardianDeniedAction") {
                return injectCurrent({{"id", envelope.at("id")}, {"result", codex::Json::object()}});
            }
            if (*method != "review/start") {
                return false;
            }

            const bool detached = envelope.at("params").value("delivery", "") == "detached";
            const codex::Json result = detached ? turnResult("thread-review-detached", "turn-review-detached")
                                                : turnResult(envelope.at("params").value("threadId", ""), "turn-review-inline");
            if (!injectCurrent({{"id", envelope.at("id")}, {"result", result}})) {
                return false;
            }
            if (duplicateFirstReviewResult && !std::exchange(firstReviewResultSent, true)) {
                return injectCurrent({{"id", envelope.at("id")}, {"result", result}});
            }
            return true;
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
            : AppServerClient(std::make_unique<UnixTranscriptTransport>(state),
                              {"codex_a1_3_review_wire_test", "Codex A1.3 Review Wire Test", "1"}) {
        }
    };

    class ReviewWireRunner {
    public:
        explicit ReviewWireRunner(tests::support::TestResult& result)
            : result(result)
            , state(std::make_shared<UnixTransportState>())
            , socketReady(state->open())
            , client(std::make_unique<TestClient>(state)) {
        }

        void start() {
            expect(socketReady, "A1.3 review wire test opens its AF_UNIX socketpair");
            if (!socketReady) {
                finished = true;
                core::SNodeC::stop();
                return;
            }

            const Submission rejectedReview = client->typed().reviews().start(
                {
                    .threadId = {"thread-local-rejection"},
                    .target = typed::UncommittedChangesReviewTarget{},
                    .delivery = typed::OptionalNullable<typed::ReviewDelivery>::omitted(),
                },
                [this](const typed::OperationResult<typed::ReviewStartResponse>&) {
                    ++unexpectedLocalCallbacks;
                });
            const Submission rejectedGuardian = client->typed().threads().approveGuardianDeniedAction(
                {
                    .threadId = {"thread-local-rejection"},
                    .event = {{"synthetic", true}},
                },
                [this](const typed::OperationResult<typed::Unit>&) {
                    ++unexpectedLocalCallbacks;
                });
            expect(!rejectedReview && !rejectedReview.id && rejectedReview.error &&
                       rejectedReview.error->category == codex::Error::Category::InvalidState && !rejectedGuardian &&
                       !rejectedGuardian.id && rejectedGuardian.error &&
                       rejectedGuardian.error->category == codex::Error::Category::InvalidState && unexpectedLocalCallbacks == 0 &&
                       state->outbound.empty(),
                   "both review operations reject synchronously before RawProtocol is ready");

            client->typed().events().setOnEvent([this](const typed::Event& event) {
                handleTypedEvent(event);
            });
            client->raw().setOnNotification([this](const codex::Notification& notification) {
                rawEventMethods.push_back(notification.method);
            });
            client->setOnStateChanged([this](const codex::StateChange& change) {
                handleState(change);
            });
            client->start();
        }

        [[nodiscard]] bool isFinished() const noexcept {
            return finished;
        }

    private:
        void expect(bool condition, std::string message) {
            result.expectTrue(condition, std::move(message));
        }

        void handleState(const codex::StateChange& change) {
            if (change.current == codex::State::Ready) {
                ++readyCount;
                if (readyCount == 1) {
                    core::EventReceiver::atNextTick([this]() {
                        beginSuccess();
                    });
                } else if (readyCount == 2) {
                    core::EventReceiver::atNextTick([this]() {
                        beginGenerationProbe();
                    });
                }
                return;
            }
            if (change.current == codex::State::Failed) {
                expect(false, "A1.3 review wire lifecycle must not fail");
                client->stop();
                return;
            }
            if (change.current != codex::State::Stopped) {
                return;
            }

            ++stoppedCount;
            if (stoppedCount == 1) {
                firstStopObserved = true;
                maybeRestart();
                return;
            }
            finished = true;
            core::SNodeC::stop();
        }

        void beginSuccess() {
            state->replyMode = UnixTransportState::ReplyMode::Success;
            state->duplicateFirstReviewResult = true;
            const std::size_t before = state->outbound.size();

            insideSubmission = true;
            const Submission inlineSubmission = client->typed().reviews().start(
                {
                    .threadId = {"thread-review-inline"},
                    .target = typed::UncommittedChangesReviewTarget{},
                    .delivery = typed::OptionalNullable<typed::ReviewDelivery>::omitted(),
                },
                [this](const typed::OperationResult<typed::ReviewStartResponse>& operation) {
                    expect(!insideSubmission, "inline review completion remains asynchronous");
                    expect(operation && operation.value && operation.value->reviewThreadId.value == "thread-review-inline" &&
                               operation.value->turn.threadId.value == "thread-review-inline" &&
                               operation.value->turn.id.value == "turn-review-inline" &&
                               operation.value->raw.value("futureResponseField", false),
                           "inline review returns the original thread and canonical Turn");
                    ++initialCallbacks;
                    maybeInjectEvents();
                });

            const Submission detachedSubmission = client->typed().reviews().start(
                {
                    .threadId = {"thread-origin"},
                    .target = typed::CustomReviewTarget{"Review only synthetic changes."},
                    .delivery = typed::OptionalNullable<typed::ReviewDelivery>::withValue(typed::ReviewDelivery::detached()),
                },
                [this](const typed::OperationResult<typed::ReviewStartResponse>& operation) {
                    expect(!insideSubmission, "detached review completion remains asynchronous");
                    expect(operation && operation.value && operation.value->reviewThreadId.value == "thread-review-detached" &&
                               operation.value->turn.threadId.value == "thread-review-detached" &&
                               operation.value->turn.id.value == "turn-review-detached",
                           "detached review returns its new review thread and canonical Turn");
                    ++initialCallbacks;
                    completionCallbackThrew = true;
                    maybeInjectEvents();
                    throw std::runtime_error("intentional detached review completion callback failure");
                });

            const Submission guardianSubmission = client->typed().threads().approveGuardianDeniedAction(
                {
                    .threadId = {"thread-guardian"},
                    .event =
                        {
                            {"assessment", "synthetic-denied"},
                            {"nested", codex::Json::array({true, nullptr, 7})},
                        },
                },
                [this](const typed::OperationResult<typed::Unit>& operation) {
                    expect(!insideSubmission, "guardian approval completion remains asynchronous");
                    expect(operation && operation.value && operation.raw == codex::Json::object(),
                           "guardian approval accepts the exact Unit response");
                    ++initialCallbacks;
                    maybeInjectEvents();
                });
            insideSubmission = false;

            expect(inlineSubmission && inlineSubmission.id && detachedSubmission && detachedSubmission.id && guardianSubmission &&
                       guardianSubmission.id,
                   "review/start and guardian approval use the existing RawProtocol");
            expect(state->outbound.size() == before + 3, "three accepted review operations cross AF_UNIX exactly once");

            if (state->outbound.size() != before + 3) {
                client->stop();
                return;
            }
            const std::vector<codex::Json> expected{
                {
                    {"id", inlineSubmission.id->value()},
                    {"method", "review/start"},
                    {"params",
                     {
                         {"target", {{"type", "uncommittedChanges"}}},
                         {"threadId", "thread-review-inline"},
                     }},
                },
                {
                    {"id", detachedSubmission.id->value()},
                    {"method", "review/start"},
                    {"params",
                     {
                         {"delivery", "detached"},
                         {"target",
                          {
                              {"instructions", "Review only synthetic changes."},
                              {"type", "custom"},
                          }},
                         {"threadId", "thread-origin"},
                     }},
                },
                {
                    {"id", guardianSubmission.id->value()},
                    {"method", "thread/approveGuardianDeniedAction"},
                    {"params",
                     {
                         {"event",
                          {
                              {"assessment", "synthetic-denied"},
                              {"nested", codex::Json::array({true, nullptr, 7})},
                          }},
                         {"threadId", "thread-guardian"},
                     }},
                },
            };
            std::size_t exact = 0;
            for (std::size_t index = 0; index < expected.size(); ++index) {
                const OutboundRecord& record = state->outbound[before + index];
                exact += record.envelope == expected[index] && record.line == expected[index].dump() + "\n" ? 1U : 0U;
            }
            expect(exact == expected.size(), "review and guardian operations emit exact JSON-RPC/JSONL bytes");

            const detail::ProtocolSurfaceEntry& reviewEntry = detail::entryFor(detail::ClientRequestTarget::ReviewStart);
            const detail::ProtocolSurfaceEntry& guardianEntry =
                detail::entryFor(detail::ClientRequestTarget::ThreadApproveGuardianDeniedAction);
            expect(reviewEntry.key.name == "review/start" &&
                       reviewEntry.operationContract.resultKind == detail::ResultContractKind::Concrete &&
                       guardianEntry.key.name == "thread/approveGuardianDeniedAction" &&
                       guardianEntry.operationContract.resultKind == detail::ResultContractKind::Unit,
                   "both public methods resolve through their exact registry descriptors");
        }

        void maybeInjectEvents() {
            if (eventsScheduled || initialCallbacks != 3) {
                return;
            }
            eventsScheduled = true;
            core::EventReceiver::atNextTick([this]() {
                injectEvents();
            });
        }

        codex::Json startedParams(codex::Json action, std::size_t index) {
            return {
                {"action", std::move(action)},
                {"review", guardianReview()},
                {"reviewId", "review-" + std::to_string(index)},
                {"startedAtMs", static_cast<std::int64_t>(100 + index)},
                {"targetItemId", nullptr},
                {"threadId", "thread-review-events"},
                {"turnId", "turn-review-events"},
            };
        }

        bool injectNotification(std::string method, codex::Json params) {
            return state->injectCurrent({
                {"jsonrpc", "2.0"},
                {"method", std::move(method)},
                {"params", std::move(params)},
                {"futureEnvelopeField", "preserved"},
            });
        }

        void injectEvents() {
            expect(initialCallbacks == 3, "duplicate review result produces no duplicate completion");
            expect(completionCallbackThrew, "throwing review completion callback is contained");

            std::size_t injected = 0;
            injected += injectNotification("guardianWarning",
                                           {
                                               {"message", "Synthetic warning."},
                                               {"threadId", "thread-review-events"},
                                           })
                            ? 1U
                            : 0U;
            const std::vector<codex::Json> actions = guardianActions();
            for (std::size_t index = 0; index < actions.size(); ++index) {
                injected += injectNotification("item/autoApprovalReview/started", startedParams(actions[index], index)) ? 1U : 0U;
            }

            codex::Json completed = startedParams(actions[3], 6);
            completed["completedAtMs"] = 200;
            completed["decisionSource"] = "agent";
            completed["review"] = guardianReview("approved");
            injected += injectNotification("item/autoApprovalReview/completed", completed) ? 1U : 0U;
            injected += injectNotification("item/autoApprovalReview/started",
                                           startedParams(
                                               {
                                                   {"opaque", {{"kept", true}}},
                                                   {"type", "futureGuardianAction"},
                                               },
                                               7))
                            ? 1U
                            : 0U;
            injected += injectNotification("item/autoApprovalReview/started",
                                           startedParams(
                                               {
                                                   {"command", "SYNTHETIC_SENSITIVE_COMMAND"},
                                                   {"cwd", 7},
                                                   {"source", "shell"},
                                                   {"type", "command"},
                                               },
                                               8))
                            ? 1U
                            : 0U;
            codex::Json malformedRoot = startedParams(actions[0], 9);
            malformedRoot["reviewId"] = 7;
            malformedRoot["sensitiveExtension"] = "SYNTHETIC_SENSITIVE_EXTENSION";
            injected += injectNotification("item/autoApprovalReview/started", malformedRoot) ? 1U : 0U;
            expect(injected == ExpectedNotifications, "all review/guardian notifications cross the real AF_UNIX socket");
            core::EventReceiver::atNextTick([this]() {
                completeEventPhase();
            });
        }

        std::string actionName(const typed::GuardianApprovalReviewAction& action) {
            return std::visit(
                [](const auto& alternative) -> std::string {
                    using Alternative = std::decay_t<decltype(alternative)>;
                    if constexpr (std::is_same_v<Alternative, typed::CommandGuardianApprovalReviewAction>) {
                        return "command";
                    } else if constexpr (std::is_same_v<Alternative, typed::ExecveGuardianApprovalReviewAction>) {
                        return "execve";
                    } else if constexpr (std::is_same_v<Alternative, typed::ApplyPatchGuardianApprovalReviewAction>) {
                        return "applyPatch";
                    } else if constexpr (std::is_same_v<Alternative, typed::NetworkAccessGuardianApprovalReviewAction>) {
                        return "networkAccess";
                    } else if constexpr (std::is_same_v<Alternative, typed::McpToolCallGuardianApprovalReviewAction>) {
                        return "mcpToolCall";
                    } else if constexpr (std::is_same_v<Alternative, typed::RequestPermissionsGuardianApprovalReviewAction>) {
                        return "requestPermissions";
                    } else {
                        return alternative.type.value_or("<missing>");
                    }
                },
                action);
        }

        void handleTypedEvent(const typed::Event& event) {
            if (const auto* warning = std::get_if<typed::GuardianWarningNotification>(&event)) {
                typedEventOrder.push_back("guardianWarning");
                expect(warning->threadId.value == "thread-review-events" && warning->message == "Synthetic warning." &&
                           warning->raw.value("futureEnvelopeField", "") == "preserved",
                       "guardianWarning uses the existing typed event mechanism");
                if (!reentrantSubmitted) {
                    reentrantSubmitted = true;
                    const Submission submission = client->typed().threads().approveGuardianDeniedAction(
                        {
                            .threadId = {"thread-review-events"},
                            .event = {{"assessment", "synthetic-reentrant"}},
                        },
                        [this](const typed::OperationResult<typed::Unit>& operation) {
                            expect(operation && operation.value, "event callback can submit a reentrant guardian approval");
                            ++reentrantCallbacks;
                        });
                    expect(submission && submission.id, "reentrant guardian approval is accepted by RawProtocol");
                    eventCallbackThrew = true;
                    throw std::runtime_error("intentional guardian warning callback failure");
                }
                return;
            }
            if (const auto* started = std::get_if<typed::ItemGuardianApprovalReviewStartedNotification>(&event)) {
                const std::string name = actionName(started->action);
                typedEventOrder.push_back("started:" + name);
                if (name == "futureGuardianAction") {
                    ++futureActionEvents;
                    expect(std::holds_alternative<typed::UnknownGuardianApprovalReviewAction>(started->action) &&
                               !started->diagnostics.empty() &&
                               started->diagnostics.front().severity == typed::DecodeIssueSeverity::ForwardCompatibility,
                           "future guardian action remains a typed forward-compatible event");
                } else if (name == "command" && std::holds_alternative<typed::UnknownGuardianApprovalReviewAction>(started->action)) {
                    ++malformedActionEvents;
                    expect(!started->diagnostics.empty() &&
                               started->diagnostics.front().kind == typed::DecodeIssueKind::MalformedKnownPayload,
                           "malformed known guardian action remains typed and diagnosed");
                } else {
                    ++knownActionEvents;
                }
                return;
            }
            if (const auto* completed = std::get_if<typed::ItemGuardianApprovalReviewCompletedNotification>(&event)) {
                typedEventOrder.push_back("completed:" + actionName(completed->action));
                ++completedEvents;
                expect(completed->completedAtMs == 200 && completed->decisionSource == typed::AutoReviewDecisionSource::agent() &&
                           completed->review.status == typed::GuardianApprovalReviewStatus::approved(),
                       "completed guardian review decodes its terminal fields");
                return;
            }
            if (const auto* unknown = std::get_if<typed::UnknownEvent>(&event);
                unknown && unknown->method == "item/autoApprovalReview/started") {
                typedEventOrder.push_back("malformed-root");
                ++malformedRootEvents;
                expect(unknown->diagnostic && unknown->diagnostic->kind == typed::DecodeIssueKind::MalformedKnownPayload &&
                           unknown->decodingError && unknown->decodingError->find("SYNTHETIC_SENSITIVE_COMMAND") == std::string::npos &&
                           unknown->decodingError->find("SYNTHETIC_SENSITIVE_EXTENSION") == std::string::npos,
                       "malformed notification remains raw-observable with a redacted diagnostic");
            }
        }

        void completeEventPhase() {
            if (reentrantCallbacks == 0 && eventPhasePolls++ < 4) {
                core::EventReceiver::atNextTick([this]() {
                    completeEventPhase();
                });
                return;
            }
            const std::vector<std::string> expectedTypedOrder{
                "guardianWarning",
                "started:command",
                "started:execve",
                "started:applyPatch",
                "started:networkAccess",
                "started:mcpToolCall",
                "started:requestPermissions",
                "completed:networkAccess",
                "started:futureGuardianAction",
                "started:command",
                "malformed-root",
            };
            const std::vector<std::string> expectedRawOrder{
                "guardianWarning",
                "item/autoApprovalReview/started",
                "item/autoApprovalReview/started",
                "item/autoApprovalReview/started",
                "item/autoApprovalReview/started",
                "item/autoApprovalReview/started",
                "item/autoApprovalReview/started",
                "item/autoApprovalReview/completed",
                "item/autoApprovalReview/started",
                "item/autoApprovalReview/started",
                "item/autoApprovalReview/started",
            };
            expect(typedEventOrder == expectedTypedOrder && rawEventMethods == expectedRawOrder,
                   "typed and raw observers preserve exact notification ordering");
            expect(knownActionEvents == 6 && completedEvents == 1 && futureActionEvents == 1 && malformedActionEvents == 1 &&
                       malformedRootEvents == 1,
                   "all action alternatives and both compatibility paths dispatch once");
            expect(eventCallbackThrew && reentrantSubmitted && reentrantCallbacks == 1,
                   "event callback exceptions are contained and reentrant completion runs once");
            beginRemoteError();
        }

        typed::ReviewStartParams lifecycleParams(std::string threadId) {
            return {
                .threadId = {std::move(threadId)},
                .target = typed::BaseBranchReviewTarget{"synthetic/base"},
                .delivery = typed::OptionalNullable<typed::ReviewDelivery>::explicitNull(),
            };
        }

        void beginRemoteError() {
            state->replyMode = UnixTransportState::ReplyMode::RemoteError;
            const Submission submission = client->typed().reviews().start(
                lifecycleParams("thread-remote-error"), [this](const typed::OperationResult<typed::ReviewStartResponse>& operation) {
                    expect(operation.kind == typed::OperationResult<typed::ReviewStartResponse>::Kind::RemoteError && !operation.value &&
                               operation.remoteError && operation.remoteError->code == -32'415 &&
                               operation.remoteError->raw.value("futureErrorField", false),
                           "review/start retains its exact JSON-RPC failure");
                    core::EventReceiver::atNextTick([this]() {
                        beginCancellation();
                    });
                });
            expect(submission && submission.id, "review/start remote-error probe is accepted");
        }

        void beginCancellation() {
            state->replyMode = UnixTransportState::ReplyMode::Hold;
            const Submission submission = client->typed().reviews().start(
                lifecycleParams("thread-cancelled"), [this](const typed::OperationResult<typed::ReviewStartResponse>& operation) {
                    expect(operation.kind == typed::OperationResult<typed::ReviewStartResponse>::Kind::Cancelled && !operation.value &&
                               operation.localError && operation.localError->category == codex::Error::Category::Cancelled,
                           "held review/start receives one cancellation on disconnect");
                    ++cancellationCallbacks;
                    maybeRestart();
                });
            expect(submission && submission.id, "held review/start is accepted before disconnect");
            client->stop();
        }

        void maybeRestart() {
            if (restartScheduled || !firstStopObserved || cancellationCallbacks != 1) {
                return;
            }
            restartScheduled = true;
            core::EventReceiver::atNextTick([this]() {
                client->start();
            });
        }

        void beginGenerationProbe() {
            state->replyMode = UnixTransportState::ReplyMode::Hold;
            const Submission submission = client->typed().reviews().start(
                lifecycleParams("thread-generation"), [this](const typed::OperationResult<typed::ReviewStartResponse>& operation) {
                    expect(operation && operation.value && operation.value->reviewThreadId.value == "thread-generation" &&
                               operation.value->turn.threadId.value == "thread-generation",
                           "current-generation review response completes exactly once");
                    ++generationCallbacks;
                    core::EventReceiver::atNextTick([this]() {
                        client->stop();
                    });
                });
            expect(submission && submission.id, "generation review/start is accepted after reconnect");
            if (!submission.id || state->callbackGenerations.size() < 2) {
                client->stop();
                return;
            }
            const codex::Json response{
                {"id", submission.id->value()},
                {"result", turnResult("thread-generation", "turn-generation")},
            };
            const std::size_t typedEventsBeforeStale = typedEventOrder.size();
            const std::size_t rawEventsBeforeStale = rawEventMethods.size();
            expect(state->injectWith(state->callbackGenerations.front(),
                                     {
                                         {"jsonrpc", "2.0"},
                                         {"method", "guardianWarning"},
                                         {"params",
                                          {
                                              {"message", "Stale synthetic warning."},
                                              {"threadId", "thread-stale-generation"},
                                          }},
                                     }),
                   "stale-generation guardian notification bytes cross AF_UNIX");
            expect(state->injectWith(state->callbackGenerations.front(), response), "stale-generation review bytes cross AF_UNIX");
            core::EventReceiver::atNextTick([this, response, typedEventsBeforeStale, rawEventsBeforeStale]() {
                expect(generationCallbacks == 0, "stale transport callback cannot complete a review");
                expect(typedEventOrder.size() == typedEventsBeforeStale && rawEventMethods.size() == rawEventsBeforeStale,
                       "stale transport callback cannot dispatch guardian notifications");
                expect(state->injectWith(state->callbackGenerations.back(), response), "current-generation review bytes cross AF_UNIX");
                expect(state->injectWith(state->callbackGenerations.back(), response),
                       "duplicate current-generation review bytes cross AF_UNIX");
            });
        }

        static constexpr std::size_t ExpectedNotifications = 11;

        tests::support::TestResult& result;
        std::shared_ptr<UnixTransportState> state;
        bool socketReady = false;
        std::unique_ptr<TestClient> client;
        std::vector<std::string> typedEventOrder;
        std::vector<std::string> rawEventMethods;
        std::size_t initialCallbacks = 0;
        std::size_t reentrantCallbacks = 0;
        std::size_t eventPhasePolls = 0;
        std::size_t knownActionEvents = 0;
        std::size_t completedEvents = 0;
        std::size_t futureActionEvents = 0;
        std::size_t malformedActionEvents = 0;
        std::size_t malformedRootEvents = 0;
        std::size_t unexpectedLocalCallbacks = 0;
        std::size_t cancellationCallbacks = 0;
        std::size_t generationCallbacks = 0;
        int readyCount = 0;
        int stoppedCount = 0;
        bool insideSubmission = false;
        bool completionCallbackThrew = false;
        bool eventsScheduled = false;
        bool reentrantSubmitted = false;
        bool eventCallbackThrew = false;
        bool firstStopObserved = false;
        bool restartScheduled = false;
        bool finished = false;
    };
} // namespace

int main(int argc, char* argv[]) {
    tests::support::TestResult result;
    core::SNodeC::init(argc, argv);

    ReviewWireRunner runner(result);
    runner.start();

    bool timedOut = false;
    [[maybe_unused]] core::timer::Timer watchdog = core::timer::Timer::singleshotTimer(
        [&]() {
            timedOut = true;
            core::SNodeC::stop();
        },
        utils::Timeval({15, 0}));
    const int startResult = core::SNodeC::start(utils::Timeval({16, 0}));

    result.expectTrue(!timedOut && runner.isFinished(), "A1.3 review AF_UNIX lifecycle completes before watchdog");
    result.expectEqual(0, startResult, "A1.3 review lifecycle stops the event loop cleanly");
    core::SNodeC::free();
    return result.processResult();
}
