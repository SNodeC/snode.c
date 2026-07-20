/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include "ai/openai/codex/AppServerClient.h"
#include "ai/openai/codex/detail/Transport.h"
#include "core/EventReceiver.h"
#include "core/SNodeC.h"
#include "core/timer/Timer.h"
#include "support/TestResult.h"
#include "utils/Timeval.h"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace {
    using ai::openai::codex::AppServerClient;
    using ai::openai::codex::ClientRequestId;
    using ai::openai::codex::Diagnostic;
    using ai::openai::codex::Error;
    using ai::openai::codex::Json;
    using ai::openai::codex::Notification;
    using ai::openai::codex::ProtocolError;
    using ai::openai::codex::Response;
    using ai::openai::codex::ServerRequest;
    using ai::openai::codex::ServerRequestId;
    using ai::openai::codex::State;
    using ai::openai::codex::StateChange;
    using ai::openai::codex::UnknownMessage;
    using ai::openai::codex::detail::ProcessExit;
    using ai::openai::codex::detail::Transport;
    using ai::openai::codex::detail::TransportCallbacks;

    struct FakeTransportState {
        TransportCallbacks callbacks;
        std::vector<TransportCallbacks> callbackGenerations;
        std::vector<Json> outgoing;
        std::function<void(const Json&, const TransportCallbacks&)> sendHook;
        bool rejectNextSend = false;
        bool failStartup = false;
        bool running = false;
        std::size_t startCount = 0;
        std::size_t stopCount = 0;

        void inject(const Json& message) const {
            if (callbacks.onMessage) {
                callbacks.onMessage(message.dump());
            }
        }

        void inject(std::size_t generation, const Json& message) const {
            if (generation < callbackGenerations.size() && callbackGenerations[generation].onMessage) {
                callbackGenerations[generation].onMessage(message.dump());
            }
        }

        void fail(Error error) const {
            if (callbacks.onError) {
                callbacks.onError(std::move(error));
            }
        }

        void exit(ProcessExit status) {
            running = false;
            if (callbacks.onExited) {
                callbacks.onExited(status);
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
            if (state->failStartup) {
                if (state->callbacks.onError) {
                    state->callbacks.onError({Error::Category::Launch, 17, "deterministic startup failure"});
                }
                return;
            }

            state->running = true;
            if (state->callbacks.onStarted) {
                state->callbacks.onStarted();
            }
        }

        bool send(std::string message) override {
            const Json parsed = Json::parse(message);
            state->outgoing.push_back(parsed);

            const bool accepted = !std::exchange(state->rejectNextSend, false);
            if (accepted && state->sendHook) {
                const TransportCallbacks callbacks = state->callbacks;
                state->sendHook(parsed, callbacks);
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
                state->callbacks.onExited({true, 0, false, 0});
            }
        }

    private:
        std::shared_ptr<FakeTransportState> state;
    };

    class TestClient final : public AppServerClient {
    public:
        explicit TestClient(const std::shared_ptr<FakeTransportState>& state)
            : AppServerClient(std::make_unique<FakeTransport>(state), {"protocol_engine_test", "Protocol Engine Test", "1"}) {
        }
    };

    void inject(const TransportCallbacks& callbacks, const Json& message) {
        if (callbacks.onMessage) {
            callbacks.onMessage(message.dump());
        }
    }

    bool hasIntegerServerId(const ServerRequestId& id, std::int64_t expected) {
        return std::holds_alternative<std::int64_t>(id.value()) && std::get<std::int64_t>(id.value()) == expected;
    }

    bool hasStringServerId(const ServerRequestId& id, const std::string& expected) {
        return std::holds_alternative<std::string>(id.value()) && std::get<std::string>(id.value()) == expected;
    }

    enum class NotifyInvalidation { Error, Exit, MalformedMessage };

    class ProtocolEngineRunner {
    public:
        explicit ProtocolEngineRunner(tests::support::TestResult& testResult)
            : testResult(testResult) {
        }

        void start() {
            beginFunctionalScenario();
        }

        bool isFinished() const noexcept {
            return finished;
        }

    private:
        void expect(bool condition, const std::string& message) {
            testResult.expectTrue(condition, message);
        }

        static void defer(std::function<void()> callback) {
            core::EventReceiver::atNextTick(callback);
        }

        void installStandardSendHook(bool queuePreReadyMessages = false,
                                     bool overflowPreReadyQueue = false,
                                     bool duplicateDuringServerAnswer = false,
                                     bool failDuringInitializedSend = false) {
            state->sendHook = [queuePreReadyMessages, overflowPreReadyQueue, duplicateDuringServerAnswer, failDuringInitializedSend](
                                  const Json& message, const TransportCallbacks& callbacks) {
                if (!message.contains("method") || !message["method"].is_string()) {
                    if (duplicateDuringServerAnswer && message.value("id", Json()) == Json("answer-in-flight") &&
                        message.contains("result")) {
                        inject(callbacks,
                               {{"method", "server/answer-in-flight"}, {"id", "answer-in-flight"}, {"params", {{"duplicate", true}}}});
                    }
                    return;
                }

                const std::string method = message["method"].get<std::string>();
                if (method == "initialize") {
                    if (overflowPreReadyQueue) {
                        for (std::size_t index = 0; index < 1025; ++index) {
                            inject(callbacks, {{"method", "pre/overflow"}, {"params", {{"index", index}}}, {"extension", "preserved"}});
                        }
                    } else if (queuePreReadyMessages) {
                        inject(callbacks, {{"method", "pre/one"}, {"params", Json::array({1, false})}, {"extension", "first"}});
                        inject(callbacks, {{"id", "pre-unmatched"}, {"result", nullptr}, {"extension", "middle"}});
                        inject(callbacks, {{"method", "pre/two"}, {"params", "second"}, {"extension", "last"}});
                    }

                    inject(callbacks,
                           {{"id", message["id"]},
                            {"result",
                             {{"codexHome", "/tmp/codex-home"},
                              {"platformFamily", "unix"},
                              {"platformOs", "linux"},
                              {"userAgent", "codex-test/1"},
                              {"future", {{"kept", true}}}}}});
                } else if (method == "sync/one") {
                    inject(callbacks, {{"id", message["id"]}, {"result", {{"value", 1}}}});
                } else if (method == "diagnostic/sync") {
                    if (callbacks.onDiagnostic) {
                        callbacks.onDiagnostic({"synchronous diagnostic"});
                    }
                    inject(callbacks, {{"id", message["id"]}, {"result", "diagnostic-result"}});
                } else if (method == "callback/child") {
                    inject(callbacks, {{"id", message["id"]}, {"result", "child-result"}});
                } else if (method == "remote/error") {
                    inject(callbacks,
                           {{"id", message["id"]},
                            {"error", {{"code", -32000}, {"message", "deterministic remote failure"}, {"data", {{"reason", "example"}}}}}});
                } else if (method == "throw/test") {
                    inject(callbacks, {{"id", message["id"]}, {"result", true}});
                } else if (method == "stop/trigger") {
                    inject(callbacks, {{"id", message["id"]}, {"result", nullptr}});
                } else if (method == "destroy/response") {
                    inject(callbacks, {{"id", message["id"]}, {"result", Json::array({"destroy"})}});
                } else if (method == "initialized" && failDuringInitializedSend && callbacks.onError) {
                    callbacks.onError({Error::Category::Transport, 31, "failure during initialized notification send"});
                }
            };
        }

        void beginFunctionalScenario() {
            state = std::make_shared<FakeTransportState>();
            installStandardSendHook(true);
            client = std::make_unique<TestClient>(state);

            client->raw().setOnNotification([this](const Notification& notification) {
                functionalNotification(notification);
            });
            client->raw().setOnUnknownMessage([this](const UnknownMessage& message) {
                functionalUnknown(message);
            });
            client->raw().setOnServerRequest([this](const ServerRequest& request) {
                functionalServerRequest(request);
            });
            client->setOnDiagnostic([this](const Diagnostic& diagnostic) {
                expect(!insideDiagnosticRequestCall, "a diagnostic emitted synchronously by send is delivered asynchronously");
                expect(diagnostic.message == "synchronous diagnostic", "deferred diagnostic preserves its message");
                synchronousDiagnosticSeen = true;
            });
            client->setOnStateChanged([this](const StateChange& change) {
                functionalStateChanged(change);
            });
            client->start();
        }

        void functionalStateChanged(const StateChange& change) {
            if (change.current == State::Ready) {
                if (functionalGeneration == 1) {
                    firstGenerationReady = true;
                    maybeStartFunctionalRequests();
                } else {
                    beginRestartChecks();
                }
                return;
            }

            if (change.current == State::Failed) {
                expect(functionalGeneration == 2 && awaitingProcessFailure,
                       "only the deliberate second-generation process exit fails the functional client");
                expect(change.error && change.error->category == Error::Category::Process && change.error->code == 23,
                       "unexpected process exit reports its process category and exit status");
                expect(exitCancellationSeen, "process exit cancels the pending request before Failed is delivered");
                client->stop();
                return;
            }

            if (change.current != State::Stopped) {
                return;
            }

            if (functionalGeneration == 1) {
                expect(synchronousDiagnosticSeen && synchronousDiagnosticResponseSeen,
                       "synchronous transport diagnostic and correlated response are both delivered before stop completes");
                expect(cancellationMethods == std::vector<std::string>({"cancel/one", "cancel/two", "cancel/three"}),
                       "stop cancellation callbacks run exactly once in deterministic submission order");
                expect(cancellationCallbacks == 3, "all caller-owned requests are cancelled exactly once");
                expect(!sentAnswerForDisconnectedServerRequest,
                       "disconnect clears a pending server request without sending an implicit acceptance");

                functionalGeneration = 2;
                client->start();
            } else {
                expect(awaitingProcessFailure && exitCancellationSeen,
                       "the restarted connection reaches explicit Failed-to-Stopped recovery");
                defer([this]() {
                    client.reset();
                    beginResponseDestructionScenario();
                });
            }
        }

        void functionalNotification(const Notification& notification) {
            if (notification.method == "pre/one") {
                expect(notification.params == Json::array({1, false}) && notification.raw["extension"] == "first",
                       "first pre-ready notification preserves arbitrary params and its raw envelope");
                preReadyOrder.push_back("notification-one");
                maybeStartFunctionalRequests();
            } else if (notification.method == "pre/two") {
                expect(notification.params == "second" && notification.raw["extension"] == "last",
                       "second pre-ready notification preserves scalar params and unknown fields");
                preReadyOrder.push_back("notification-two");
                maybeStartFunctionalRequests();
            } else if (notification.method == "future/unknown") {
                expect(notification.params == Json::array({true, nullptr}) && notification.raw["futureField"] == 9,
                       "unknown notification methods are delivered without classification and preserve raw JSON");
                futureNotificationSeen = true;
                maybeBeginServerRequestChecks();
            } else if (notification.method == "after/throw") {
                exceptionBoundarySurvived = true;
                beginCancellationChecks();
            } else if (notification.method == "restart/current") {
                expect(functionalGeneration == 2, "installed notification handler survives restart");
                ++currentGenerationNotificationCount;
                restartWireOrder.push_back("notification");
            } else if (notification.method == "restart/stale") {
                staleGenerationMessageDelivered = true;
            }
        }

        void functionalUnknown(const UnknownMessage& message) {
            if (message.raw.value("id", Json()) == Json("pre-unmatched")) {
                expect(message.reason.find("string response ID") != std::string::npos,
                       "queued string-ID response receives a useful unmatched reason");
                expect(message.raw["extension"] == "middle", "queued unmatched response preserves its complete raw envelope");
                preReadyOrder.push_back("unknown-middle");
                maybeStartFunctionalRequests();
            } else if (rejectedRequestWireId && message.raw.value("id", Json()) == Json(*rejectedRequestWireId)) {
                rollbackUnknownSeen = true;
                maybeBeginServerRequestChecks();
            } else if (message.raw.value("id", Json()) == Json(9191)) {
                expect(message.reason.find("does not match") != std::string::npos,
                       "uncorrelated integer response reports that it has no pending owner");
                unknownIntegerResponseSeen = true;
                maybeBeginServerRequestChecks();
            } else if (message.raw.value("id", Json()) == Json("future-response-id")) {
                expect(message.reason.find("string response ID") != std::string::npos,
                       "string response ID is valid but cannot correlate with a client request");
                unknownStringResponseSeen = true;
                maybeBeginServerRequestChecks();
            }
        }

        void maybeStartFunctionalRequests() {
            if (functionalRequestsStarted || !firstGenerationReady || preReadyOrder.size() != 3) {
                return;
            }

            functionalRequestsStarted = true;
            expect(preReadyOrder == std::vector<std::string>({"notification-one", "unknown-middle", "notification-two"}),
                   "pre-ready messages are dispatched asynchronously in original wire order");
            expect(state->outgoing.size() >= 2 && state->outgoing[0]["method"] == "initialize" &&
                       state->outgoing[1]["method"] == "initialized",
                   "initialize request and initialized notification remain internally owned and wire ordered");

            const std::optional<ai::openai::codex::InitializeResult> initializeResult = client->getInitializeResult();
            expect(initializeResult && initializeResult->codexHome == "/tmp/codex-home" && initializeResult->platformFamily == "unix" &&
                       initializeResult->platformOs == "linux" && initializeResult->userAgent == "codex-test/1",
                   "typed initialization fields are cached after the internal handshake");
            expect(initializeResult && initializeResult->raw["future"]["kept"] == true,
                   "the complete raw initialization result retains future fields");

            const auto emptyMethod = client->raw().request("", Json::object(), [](const Response&) {
            });
            expect(!emptyMethod && emptyMethod.error && emptyMethod.error->category == Error::Category::Protocol,
                   "an empty raw request method is rejected with a local protocol error");
            const auto missingHandler = client->raw().request("missing/handler", Json::object(), {});
            expect(!missingHandler && missingHandler.error && missingHandler.error->category == Error::Category::Protocol,
                   "a raw request without a response handler is rejected locally");
            const auto reservedRequest = client->raw().request("initialize", Json::object(), [](const Response&) {
            });
            expect(!reservedRequest && reservedRequest.error && reservedRequest.error->category == Error::Category::InvalidState,
                   "raw callers cannot submit the internally owned initialize request");
            const auto reservedNotification = client->raw().notify("initialized");
            expect(!reservedNotification && reservedNotification.error &&
                       reservedNotification.error->category == Error::Category::InvalidState,
                   "raw callers cannot submit the internally owned initialized notification");
            expect(!client->raw().respond(ServerRequestId(404), nullptr), "respond accepts only a currently pending server-request ID");

            state->rejectNextSend = true;
            const auto rejectedNotification = client->raw().notify("client/rejected", Json::array({1}));
            expect(!rejectedNotification && rejectedNotification.error && rejectedNotification.error->category == Error::Category::Enqueue,
                   "notification transport rejection is observable to the caller");
            const auto acceptedNotification = client->raw().notify("client/accepted", false);
            expect(acceptedNotification && state->outgoing.back()["method"] == "client/accepted" &&
                       state->outgoing.back()["params"] == false,
                   "accepted notifications preserve arbitrary params in the outgoing document");

            state->rejectNextSend = true;
            const auto rejectedRequest = client->raw().request("send/rejected", Json::object(), [](const Response&) {
            });
            expect(!rejectedRequest && rejectedRequest.error && rejectedRequest.error->category == Error::Category::Enqueue,
                   "transport rejection rolls back a raw request submission");
            rejectedRequestWireId = state->outgoing.back()["id"].get<std::int64_t>();
            state->inject({{"id", *rejectedRequestWireId}, {"result", "late"}, {"rollbackProbe", true}});

            insideDiagnosticRequestCall = true;
            const auto diagnosticRequest = client->raw().request("diagnostic/sync", Json::object(), [this](const Response& response) {
                expect(response.kind == Response::Kind::Result && response.result == "diagnostic-result",
                       "request response remains correlated when send also emits a synchronous diagnostic");
                synchronousDiagnosticResponseSeen = true;
            });
            insideDiagnosticRequestCall = false;
            expect(static_cast<bool>(diagnosticRequest), "request that synchronously emits a diagnostic is accepted");

            insideRequestCall = true;
            const auto synchronous = client->raw().request("sync/one", Json::object(), [this](const Response& response) {
                expect(!insideRequestCall, "a response injected synchronously by send is delivered asynchronously");
                expect(response.kind == Response::Kind::Result && response.method == "sync/one" && response.result == Json{{"value", 1}},
                       "a client request correlates its result and retains its original method");

                const auto child = client->raw().request("callback/child", Json::object(), [this](const Response& childResponse) {
                    expect(childResponse.kind == Response::Kind::Result && childResponse.result == "child-result",
                           "a response callback can reentrantly submit and complete another request");
                    beginOutOfOrderChecks();
                });
                expect(static_cast<bool>(child), "a response callback may reentrantly submit another request");
            });
            insideRequestCall = false;
            expect(static_cast<bool>(synchronous), "request remains accepted when the fake injects its response from send");
        }

        void beginOutOfOrderChecks() {
            const auto first = client->raw().request("order/first", Json::object(), [this](const Response& response) {
                expect(response.method == "order/first" && response.result == 1,
                       "first submitted request receives its own out-of-order result");
                outOfOrderCallbacks.push_back("first");
                finishOutOfOrderChecks();
            });
            const auto second = client->raw().request("order/second", Json::object(), [this](const Response& response) {
                expect(response.method == "order/second" && response.result == Json::array({2}),
                       "second submitted request receives its own out-of-order result");
                outOfOrderCallbacks.push_back("second");
                finishOutOfOrderChecks();
            });
            expect(first && second, "multiple raw requests can remain pending concurrently");
            if (!first || !second) {
                return;
            }

            state->inject({{"id", second.id->value()}, {"result", Json::array({2})}});
            state->inject({{"id", first.id->value()}, {"result", 1}});
        }

        void finishOutOfOrderChecks() {
            if (outOfOrderCallbacks.size() != 2) {
                return;
            }
            expect(outOfOrderCallbacks == std::vector<std::string>({"second", "first"}),
                   "out-of-order responses dispatch in incoming wire order");

            const auto remoteError = client->raw().request("remote/error", nullptr, [this](const Response& response) {
                expect(response.kind == Response::Kind::RemoteError && response.method == "remote/error" && response.remoteError &&
                           response.remoteError->code == -32000 && response.remoteError->message == "deterministic remote failure" &&
                           response.remoteError->data && *response.remoteError->data == Json{{"reason", "example"}} && !response.localError,
                       "remote protocol errors remain separate from local errors and preserve arbitrary data");

                state->inject({{"id", 9191}, {"result", Json::object()}, {"future", true}});
                state->inject({{"id", "future-response-id"}, {"result", false}});
                state->inject({{"method", "future/unknown"}, {"params", Json::array({true, nullptr})}, {"futureField", 9}});
            });
            expect(static_cast<bool>(remoteError), "remote-error request is accepted");
        }

        void maybeBeginServerRequestChecks() {
            if (serverRequestChecksStarted || !rollbackUnknownSeen || !unknownIntegerResponseSeen || !unknownStringResponseSeen ||
                !futureNotificationSeen) {
                return;
            }

            serverRequestChecksStarted = true;
            state->inject({{"method", "server/integer"}, {"id", 77}, {"params", {{"question", "continue?"}}}, {"extension", true}});
        }

        void functionalServerRequest(const ServerRequest& request) {
            if (request.method == "server/integer") {
                expect(hasIntegerServerId(request.id, 77) && request.params["question"] == "continue?" && request.raw["extension"] == true,
                       "integer server-request ID, params, and raw envelope are preserved exactly");
                const auto response = client->raw().respond(request.id, {{"decision", "accept"}});
                expect(response && state->outgoing.back()["id"] == 77 && state->outgoing.back()["result"] == Json{{"decision", "accept"}},
                       "server request can be answered reentrantly with the exact integer ID");
                expect(!client->raw().respond(request.id, nullptr), "a second response attempt is rejected after the first was accepted");

                state->inject({{"method", "server/string"}, {"id", "approval-123"}, {"params", Json::array({"arbitrary", 2})}});
            } else if (request.method == "server/string") {
                expect(hasStringServerId(request.id, "approval-123") && request.params == Json::array({"arbitrary", 2}),
                       "string server-request IDs and arbitrary params are preserved exactly");

                state->rejectNextSend = true;
                const auto failedResponse = client->raw().respond(request.id, {{"decision", "retry"}});
                expect(!failedResponse && failedResponse.error && failedResponse.error->category == Error::Category::Enqueue,
                       "failed response enqueue remains observable and retains server-request ownership");

                const auto rejection =
                    client->raw().reject(request.id, ProtocolError{-32001, "Request rejected", Json{{"reason", "test rejection"}}});
                expect(rejection && state->outgoing.back()["id"] == "approval-123" && state->outgoing.back()["error"]["code"] == -32001 &&
                           state->outgoing.back()["error"]["message"] == "Request rejected" &&
                           state->outgoing.back()["error"]["data"] == Json{{"reason", "test rejection"}},
                       "retry after enqueue failure can reject with exact string ID and remote error data");
                expect(!client->raw().reject(request.id, ProtocolError{-1, "again", std::nullopt}),
                       "a second rejection attempt fails after ownership was consumed");

                const auto throwing = client->raw().request("throw/test", Json::object(), [](const Response&) {
                    throw std::runtime_error("intentional public callback exception");
                });
                expect(static_cast<bool>(throwing), "request with a throwing user callback is still accepted");
                defer([this]() {
                    state->inject({{"method", "after/throw"}, {"params", nullptr}});
                });
            } else if (request.method == "server/pending-disconnect") {
                expect(hasStringServerId(request.id, "left-pending"),
                       "pending server request retains its string identity until disconnect");
                disconnectedServerRequestId = request.id;
                const std::size_t outgoingBefore = state->outgoing.size();

                submitCancellationRequest("cancel/one");
                submitCancellationRequest("cancel/two");
                submitCancellationRequest("cancel/three");
                const auto stopTrigger = client->raw().request("stop/trigger", Json::object(), [this](const Response& response) {
                    expect(response.kind == Response::Kind::Result,
                           "response callback that initiates stop first receives its correlated result");
                    client->stop();
                });
                expect(static_cast<bool>(stopTrigger), "request whose response callback calls stop is accepted");

                sentAnswerForDisconnectedServerRequest = false;
                for (std::size_t index = outgoingBefore; index < state->outgoing.size(); ++index) {
                    const Json& outgoing = state->outgoing[index];
                    if (outgoing.value("id", Json()) == Json("left-pending") &&
                        (outgoing.contains("result") || outgoing.contains("error"))) {
                        sentAnswerForDisconnectedServerRequest = true;
                    }
                }
            }
        }

        void beginCancellationChecks() {
            expect(exceptionBoundarySurvived,
                   "an exception escaping a user callback is caught and later event-loop callbacks still execute");
            state->inject({{"method", "server/pending-disconnect"}, {"id", "left-pending"}, {"params", Json::object()}});
        }

        void submitCancellationRequest(const std::string& method) {
            const auto submission = client->raw().request(method, Json::object(), [this](const Response& response) {
                expect(response.kind == Response::Kind::Cancelled && response.localError &&
                           response.localError->category == Error::Category::Cancelled && !response.remoteError,
                       "lifecycle cancellation is reported as a local Cancelled response");
                cancellationMethods.push_back(response.method);
                ++cancellationCallbacks;
            });
            expect(static_cast<bool>(submission), method + " is accepted before stop");
            if (submission) {
                lastFirstGenerationRequestId = submission.id->value();
            }
        }

        void beginRestartChecks() {
            expect(state->startCount == 2 && state->callbackGenerations.size() == 2,
                   "the same transport captures distinct callback generations across restart");
            expect(disconnectedServerRequestId && !client->raw().respond(*disconnectedServerRequestId, nullptr),
                   "disconnect clears pending server-request ownership without clearing installed handlers");

            const auto restarted = client->raw().request("restart/current", Json::object(), [this](const Response& response) {
                expect(response.kind == Response::Kind::Result && response.result == "current" && response.method == "restart/current",
                       "current-generation response completes the restarted request");
                expect(!staleGenerationMessageDelivered && currentGenerationNotificationCount == 1,
                       "old callback generation cannot deliver a stale response or notification");
                restartWireOrder.push_back("response");
                expect(restartWireOrder == std::vector<std::string>({"notification", "response"}),
                       "current-generation notification and response callbacks retain incoming wire order");

                const auto pending = client->raw().request("exit/pending", Json::object(), [this](const Response& cancelled) {
                    expect(cancelled.kind == Response::Kind::Cancelled && cancelled.method == "exit/pending" && cancelled.localError &&
                               cancelled.localError->category == Error::Category::Cancelled,
                           "unexpected process exit cancels a pending restarted request exactly once");
                    exitCancellationSeen = true;
                });
                expect(static_cast<bool>(pending), "a request is pending when the fake process exits");
                awaitingProcessFailure = true;
                state->exit({true, 23, false, 0});
            });
            expect(restarted && restarted.id->value() > lastFirstGenerationRequestId,
                   "client request IDs remain monotonic across an explicit stop and restart");
            if (!restarted) {
                return;
            }

            restartResponseDelivered = false;
            state->inject(0, {{"id", restarted.id->value()}, {"result", "stale"}});
            state->inject(0, {{"method", "restart/stale"}, {"params", Json::object()}});
            defer([this, id = restarted.id->value()]() {
                expect(!restartResponseDelivered && !staleGenerationMessageDelivered,
                       "captured callbacks from the old generation are ignored before current traffic arrives");
                state->inject({{"method", "restart/current"}, {"params", Json::object()}});
                state->inject({{"id", id}, {"result", "current"}});
                restartResponseDelivered = true;
            });
        }

        void beginResponseDestructionScenario() {
            state = std::make_shared<FakeTransportState>();
            installStandardSendHook();
            client = std::make_unique<TestClient>(state);
            client->setOnStateChanged([this](const StateChange& change) {
                if (change.current == State::Ready) {
                    const auto submission = client->raw().request("destroy/response", Json::object(), [this](const Response& response) {
                        expect(response.kind == Response::Kind::Result && response.result == Json::array({"destroy"}),
                               "response callback receives its value before destroying the public client");
                        client.reset();
                        responseCallbackDestroyedClient = true;
                        defer([this]() {
                            beginServerRequestDestructionScenario();
                        });
                    });
                    expect(static_cast<bool>(submission), "response-destruction request is accepted");
                }
            });
            client->start();
        }

        void beginServerRequestDestructionScenario() {
            expect(responseCallbackDestroyedClient, "destroying the client inside a response callback returns safely");
            state = std::make_shared<FakeTransportState>();
            installStandardSendHook();
            client = std::make_unique<TestClient>(state);
            client->raw().setOnServerRequest([this](const ServerRequest& request) {
                expect(hasStringServerId(request.id, "destroy-client"),
                       "server-request callback receives the exact ID before destroying the client");
                const std::size_t outgoingBefore = state->outgoing.size();
                client.reset();
                expect(state->outgoing.size() == outgoingBefore,
                       "destroying the client in a server-request callback does not implicitly answer it");
                serverRequestCallbackDestroyedClient = true;
                defer([this]() {
                    beginStartupFailureScenario();
                });
            });
            client->setOnStateChanged([this](const StateChange& change) {
                if (change.current == State::Ready) {
                    state->inject({{"method", "server/destroy"}, {"id", "destroy-client"}, {"params", Json::object()}});
                }
            });
            client->start();
        }

        void beginStartupFailureScenario() {
            expect(serverRequestCallbackDestroyedClient, "destroying the client inside a server-request callback returns safely");
            state = std::make_shared<FakeTransportState>();
            state->failStartup = true;
            client = std::make_unique<TestClient>(state);
            client->setOnStateChanged([this](const StateChange& change) {
                if (change.current == State::Failed) {
                    expect(change.error && change.error->category == Error::Category::Launch && change.error->code == 17,
                           "fake transport can report deterministic startup failure");
                    client->stop();
                } else if (change.current == State::Stopped) {
                    defer([this]() {
                        client.reset();
                        beginInitializedSendFailureScenario();
                    });
                }
            });
            client->start();
        }

        void beginInitializedSendFailureScenario() {
            state = std::make_shared<FakeTransportState>();
            installStandardSendHook(false, false, false, true);
            client = std::make_unique<TestClient>(state);
            client->setOnStateChanged([this](const StateChange& change) {
                if (change.current == State::Ready) {
                    initializedSendFailureReachedReady = true;
                } else if (change.current == State::Failed) {
                    expect(!initializedSendFailureReachedReady && change.error && change.error->category == Error::Category::Transport &&
                               change.error->code == 31,
                           "synchronous failure during initialized send cannot resurrect initialization into Ready");
                    expect(!client->getInitializeResult(),
                           "failed initialized notification send does not publish an initialization result");
                    client->stop();
                } else if (change.current == State::Stopped) {
                    defer([this]() {
                        client.reset();
                        beginNotifyInvalidationScenario(NotifyInvalidation::Error);
                    });
                }
            });
            client->start();
        }

        void beginNotifyInvalidationScenario(NotifyInvalidation invalidation) {
            state = std::make_shared<FakeTransportState>();
            installStandardSendHook();
            client = std::make_unique<TestClient>(state);
            notifyInvalidationReturnedFailure = false;
            client->setOnStateChanged([this, invalidation](const StateChange& change) {
                if (change.current == State::Ready) {
                    state->sendHook = [invalidation](const Json& message, const TransportCallbacks& callbacks) {
                        if (message.value("method", "") != "notify/transaction") {
                            return;
                        }

                        switch (invalidation) {
                            case NotifyInvalidation::Error:
                                if (callbacks.onError) {
                                    callbacks.onError({Error::Category::Transport, 41, "synchronous failure during notification send"});
                                }
                                break;
                            case NotifyInvalidation::Exit:
                                if (callbacks.onExited) {
                                    callbacks.onExited({true, 42, false, 0});
                                }
                                break;
                            case NotifyInvalidation::MalformedMessage:
                                if (callbacks.onMessage) {
                                    callbacks.onMessage("{malformed-notification-response");
                                }
                                break;
                        }
                    };

                    const auto result = client->raw().notify("notify/transaction", Json::object());
                    notifyInvalidationReturnedFailure =
                        !result && !result.accepted && result.error && result.error->category == Error::Category::Cancelled;
                    expect(notifyInvalidationReturnedFailure,
                           "notification reports cancellation when send synchronously invalidates its connection");
                } else if (change.current == State::Failed) {
                    Error::Category expectedCategory = Error::Category::Protocol;
                    int expectedCode = 0;
                    if (invalidation == NotifyInvalidation::Error) {
                        expectedCategory = Error::Category::Transport;
                        expectedCode = 41;
                    } else if (invalidation == NotifyInvalidation::Exit) {
                        expectedCategory = Error::Category::Process;
                        expectedCode = 42;
                    }
                    expect(notifyInvalidationReturnedFailure && change.error && change.error->category == expectedCategory &&
                               change.error->code == expectedCode,
                           "synchronous notification-send invalidation preserves its fatal lifecycle error");
                    client->stop();
                } else if (change.current == State::Stopped) {
                    defer([this, invalidation]() {
                        client.reset();
                        if (invalidation == NotifyInvalidation::Error) {
                            beginNotifyInvalidationScenario(NotifyInvalidation::Exit);
                        } else if (invalidation == NotifyInvalidation::Exit) {
                            beginNotifyInvalidationScenario(NotifyInvalidation::MalformedMessage);
                        } else {
                            beginTransportFailureScenario();
                        }
                    });
                }
            });
            client->start();
        }

        void beginTransportFailureScenario() {
            state = std::make_shared<FakeTransportState>();
            installStandardSendHook();
            client = std::make_unique<TestClient>(state);
            client->setOnStateChanged([this](const StateChange& change) {
                if (change.current == State::Ready) {
                    const auto pending = client->raw().request("transport/pending", Json::object(), [this](const Response& response) {
                        expect(response.kind == Response::Kind::Cancelled && response.method == "transport/pending",
                               "transport failure cancels a pending client request");
                        ++transportFailureCancellationCount;
                    });
                    expect(static_cast<bool>(pending), "request is accepted before deterministic transport failure");
                    state->fail({Error::Category::Transport, 29, "deterministic active transport failure"});
                } else if (change.current == State::Failed) {
                    expect(change.error && change.error->category == Error::Category::Transport && change.error->code == 29,
                           "active transport failure retains its local category and code");
                    expect(transportFailureCancellationCount == 1,
                           "transport failure cancellation callback runs exactly once before Failed state delivery");
                    client->stop();
                } else if (change.current == State::Stopped) {
                    defer([this]() {
                        client.reset();
                        beginDuplicateServerRequestScenario();
                    });
                }
            });
            client->start();
        }

        void beginDuplicateServerRequestScenario() {
            state = std::make_shared<FakeTransportState>();
            installStandardSendHook();
            client = std::make_unique<TestClient>(state);
            client->setOnStateChanged([this](const StateChange& change) {
                if (change.current == State::Ready) {
                    const Json request = {{"method", "server/duplicate"}, {"id", "duplicate"}, {"params", Json::object()}};
                    state->inject(request);
                    state->inject(request);
                } else if (change.current == State::Failed) {
                    expect(change.error && change.error->category == Error::Category::Protocol,
                           "duplicate still-pending server-request ID is a protocol failure");
                    client->stop();
                } else if (change.current == State::Stopped) {
                    defer([this]() {
                        client.reset();
                        beginAnswerInFlightDuplicateScenario();
                    });
                }
            });
            client->start();
        }

        void beginAnswerInFlightDuplicateScenario() {
            state = std::make_shared<FakeTransportState>();
            installStandardSendHook(false, false, true);
            client = std::make_unique<TestClient>(state);
            client->raw().setOnServerRequest([this](const ServerRequest& request) {
                expect(hasStringServerId(request.id, "answer-in-flight"),
                       "answer-in-flight scenario receives the exact pending server-request ID");
                const auto answer = client->raw().respond(request.id, {{"decision", "accept"}});
                expect(!answer && answer.error && answer.error->category == Error::Category::Cancelled,
                       "answer reports cancellation when an in-flight duplicate makes ownership ambiguous");
                answerInFlightDuplicateObserved = true;
            });
            client->setOnStateChanged([this](const StateChange& change) {
                if (change.current == State::Ready) {
                    state->inject({{"method", "server/answer-in-flight"}, {"id", "answer-in-flight"}, {"params", {{"duplicate", false}}}});
                } else if (change.current == State::Failed) {
                    expect(answerInFlightDuplicateObserved && change.error && change.error->category == Error::Category::Protocol,
                           "duplicate ID injected while respond is in-flight causes a protocol failure, not fresh ownership");
                    client->stop();
                } else if (change.current == State::Stopped) {
                    defer([this]() {
                        client.reset();
                        beginServerCapacityScenario();
                    });
                }
            });
            client->start();
        }

        void beginServerCapacityScenario() {
            state = std::make_shared<FakeTransportState>();
            installStandardSendHook();
            client = std::make_unique<TestClient>(state);
            client->setOnStateChanged([this](const StateChange& change) {
                if (change.current == State::Ready) {
                    for (std::int64_t id = 0; id < 1025 && client->getState() == State::Ready; ++id) {
                        state->inject({{"method", "server/capacity"}, {"id", "capacity-" + std::to_string(id)}, {"params", nullptr}});
                    }
                } else if (change.current == State::Failed) {
                    expect(change.error && change.error->category == Error::Category::Protocol &&
                               change.error->message.find("capacity") != std::string::npos,
                           "pending server-request registry enforces its 1024-entry bound as a protocol failure");
                    client->stop();
                } else if (change.current == State::Stopped) {
                    defer([this]() {
                        client.reset();
                        beginPreReadyCapacityScenario();
                    });
                }
            });
            client->start();
        }

        void beginPreReadyCapacityScenario() {
            state = std::make_shared<FakeTransportState>();
            installStandardSendHook(false, true);
            client = std::make_unique<TestClient>(state);
            client->setOnStateChanged([this](const StateChange& change) {
                if (change.current == State::Ready) {
                    preReadyOverflowReachedReady = true;
                } else if (change.current == State::Failed) {
                    expect(!preReadyOverflowReachedReady && change.error && change.error->category == Error::Category::Protocol &&
                               change.error->message.find("pre-ready") != std::string::npos,
                           "the 1024-entry pre-ready queue fails safely on the next structurally valid message");
                    client->stop();
                } else if (change.current == State::Stopped) {
                    defer([this]() {
                        client.reset();
                        beginClientCapacityScenario();
                    });
                }
            });
            client->start();
        }

        void beginClientCapacityScenario() {
            state = std::make_shared<FakeTransportState>();
            installStandardSendHook();
            client = std::make_unique<TestClient>(state);
            client->setOnStateChanged([this](const StateChange& change) {
                if (change.current == State::Ready) {
                    std::int64_t previousId = -1;
                    bool allAccepted = true;
                    for (std::size_t index = 0; index < 4096; ++index) {
                        const std::string method = "capacity/" + std::to_string(index);
                        const auto submission = client->raw().request(method, Json::object(), [this](const Response& response) {
                            if (response.kind == Response::Kind::Cancelled) {
                                if (capacityCancellationCount == 0) {
                                    firstCapacityCancellationMethod = response.method;
                                }
                                lastCapacityCancellationMethod = response.method;
                                ++capacityCancellationCount;
                            }
                        });
                        allAccepted = allAccepted && static_cast<bool>(submission) && submission.id->value() > previousId;
                        if (submission) {
                            previousId = submission.id->value();
                        }
                    }
                    expect(allAccepted, "pending client-request registry accepts 4096 requests with monotonic IDs");

                    const auto overflow = client->raw().request("capacity/overflow", Json::object(), [](const Response&) {
                    });
                    expect(!overflow && overflow.error && overflow.error->category == Error::Category::Capacity,
                           "the 4097th pending client request fails locally with Capacity and leaves no pending entry");
                    client->stop();
                } else if (change.current == State::Stopped) {
                    expect(capacityCancellationCount == 4096, "stopping at client capacity completes every accepted request exactly once");
                    expect(firstCapacityCancellationMethod == "capacity/0" && lastCapacityCancellationMethod == "capacity/4095",
                           "capacity cancellation callbacks remain deterministically submission ordered");
                    finish();
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

        int functionalGeneration = 1;
        bool firstGenerationReady = false;
        bool functionalRequestsStarted = false;
        bool insideRequestCall = false;
        bool insideDiagnosticRequestCall = false;
        bool synchronousDiagnosticSeen = false;
        bool synchronousDiagnosticResponseSeen = false;
        std::vector<std::string> preReadyOrder;
        std::optional<std::int64_t> rejectedRequestWireId;
        bool rollbackUnknownSeen = false;
        bool unknownIntegerResponseSeen = false;
        bool unknownStringResponseSeen = false;
        bool futureNotificationSeen = false;
        bool serverRequestChecksStarted = false;
        std::vector<std::string> outOfOrderCallbacks;
        bool exceptionBoundarySurvived = false;
        std::optional<ServerRequestId> disconnectedServerRequestId;
        bool sentAnswerForDisconnectedServerRequest = false;
        std::vector<std::string> cancellationMethods;
        std::size_t cancellationCallbacks = 0;
        std::int64_t lastFirstGenerationRequestId = -1;
        bool staleGenerationMessageDelivered = false;
        std::size_t currentGenerationNotificationCount = 0;
        bool restartResponseDelivered = false;
        std::vector<std::string> restartWireOrder;
        bool awaitingProcessFailure = false;
        bool exitCancellationSeen = false;
        bool responseCallbackDestroyedClient = false;
        bool serverRequestCallbackDestroyedClient = false;
        bool initializedSendFailureReachedReady = false;
        bool notifyInvalidationReturnedFailure = false;
        std::size_t transportFailureCancellationCount = 0;
        bool answerInFlightDuplicateObserved = false;
        bool preReadyOverflowReachedReady = false;
        std::size_t capacityCancellationCount = 0;
        std::string firstCapacityCancellationMethod;
        std::string lastCapacityCancellationMethod;
        bool finished = false;
    };
} // namespace

int main(int argc, char* argv[]) {
    tests::support::TestResult testResult;
    int result = tests::support::cTestSkipReturnCode;

    if (tests::support::shouldSkipRootWithoutSNodeCGroup()) {
        tests::support::printRootWithoutSNodeCGroupSkipMessage("CodexProtocolEngineTest");
    } else {
        core::SNodeC::init(argc, argv);

        bool timedOut = false;
        ProtocolEngineRunner runner(testResult);
        [[maybe_unused]] core::timer::Timer watchdog = core::timer::Timer::singleshotTimer(
            [&timedOut]() {
                timedOut = true;
                core::SNodeC::stop();
            },
            utils::Timeval({25, 0}));

        runner.start();
        const int startResult = core::SNodeC::start(utils::Timeval({27, 0}));

        testResult.expectTrue(!timedOut, "protocol-engine scenarios complete before the watchdog");
        testResult.expectTrue(runner.isFinished(), "all deterministic protocol-engine scenarios complete");
        testResult.expectEqual(0, startResult, "protocol-engine event loop stops cleanly");

        core::SNodeC::free();
        result = testResult.processResult();
    }

    return result;
}
