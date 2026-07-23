/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include "ai/openai/codex/AppServerClient.h"
#include "ai/openai/codex/detail/Transport.h"
#include "ai/openai/codex/typed/Client.h"
#include "core/EventReceiver.h"
#include "core/SNodeC.h"
#include "core/timer/Timer.h"
#include "support/TestResult.h"
#include "utils/Timeval.h"

#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace {
    using ai::openai::codex::AppServerClient;
    using ai::openai::codex::Error;
    using ai::openai::codex::Json;
    using ai::openai::codex::Notification;
    using ai::openai::codex::ProtocolError;
    using ai::openai::codex::ServerRequest;
    using ai::openai::codex::ServerRequestId;
    using ai::openai::codex::State;
    using ai::openai::codex::StateChange;
    using ai::openai::codex::detail::ProcessExit;
    using ai::openai::codex::detail::Transport;
    using ai::openai::codex::detail::TransportCallbacks;
    using namespace ai::openai::codex::typed;

    struct FakeTransportState {
        TransportCallbacks callbacks;
        std::vector<TransportCallbacks> callbackGenerations;
        std::vector<Json> outgoing;
        std::function<void(const Json&, const TransportCallbacks&)> sendHook;
        bool rejectNextSend = false;
        bool running = false;
        std::size_t startCount = 0;
        std::size_t stopCount = 0;

        void inject(const Json& message) const {
            if (callbacks.onMessage) {
                callbacks.onMessage(message.dump());
            }
        }
    };

    bool hasCallbacks(const TransportCallbacks& callbacks) {
        return callbacks.onStarted || callbacks.onMessage || callbacks.onDiagnostic || callbacks.onError || callbacks.onExited;
    }

    class FakeTransport final : public Transport {
    public:
        explicit FakeTransport(std::shared_ptr<FakeTransportState> state)
            : state(std::move(state)) {
        }

        void setCallbacks(TransportCallbacks callbacks) override {
            state->callbacks = std::move(callbacks);
            if (hasCallbacks(state->callbacks)) {
                state->callbackGenerations.push_back(state->callbacks);
            }
        }

        void start() override {
            ++state->startCount;
            state->running = true;
            if (state->callbacks.onStarted) {
                state->callbacks.onStarted();
            }
        }

        bool send(std::string message) override {
            const Json decoded = Json::parse(message);
            state->outgoing.push_back(decoded);

            const bool accepted = !std::exchange(state->rejectNextSend, false);
            if (accepted && state->sendHook) {
                const TransportCallbacks callbacks = state->callbacks;
                state->sendHook(decoded, callbacks);
            }
            return accepted;
        }

        void stop() override {
            ++state->stopCount;
            if (!state->running) {
                return;
            }

            state->running = false;
            if (state->callbacks.onExited) {
                state->callbacks.onExited(ProcessExit{true, 0, false, 0});
            }
        }

    private:
        std::shared_ptr<FakeTransportState> state;
    };

    class TestClient final : public AppServerClient {
    public:
        explicit TestClient(const std::shared_ptr<FakeTransportState>& state)
            : AppServerClient(std::make_unique<FakeTransport>(state), {"typed_protocol_test", "Typed Protocol Test", "1"}) {
        }
    };

    void inject(const TransportCallbacks& callbacks, const Json& message) {
        if (callbacks.onMessage) {
            callbacks.onMessage(message.dump());
        }
    }

    Json initializeResult() {
        return {
            {"codexHome", "/tmp/codex-typed-test"},
            {"platformFamily", "unix"},
            {"platformOs", "linux"},
            {"userAgent", "codex-typed-test/1"},
        };
    }

    Json turnValue(std::string id, std::string status = "inProgress") {
        return {
            {"id", std::move(id)},
            {"status", std::move(status)},
            {"items", Json::array()},
        };
    }

    Json threadValue(std::string id) {
        return {
            {"id", std::move(id)},
            {"name", "Typed thread"},
            {"cwd", "/tmp/project"},
            {"modelProvider", "openai"},
            {"preview", "typed test"},
            {"cliVersion", "0.144.6"},
            {"sessionId", "session-typed"},
            {"createdAt", 1},
            {"updatedAt", 2},
            {"ephemeral", false},
            {"source", "appServer"},
            {"status", {{"type", "idle"}}},
            {"turns", Json::array()},
        };
    }

    Json threadOperationResult(const std::string& id) {
        return {
            {"approvalPolicy", "on-request"},
            {"approvalsReviewer", "user"},
            {"cwd", "/tmp/project"},
            {"model", "gpt-5"},
            {"modelProvider", "openai"},
            {"sandbox", {{"type", "workspaceWrite"}}},
            {"thread", threadValue(id)},
        };
    }

    Json remoteTurnErrorData() {
        return {
            {"message", "typed turn failure"},
            {"additionalDetails", nullptr},
            {"codexErrorInfo", {{"httpConnectionFailed", {{"httpStatusCode", 503}}}}},
        };
    }

    Json commandApprovalParams(std::string command) {
        return {
            {"threadId", "thread-requests"},
            {"turnId", "turn-requests"},
            {"itemId", "item-" + command},
            {"startedAtMs", 100},
            {"command", std::move(command)},
            {"cwd", "/tmp/project"},
        };
    }

    Json fileApprovalParams(std::string suffix) {
        return {
            {"threadId", "thread-requests"},
            {"turnId", "turn-requests"},
            {"itemId", "item-file-" + suffix},
            {"startedAtMs", 101},
            {"reason", "apply typed test changes"},
            {"grantRoot", nullptr},
        };
    }

    Json userInputParams() {
        return {
            {"threadId", "thread-requests"},
            {"turnId", "turn-requests"},
            {"itemId", "item-user-input"},
            {"questions",
             Json::array({
                 Json{{"id", "first"}, {"header", "First"}, {"question", "First answer?"}, {"options", nullptr}},
                 Json{{"id", "second"}, {"header", "Second"}, {"question", "Second answer?"}, {"options", nullptr}},
             })},
            {"autoResolutionMs", nullptr},
        };
    }

    std::string requestIdString(const ServerRequestId& requestId) {
        return std::visit(
            [](const auto& value) -> std::string {
                using Value = std::decay_t<decltype(value)>;
                if constexpr (std::is_same_v<Value, std::string>) {
                    return value;
                } else {
                    return std::to_string(value);
                }
            },
            requestId.value());
    }

    class TypedProtocolRunner {
    public:
        explicit TypedProtocolRunner(tests::support::TestResult& testResult)
            : testResult(testResult) {
        }

        void start() {
            beginCoreScenario();
        }

        bool isFinished() const noexcept {
            return finished;
        }

    private:
        void expect(bool condition, const std::string& message) {
            testResult.expectTrue(condition, message);
        }

        static void defer(std::function<void()> callback) {
            core::EventReceiver::atNextTick(std::move(callback));
        }

        void installStandardSendHook() {
            state->sendHook = [this](const Json& message, const TransportCallbacks& callbacks) {
                const auto method = message.find("method");
                if (method == message.end() || !method->is_string()) {
                    return;
                }

                const auto id = message.find("id");
                if (*method == "initialize") {
                    if (id != message.end()) {
                        inject(callbacks, Json{{"id", *id}, {"result", initializeResult()}});
                    }
                    return;
                }
                if (id == message.end()) {
                    return;
                }

                const auto params = message.find("params");
                if (*method == "thread/start" && params != message.end() && params->is_object()) {
                    const auto cwd = params->find("cwd");
                    if (cwd != params->end() && cwd->is_string() && *cwd == "/malformed") {
                        inject(callbacks, Json{{"id", *id}, {"result", malformedThreadResult}});
                    } else if (cwd != params->end() && cwd->is_string() && *cwd == "/chain") {
                        inject(callbacks, Json{{"id", *id}, {"result", threadOperationResult("thread-chain")}});
                    } else if (cwd != params->end() && cwd->is_string() && *cwd == "/destroy") {
                        inject(callbacks, Json{{"id", *id}, {"result", threadOperationResult("thread-destroy")}});
                    }
                } else if (*method == "thread/resume") {
                    inject(callbacks,
                           Json{{"id", *id},
                                {"error",
                                 {{"code", -32'001},
                                  {"message", "typed remote error"},
                                  {"data", remoteTurnErrorData()}}}});
                } else if (*method == "turn/start") {
                    inject(callbacks, Json{{"id", *id}, {"result", {{"turn", turnValue("turn-chain")}}}});
                }
            };
        }

        void beginCoreScenario() {
            state = std::make_shared<FakeTransportState>();
            installStandardSendHook();
            client = std::make_unique<TestClient>(state);

            ThreadStartOptions notReadyOptions;
            notReadyOptions.cwd = "/not-ready";
            const auto localFailure = client->typed().threads().start(std::move(notReadyOptions), [this](const OperationResult<Thread>&) {
                ++localSubmissionCallbacks;
            });
            expect(!localFailure && localFailure.error && localFailure.error->category == Error::Category::InvalidState,
                   "typed operation submission fails locally while the client is not ready");

            client->typed().events().setOnEvent([this](const Event& event) {
                const UnknownEvent* unknown = std::get_if<UnknownEvent>(&event);
                if (unknown == nullptr) {
                    return;
                }

                if (unknown->method == "typed/order") {
                    expect(unknown->diagnostic &&
                               unknown->diagnostic->kind == DecodeIssueKind::UnknownMethod &&
                               unknown->diagnostic->severity == DecodeIssueSeverity::ForwardCompatibility &&
                               unknown->diagnostic->surface == "typed/order" &&
                               unknown->raw == Json{{"method", "typed/order"},
                                                    {"params", {{"future", true}}},
                                                    {"extension", "same-raw-message"}} &&
                               client->isReady(),
                           "future methods remain connected, raw-retained, and classified as ForwardCompatibility");
                    eventObserverOrder.emplace_back("typed");
                } else if (unknown->method == "thread/started") {
                    expect(unknown->diagnostic &&
                               unknown->diagnostic->kind == DecodeIssueKind::MalformedKnownPayload &&
                               unknown->diagnostic->severity == DecodeIssueSeverity::ProtocolWarning &&
                               unknown->diagnostic->surface == "thread/started" &&
                               unknown->raw == Json{{"method", "thread/started"}, {"params", Json::array({1})}} &&
                               client->isReady(),
                           "known malformed payloads remain connected, raw-retained, and classified as ProtocolWarning");
                    ++malformedStructuredEvents;
                } else if (unknown->method == "typed/throw") {
                    ++throwingEventCallbacks;
                    throw std::runtime_error("intentional typed event callback failure");
                } else if (unknown->method == "typed/persist") {
                    ++persistentTypedEvents;
                }
            });
            client->raw().setOnNotification([this](const Notification& notification) {
                if (notification.method == "typed/order") {
                    eventObserverOrder.emplace_back("raw");
                } else if (notification.method == "typed/throw") {
                    ++rawAfterThrowEvents;
                } else if (notification.method == "typed/persist") {
                    ++persistentRawEvents;
                }
            });

            client->typed().requests().setOnRequest([this](const TypedServerRequest& request) {
                handleTypedServerRequest(request);
            });
            client->raw().setOnServerRequest([this](const ServerRequest& request) {
                ++rawServerRequests;
                serverObserverOrder.emplace_back("raw:" + requestIdString(request.id));
            });

            client->setOnStateChanged([this](const StateChange& change) {
                if (change.current == State::Ready) {
                    ++coreReadyCount;
                    if (coreReadyCount == 1) {
                        launchCoreChecks();
                    } else if (coreReadyCount == 2) {
                        launchRestartChecks();
                    }
                } else if (change.current == State::Failed) {
                    expect(false, "typed protocol core scenario must not fail the connection");
                    client->stop();
                } else if (change.current == State::Stopped && coreFinalStop) {
                    defer([this]() {
                        client.reset();
                        beginStopFromEventScenario();
                    });
                }
            });
            client->start();
        }

        void launchCoreChecks() {
            ThreadStartOptions malformedOptions;
            malformedOptions.cwd = "/malformed";
            insideMalformedSubmission = true;
            const auto malformed =
                client->typed().threads().start(std::move(malformedOptions), [this](const OperationResult<Thread>& result) {
                expect(!insideMalformedSubmission, "typed decoding callback remains asynchronous with a synchronous fake response");
                expect(result.kind == OperationResult<Thread>::Kind::LocalError && result.localError &&
                           result.localError->category == Error::Category::Protocol && result.raw == malformedThreadResult,
                       "malformed successful result becomes a typed LocalError and preserves raw result JSON");
                ++malformedCallbacks;
            });
            insideMalformedSubmission = false;
            expect(static_cast<bool>(malformed), "malformed-result typed operation is accepted for asynchronous completion");

            insideRemoteSubmission = true;
            const auto remote =
                client->typed().threads().resume(ThreadId{"thread-remote"}, {}, [this](const OperationResult<Thread>& result) {
                expect(!insideRemoteSubmission, "typed RemoteError callback remains asynchronous");
                const auto* structuredInfo =
                    result.codexErrorInfo ? std::get_if<HttpConnectionFailedCodexErrorInfo>(&*result.codexErrorInfo) : nullptr;
                expect(result.kind == OperationResult<Thread>::Kind::RemoteError && result.remoteError &&
                           result.remoteError->code == -32'001 && result.remoteError->message == "typed remote error" &&
                           result.remoteError->data == std::optional<Json>(remoteTurnErrorData()) && structuredInfo &&
                           structuredInfo->httpStatusCode.present && structuredInfo->httpStatusCode.value == 503 &&
                           structuredInfo->raw == Json{{"httpConnectionFailed", {{"httpStatusCode", 503}}}} &&
                           !result.codexErrorDiagnostic,
                       "production response adaptation preserves raw RemoteError details and adds structured CodexErrorInfo");
                ++remoteErrorCallbacks;
            });
            insideRemoteSubmission = false;
            expect(static_cast<bool>(remote), "remote-error typed operation is submitted successfully");

            const auto pendingList = client->typed().threads().list({}, [this](const OperationResult<ThreadPage>& result) {
                expect(result.kind == OperationResult<ThreadPage>::Kind::Cancelled && result.localError &&
                           result.localError->category == Error::Category::Cancelled,
                       "pending typed request receives a typed cancellation");
                ++cancellationCallbacks;
            });
            expect(static_cast<bool>(pendingList), "typed request intended for cancellation is accepted");

            ThreadStartOptions chainOptions;
            chainOptions.cwd = "/chain";
            insideChainSubmission = true;
            const auto chain =
                client->typed().threads().start(std::move(chainOptions), [this](const OperationResult<Thread>& result) {
                expect(!insideChainSubmission && static_cast<bool>(result) && result.value && result.value->id.value == "thread-chain",
                       "first callback in a typed operation chain is asynchronous and successfully decoded");
                ++chainThreadCallbacks;
                if (!result.value) {
                    return;
                }

                insideTurnSubmission = true;
                const auto turn = client->typed().turns().start(
                    result.value->id, {TextInput{"continue the typed chain"}}, {}, [this](const OperationResult<Turn>& turnResult) {
                        expect(!insideTurnSubmission && static_cast<bool>(turnResult) && turnResult.value &&
                                   turnResult.value->id.value == "turn-chain",
                               "typed operation callback can submit another typed operation with asynchronous completion");
                        ++chainTurnCallbacks;
                    });
                insideTurnSubmission = false;
                expect(static_cast<bool>(turn), "typed operation chained from a callback is accepted reentrantly");
            });
            insideChainSubmission = false;
            expect(static_cast<bool>(chain), "first typed operation in the callback chain is accepted");

            state->inject({{"method", "typed/order"}, {"params", {{"future", true}}}, {"extension", "same-raw-message"}});
            state->inject({{"method", "thread/started"}, {"params", Json::array({1})}});
            state->inject({{"method", "typed/throw"}, {"params", Json::object()}});
            injectCoreServerRequests();
            defer([this]() {
                checkCoreCallbacks();
            });
        }

        void injectCoreServerRequests() {
            state->inject({{"method", "item/commandExecution/requestApproval"},
                           {"id", "decision-accept"},
                           {"params", commandApprovalParams("accept")}});
            state->inject({{"method", "item/commandExecution/requestApproval"},
                           {"id", "decision-session"},
                           {"params", commandApprovalParams("session")}});
            state->inject(
                {{"method", "item/fileChange/requestApproval"}, {"id", "decision-decline"}, {"params", fileApprovalParams("decline")}});
            state->inject(
                {{"method", "item/fileChange/requestApproval"}, {"id", "decision-cancel"}, {"params", fileApprovalParams("cancel")}});
            state->inject({{"method", "item/tool/requestUserInput"}, {"id", "user-answers"}, {"params", userInputParams()}});
            state->inject({{"method", "account/chatgptAuthTokens/refresh"},
                           {"id", "auth-response"},
                           {"params", {{"reason", "expired"}, {"previousAccountId", "account-old"}}}});
            state->inject({{"method", "item/commandExecution/requestApproval"},
                           {"id", "retry-response"},
                           {"params", commandApprovalParams("retry")}});
            state->inject({{"method", "future/noAutomaticAnswer"}, {"id", "no-auto-answer"}, {"params", {{"keep", true}}}});
            state->inject({{"method", "future/reject"}, {"id", "typed-reject"}, {"params", {{"reject", true}}}});
            state->inject({{"method", "future/reusedOwnership"}, {"id", "reused-owned-request"}, {"params", {{"generation", 1}}}});
        }

        void handleTypedServerRequest(const TypedServerRequest& request) {
            ++typedServerRequests;
            std::visit(
                [this](const auto& typedRequest) {
                    using Request = std::decay_t<decltype(typedRequest)>;
                    const std::string id = requestIdString(typedRequest.requestId);
                    serverObserverOrder.emplace_back("typed:" + id);

                    if constexpr (std::is_same_v<Request, CommandApprovalRequest>) {
                        if (id == "decision-accept") {
                            const auto sent = client->typed().requests().respond(typedRequest, ApprovalDecision::accept());
                            expect(static_cast<bool>(sent), "accept approval response is accepted reentrantly");
                            const auto second = client->typed().requests().respond(typedRequest, ApprovalDecision::decline());
                            expect(!second && second.error && second.error->category == Error::Category::InvalidState,
                                   "second response attempt for an answered request is rejected locally");
                            secondResponseRejected = true;
                        } else if (id == "decision-session") {
                            const auto sent = client->typed().requests().respond(typedRequest, ApprovalDecision::acceptForSession());
                            expect(static_cast<bool>(sent), "accept-for-session approval response is accepted reentrantly");
                        } else if (id == "retry-response") {
                            state->rejectNextSend = true;
                            const std::size_t before = state->outgoing.size();
                            const auto rejected = client->typed().requests().respond(typedRequest, ApprovalDecision::accept());
                            expect(!rejected && rejected.error && rejected.error->category == Error::Category::Enqueue,
                                   "transport enqueue failure is reported by a typed approval helper");
                            const auto retried = client->typed().requests().respond(typedRequest, ApprovalDecision::accept());
                            expect(static_cast<bool>(retried) && state->outgoing.size() == before + 2,
                                   "failed response enqueue retains server-request ownership for an immediate retry");
                            const auto third = client->typed().requests().respond(typedRequest, ApprovalDecision::accept());
                            expect(!third && state->outgoing.size() == before + 2,
                                   "successful retry consumes ownership and a later response emits no wire message");
                            retryChecksComplete = true;
                        }
                    } else if constexpr (std::is_same_v<Request, FileChangeApprovalRequest>) {
                        const ApprovalDecision decision =
                            id == "decision-decline" ? ApprovalDecision::decline() : ApprovalDecision::cancel();
                        const auto sent = client->typed().requests().respond(typedRequest, decision);
                        expect(static_cast<bool>(sent), "file-change approval response is accepted reentrantly");
                    } else if constexpr (std::is_same_v<Request, UserInputRequest>) {
                        const std::size_t before = state->outgoing.size();
                        const auto invalid =
                            client->typed().requests().respond(typedRequest, {UserInputAnswer{"missing", {"invalid"}}});
                        expect(!invalid && invalid.error && invalid.error->category == Error::Category::Protocol &&
                                   state->outgoing.size() == before,
                               "unknown user-input answer ID is rejected locally without emitting a response");
                        const auto duplicate = client->typed().requests().respond(
                            typedRequest, {UserInputAnswer{"first", {"one"}}, UserInputAnswer{"first", {"duplicate"}}});
                        expect(!duplicate && duplicate.error && duplicate.error->category == Error::Category::Protocol &&
                                   state->outgoing.size() == before,
                               "duplicate user-input answer ID is rejected locally without consuming the request");
                        const auto valid = client->typed().requests().respond(
                            typedRequest, {UserInputAnswer{"first", {"one", "two"}}, UserInputAnswer{"second", {"three"}}});
                        expect(static_cast<bool>(valid),
                               "valid user-input answers are accepted reentrantly after local validation failures");
                        userInputChecksComplete = true;
                    } else if constexpr (std::is_same_v<Request, AuthenticationRequest>) {
                        const auto sent = client->typed().requests().respond(
                            typedRequest, AuthenticationResponse{"access-token", "account-new", std::string("plus")});
                        expect(static_cast<bool>(sent), "authentication response is accepted reentrantly");
                        authenticationResponseComplete = true;
                    } else if constexpr (std::is_same_v<Request, UnknownServerRequest>) {
                        if (id == "no-auto-answer") {
                            unansweredRequest = typedRequest;
                        } else if (id == "typed-reject") {
                            const auto rejected = client->typed().requests().reject(
                                typedRequest, ProtocolError{-32'777, "typed rejection", std::optional<Json>{Json{{"reason", "test"}}}});
                            expect(static_cast<bool>(rejected), "unknown server request can be rejected through the typed facade");
                            typedRejectionComplete = true;
                        } else if (id == "persist-request") {
                            const auto responded = client->typed().requests().respondRaw(typedRequest, Json{{"persisted", true}});
                            expect(static_cast<bool>(responded), "typed server-request handler remains installed across explicit restart");
                            ++persistentTypedRequests;
                        } else if (id == "reused-owned-request") {
                            const auto generation = typedRequest.params.find("generation");
                            if (generation != typedRequest.params.end() && generation->is_number_integer() && *generation == 1) {
                                staleOwnedRequest = typedRequest;
                            } else {
                                const std::size_t outgoingBefore = state->outgoing.size();
                                if (staleOwnedRequest) {
                                    const auto staleResponse =
                                        client->typed().requests().respondRaw(*staleOwnedRequest, Json{{"owner", "stale"}});
                                    expect(!staleResponse && staleResponse.error &&
                                               staleResponse.error->category == Error::Category::InvalidState &&
                                               staleResponse.error->code == ESTALE && state->outgoing.size() == outgoingBefore,
                                           "a retained stale typed request cannot answer a new same-ID request after restart");
                                } else {
                                    expect(false, "the first-generation typed request is retained for the ownership-token check");
                                }

                                const auto freshResponse =
                                    client->typed().requests().respondRaw(typedRequest, Json{{"owner", "fresh"}});
                                expect(static_cast<bool>(freshResponse) && state->outgoing.size() == outgoingBefore + 1,
                                       "the fresh same-ID typed request remains answerable after stale ownership is rejected");
                                staleOwnershipCheckComplete = true;
                            }
                        }
                    }
                },
                request);
        }

        std::size_t responseCount(const Json& id) const {
            std::size_t count = 0;
            for (const Json& message : state->outgoing) {
                const auto method = message.find("method");
                const auto responseId = message.find("id");
                if (method == message.end() && responseId != message.end() && *responseId == id) {
                    ++count;
                }
            }
            return count;
        }

        bool hasResultResponse(const Json& id, const Json& expectedResult) const {
            for (const Json& message : state->outgoing) {
                const auto method = message.find("method");
                const auto responseId = message.find("id");
                const auto result = message.find("result");
                if (method == message.end() && responseId != message.end() && *responseId == id && result != message.end() &&
                    *result == expectedResult) {
                    return true;
                }
            }
            return false;
        }

        bool hasErrorResponse(const Json& id, const Json& expectedError) const {
            for (const Json& message : state->outgoing) {
                const auto method = message.find("method");
                const auto responseId = message.find("id");
                const auto error = message.find("error");
                if (method == message.end() && responseId != message.end() && *responseId == id && error != message.end() &&
                    *error == expectedError) {
                    return true;
                }
            }
            return false;
        }

        bool coreCallbacksComplete() const {
            return malformedCallbacks == 1 && remoteErrorCallbacks == 1 && chainThreadCallbacks == 1 && chainTurnCallbacks == 1 &&
                   eventObserverOrder.size() == 2 && malformedStructuredEvents == 1 && throwingEventCallbacks == 1 &&
                   rawAfterThrowEvents == 1 && typedServerRequests == 10 && rawServerRequests == 10 && secondResponseRejected &&
                   retryChecksComplete && userInputChecksComplete && authenticationResponseComplete && typedRejectionComplete &&
                   unansweredRequest.has_value() && staleOwnedRequest.has_value();
        }

        void checkCoreCallbacks() {
            if (!coreCallbacksComplete() && ++coreCallbackPolls < 12) {
                defer([this]() {
                    checkCoreCallbacks();
                });
                return;
            }

            expect(coreCallbacksComplete(), "all core typed protocol callbacks complete deterministically");
            expect(localSubmissionCallbacks == 0, "local typed submission failure never invokes its result callback");
            expect(eventObserverOrder == std::vector<std::string>({"typed", "raw"}),
                   "typed event observer runs before the coexisting raw observer for the same notification");
            expect(throwingEventCallbacks == 1 && rawAfterThrowEvents == 1,
                   "typed event callback exception is contained and does not suppress the later raw observer");
            expect(serverObserverOrder.size() >= 2 && serverObserverOrder[0] == "typed:decision-accept" &&
                       serverObserverOrder[1] == "raw:decision-accept",
                   "typed and raw server-request observers coexist in deterministic typed-then-raw order");

            expect(hasResultResponse("decision-accept", Json{{"decision", "accept"}}),
                   "accept decision helper emits the exact schema response");
            expect(hasResultResponse("decision-session", Json{{"decision", "acceptForSession"}}),
                   "accept-for-session decision helper emits the exact schema response");
            expect(hasResultResponse("decision-decline", Json{{"decision", "decline"}}),
                   "decline decision helper emits the exact schema response");
            expect(hasResultResponse("decision-cancel", Json{{"decision", "cancel"}}),
                   "cancel decision helper emits the exact schema response");
            expect(hasResultResponse(
                       "user-answers",
                       Json{{"answers",
                             {{"first", {{"answers", Json::array({"one", "two"})}}}, {"second", {{"answers", Json::array({"three"})}}}}}}),
                   "typed user-input response emits the exact question-ID answer map");
            expect(
                hasResultResponse("auth-response",
                                  Json{{"accessToken", "access-token"}, {"chatgptAccountId", "account-new"}, {"chatgptPlanType", "plus"}}),
                "typed authentication response emits the exact schema payload");
            expect(responseCount("retry-response") == 2 && hasResultResponse("retry-response", Json{{"decision", "accept"}}),
                   "response enqueue failure and successful retry use the same exact approval wire payload");
            expect(
                hasErrorResponse("typed-reject", Json{{"code", -32'777}, {"message", "typed rejection"}, {"data", {{"reason", "test"}}}}),
                "typed rejection emits the exact protocol error response");
            expect(responseCount("no-auto-answer") == 0, "receiving a typed unknown server request never emits an automatic answer");

            if (unansweredRequest) {
                const auto rawResponse =
                    client->typed().requests().respondRaw(*unansweredRequest, Json{{"raw", Json::array({1, 2})}});
                expect(static_cast<bool>(rawResponse) && hasResultResponse("no-auto-answer", Json{{"raw", Json::array({1, 2})}}),
                       "unknown typed server request remains answerable with an exact raw response");
            } else {
                expect(false, "unknown typed server request remains available for an explicit raw response");
            }

            beginGenerationChecks();
        }

        void beginGenerationChecks() {
            const auto stale = client->typed().threads().read(ThreadId{"thread-old"}, [this](const OperationResult<Thread>&) {
                ++staleCompletionCallbacks;
            });
            expect(static_cast<bool>(stale), "generation-safety typed operation is accepted");
            if (!stale || !stale.id) {
                coreFinalStop = true;
                client->stop();
                return;
            }

            state->inject({{"id", stale.id->value()}, {"result", {{"thread", threadValue("thread-old")}}}});
            expect(staleCompletionCallbacks == 0, "matched typed completion remains asynchronous before restart");
            client->stop();
            client->start();
        }

        void launchRestartChecks() {
            state->inject({{"method", "typed/persist"}, {"params", {{"generation", 2}}}});
            state->inject({{"method", "future/persist"}, {"id", "persist-request"}, {"params", {{"generation", 2}}}});
            state->inject({{"method", "future/reusedOwnership"}, {"id", "reused-owned-request"}, {"params", {{"generation", 2}}}});
            defer([this]() {
                checkRestartCallbacks();
            });
        }

        void checkRestartCallbacks() {
            const bool ready = cancellationCallbacks == 1 && persistentTypedEvents == 1 && persistentRawEvents == 1 &&
                               persistentTypedRequests == 1 && staleOwnershipCheckComplete;
            if (!ready && ++restartCallbackPolls < 10) {
                defer([this]() {
                    checkRestartCallbacks();
                });
                return;
            }

            expect(cancellationCallbacks == 1, "typed pending operation cancellation occurs exactly once across restart");
            expect(staleCompletionCallbacks == 0, "old-generation normal typed completion is suppressed after immediate explicit restart");
            expect(persistentTypedEvents == 1 && persistentRawEvents == 1 && persistentTypedRequests == 1,
                   "typed handlers and coexisting raw observer remain installed across explicit restart");
            expect(hasResultResponse("persist-request", Json{{"persisted", true}}),
                   "post-restart typed request handler answers through the current generation registry");
            expect(responseCount("reused-owned-request") == 1 && hasResultResponse("reused-owned-request", Json{{"owner", "fresh"}}),
                   "only the fresh occurrence of a reused server-request ID is answered after restart");

            defer([this]() {
                expect(cancellationCallbacks == 1, "typed cancellation remains exactly once after an additional event-loop tick");
                expect(staleCompletionCallbacks == 0,
                       "stale normal typed completion remains suppressed after an additional event-loop tick");
                coreFinalStop = true;
                client->stop();
            });
        }

        void beginStopFromEventScenario() {
            state = std::make_shared<FakeTransportState>();
            installStandardSendHook();
            client = std::make_unique<TestClient>(state);
            client->typed().events().setOnEvent([this](const Event&) {
                ++stopEventCallbacks;
                client->stop();
            });
            client->raw().setOnNotification([this](const Notification&) {
                ++rawAfterStopEvents;
            });
            client->setOnStateChanged([this](const StateChange& change) {
                if (change.current == State::Ready) {
                    state->inject({{"method", "typed/stop"}, {"params", Json::object()}});
                } else if (change.current == State::Failed) {
                    expect(false, "event-callback stop scenario must not fail the connection");
                    client->stop();
                } else if (change.current == State::Stopped && stopEventCallbacks == 1) {
                    expect(rawAfterStopEvents == 0,
                           "typed event callback can stop safely and suppresses later generation-bound raw delivery");
                    defer([this]() {
                        client.reset();
                        beginDestroyFromOperationScenario();
                    });
                }
            });
            client->start();
        }

        void beginDestroyFromOperationScenario() {
            state = std::make_shared<FakeTransportState>();
            installStandardSendHook();
            client = std::make_unique<TestClient>(state);
            client->setOnStateChanged([this](const StateChange& change) {
                if (change.current == State::Ready) {
                    ThreadStartOptions options;
                    options.cwd = "/destroy";
                    const auto submission =
                        client->typed().threads().start(std::move(options), [this](const OperationResult<Thread>& result) {
                        expect(static_cast<bool>(result) && result.value && result.value->id.value == "thread-destroy",
                               "typed operation completion receives its value before destroying the parent client");
                        ++destroyOperationCallbacks;
                        client.reset();
                        defer([this]() {
                            expect(destroyOperationCallbacks == 1, "typed operation completion can destroy its parent client safely");
                            beginDestroyFromServerRequestScenario();
                        });
                    });
                    expect(static_cast<bool>(submission), "operation-destruction typed request is accepted");
                } else if (change.current == State::Failed) {
                    expect(false, "operation-callback destruction scenario must not fail the connection");
                    client->stop();
                }
            });
            client->start();
        }

        void beginDestroyFromServerRequestScenario() {
            state = std::make_shared<FakeTransportState>();
            installStandardSendHook();
            client = std::make_unique<TestClient>(state);
            client->typed().requests().setOnRequest([this](const TypedServerRequest&) {
                ++destroyServerRequestCallbacks;
                const std::size_t outgoingBefore = state->outgoing.size();
                client.reset();
                expect(state->outgoing.size() == outgoingBefore,
                       "destroying the client in a typed server-request callback does not implicitly answer it");
                defer([this]() {
                    expect(destroyServerRequestCallbacks == 1, "typed server-request callback can destroy its parent client safely");
                    expect(rawAfterDestroyServerRequests == 0,
                           "raw observer queued after a destructive typed server-request callback is suppressed safely");
                    beginDestroyFromEventScenario();
                });
            });
            client->raw().setOnServerRequest([this](const ServerRequest&) {
                ++rawAfterDestroyServerRequests;
            });
            client->setOnStateChanged([this](const StateChange& change) {
                if (change.current == State::Ready) {
                    state->inject({{"method", "future/destroyRequest"}, {"id", "destroy-server-request"}, {"params", Json::object()}});
                } else if (change.current == State::Failed) {
                    expect(false, "server-request-callback destruction scenario must not fail the connection");
                    client->stop();
                }
            });
            client->start();
        }

        void beginDestroyFromEventScenario() {
            state = std::make_shared<FakeTransportState>();
            installStandardSendHook();
            client = std::make_unique<TestClient>(state);
            client->typed().events().setOnEvent([this](const Event&) {
                ++destroyEventCallbacks;
                client.reset();
                defer([this]() {
                    expect(destroyEventCallbacks == 1, "typed event callback can destroy its parent client safely");
                    expect(rawAfterDestroyEvents == 0,
                           "raw observer queued after a destructive typed callback never accesses destroyed client state");
                    finish();
                });
            });
            client->raw().setOnNotification([this](const Notification&) {
                ++rawAfterDestroyEvents;
            });
            client->setOnStateChanged([this](const StateChange& change) {
                if (change.current == State::Ready) {
                    state->inject({{"method", "typed/destroy"}, {"params", Json::object()}});
                } else if (change.current == State::Failed) {
                    expect(false, "event-callback destruction scenario must not fail the connection");
                    client->stop();
                }
            });
            client->start();
        }

        void finish() {
            finished = true;
            client.reset();
            core::SNodeC::stop();
        }

        tests::support::TestResult& testResult;
        std::shared_ptr<FakeTransportState> state;
        std::unique_ptr<TestClient> client;

        const Json malformedThreadResult = {{"malformed", Json::array({1, 2, 3})}, {"future", true}};
        std::optional<UnknownServerRequest> unansweredRequest;
        std::optional<UnknownServerRequest> staleOwnedRequest;
        std::vector<std::string> eventObserverOrder;
        std::vector<std::string> serverObserverOrder;

        std::size_t localSubmissionCallbacks = 0;
        std::size_t malformedCallbacks = 0;
        std::size_t remoteErrorCallbacks = 0;
        std::size_t chainThreadCallbacks = 0;
        std::size_t chainTurnCallbacks = 0;
        std::size_t cancellationCallbacks = 0;
        std::size_t staleCompletionCallbacks = 0;
        std::size_t throwingEventCallbacks = 0;
        std::size_t malformedStructuredEvents = 0;
        std::size_t rawAfterThrowEvents = 0;
        std::size_t persistentTypedEvents = 0;
        std::size_t persistentRawEvents = 0;
        std::size_t typedServerRequests = 0;
        std::size_t rawServerRequests = 0;
        std::size_t persistentTypedRequests = 0;
        std::size_t stopEventCallbacks = 0;
        std::size_t rawAfterStopEvents = 0;
        std::size_t destroyEventCallbacks = 0;
        std::size_t rawAfterDestroyEvents = 0;
        std::size_t destroyOperationCallbacks = 0;
        std::size_t destroyServerRequestCallbacks = 0;
        std::size_t rawAfterDestroyServerRequests = 0;
        std::size_t coreReadyCount = 0;
        std::size_t coreCallbackPolls = 0;
        std::size_t restartCallbackPolls = 0;
        bool secondResponseRejected = false;
        bool retryChecksComplete = false;
        bool userInputChecksComplete = false;
        bool authenticationResponseComplete = false;
        bool typedRejectionComplete = false;
        bool staleOwnershipCheckComplete = false;
        bool insideMalformedSubmission = false;
        bool insideRemoteSubmission = false;
        bool insideChainSubmission = false;
        bool insideTurnSubmission = false;
        bool coreFinalStop = false;
        bool finished = false;
    };
} // namespace

int main(int argc, char* argv[]) {
    tests::support::TestResult testResult;
    int result = tests::support::cTestSkipReturnCode;

    if (tests::support::shouldSkipRootWithoutSNodeCGroup()) {
        tests::support::printRootWithoutSNodeCGroupSkipMessage("CodexTypedProtocolEngineTest");
    } else {
        core::SNodeC::init(argc, argv);

        bool timedOut = false;
        TypedProtocolRunner runner(testResult);
        [[maybe_unused]] core::timer::Timer watchdog = core::timer::Timer::singleshotTimer(
            [&timedOut]() {
                timedOut = true;
                core::SNodeC::stop();
            },
            utils::Timeval({12, 0}));

        runner.start();
        const int startResult = core::SNodeC::start(utils::Timeval({14, 0}));

        testResult.expectTrue(!timedOut, "typed protocol-engine scenarios complete before the watchdog");
        testResult.expectTrue(runner.isFinished(), "all deterministic typed protocol-engine scenarios complete");
        testResult.expectEqual(0, startResult, "typed protocol-engine event loop stops cleanly");

        core::SNodeC::free();
        result = testResult.processResult();
    }

    return result;
}
