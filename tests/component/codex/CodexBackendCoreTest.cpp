/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include "CodexBackendTestSupport.h"
#include "ai/openai/codex/backend/BackendCore.h"
#include "core/EventReceiver.h"
#include "core/SNodeC.h"
#include "core/timer/Timer.h"
#include "support/TestResult.h"
#include "utils/Timeval.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace {
    namespace backend = ai::openai::codex::backend;
    namespace typed = ai::openai::codex::typed;

    using ai::openai::codex::Diagnostic;
    using ai::openai::codex::Error;
    using ai::openai::codex::Json;
    using ai::openai::codex::detail::TransportCallbacks;

    Json agentItemValue(const std::string& id, const std::string& text = {}) {
        return {{"type", "agentMessage"}, {"id", id}, {"text", text}};
    }

    class BackendCoreRunner {
    public:
        explicit BackendCoreRunner(tests::support::TestResult& result)
            : result(result) {
        }

        void start() {
            transport = std::make_shared<tests::codex::FakeTransportState>();
            tests::codex::installInitializingFake(transport, [this](const Json& message, const TransportCallbacks& callbacks) {
                handleOutgoing(message, callbacks);
            });

            backend::BackendCoreOptions options;
            options.initialThreadListLimit = 2;
            options.maxEventsPerCallback = 32;
            auto appServer = std::make_unique<tests::codex::FakeAppServerClient>(transport);
            appServerClient = appServer.get();
            backendCore = std::make_unique<backend::BackendCore>(std::move(appServer), std::move(options));

            controller = backendCore->openSession(sessionCallbacks(controllerEvents, false));
            observer = backendCore->openSession(sessionCallbacks(observerEvents, true));
            expect(controller.role() == backend::SessionRole::Observer && observer.role() == backend::SessionRole::Observer,
                   "BackendCore sessions start as observers before lifecycle startup");
            expect(static_cast<bool>(controller.submit("acquire-initial", backend::ControllerAcquire{})),
                   "initial controller acquisition is accepted before backend readiness");

            backendCore->start();
            waitUntil(
                "backend reaches Ready and completes exactly one bounded initial refresh",
                [this]() {
                    const backend::Snapshot snapshot = backendCore->snapshot();
                    return backendCore->isReady() && snapshot.threadList.pagesLoaded >= 1 && completionCount("acquire-initial") == 1;
                },
                [this]() {
                    verifyInitialHydrationAndSubmitOperations();
                });
        }

        bool isFinished() const noexcept {
            return finished;
        }

    private:
        struct EventLog {
            std::vector<std::vector<std::uint64_t>> batches;
            std::vector<backend::LifecycleChanged> lifecycles;
            std::size_t snapshots = 0;
        };

        static void defer(std::function<void()> callback) {
            core::EventReceiver::atNextTick(std::move(callback));
        }

        void expect(bool condition, const std::string& message) {
            result.expectTrue(condition, message);
        }

        void
        waitUntil(std::string description, std::function<bool()> predicate, std::function<void()> next, std::size_t remaining = 4'000) {
            defer([this,
                   description = std::move(description),
                   predicate = std::move(predicate),
                   next = std::move(next),
                   remaining]() mutable {
                if (finished) {
                    return;
                }
                if (predicate()) {
                    next();
                    return;
                }
                if (remaining == 0) {
                    expect(false, description);
                    finish();
                    return;
                }
                waitUntil(std::move(description), std::move(predicate), std::move(next), remaining - 1);
            });
        }

        void afterTicks(std::size_t count, std::function<void()> next) {
            if (count == 0) {
                next();
                return;
            }
            defer([this, count, next = std::move(next)]() mutable {
                afterTicks(count - 1, std::move(next));
            });
        }

        backend::FrontendSessionCallbacks sessionCallbacks(EventLog& log, bool stopOnCommand) {
            return backend::FrontendSessionCallbacks{
                [this, &log](const std::vector<backend::SequencedBackendEvent>& events) {
                    std::vector<std::uint64_t> sequences;
                    sequences.reserve(events.size());
                    for (const backend::SequencedBackendEvent& event : events) {
                        sequences.push_back(event.sequence.value());
                        if (const auto* lifecycle = std::get_if<backend::LifecycleChanged>(&event.event)) {
                            log.lifecycles.push_back(*lifecycle);
                        }
                    }
                    expect(sequences.size() <= 32, "BackendCore event callback obeys maxEventsPerCallback");
                    expect(std::is_sorted(sequences.begin(), sequences.end()), "BackendCore event callback preserves sequence order");
                    log.batches.push_back(std::move(sequences));
                },
                [&log](const backend::Snapshot&) {
                    ++log.snapshots;
                },
                [this, stopOnCommand](const backend::CommandCompletion& completion) {
                    completions.insert_or_assign(completion.requestId, completion);
                    ++completionCounts[completion.requestId];

                    if (completion.requestId == "start-success" && completionCounts[completion.requestId] == 1) {
                        backend::TurnStart command;
                        command.threadId = typed::ThreadId{"thread-success"};
                        command.input = {typed::TextInput{"submitted reentrantly"}};
                        expect(static_cast<bool>(controller.submit("turn-reentrant", std::move(command))),
                               "a command completion callback can submit another typed command reentrantly");
                    }
                    if (stopOnCommand && completion.requestId == "stop-now" && backendCore) {
                        backendCore->stop();
                    }
                },
                [this](const std::string&) {
                    ++closedCallbacks;
                }};
        }

        std::size_t completionCount(const std::string& requestId) const {
            const auto iterator = completionCounts.find(requestId);
            return iterator == completionCounts.end() ? 0 : iterator->second;
        }

        const backend::CommandCompletion* completion(const std::string& requestId) const {
            const auto iterator = completions.find(requestId);
            return iterator == completions.end() ? nullptr : &iterator->second;
        }

        bool hasSuccess(const std::string& requestId) const {
            const backend::CommandCompletion* value = completion(requestId);
            return value && !value->result.error;
        }

        bool hasError(const std::string& requestId, backend::CommandErrorCode code) const {
            const backend::CommandCompletion* value = completion(requestId);
            return value && value->result.error && value->result.error->code == code;
        }

        std::uint64_t lastSequence(const EventLog& log) const {
            std::uint64_t last = 0;
            for (const auto& batch : log.batches) {
                if (!batch.empty()) {
                    last = std::max(last, batch.back());
                }
            }
            return last;
        }

        std::vector<std::uint64_t> sequencesAfter(const EventLog& log, std::uint64_t sequence) const {
            std::vector<std::uint64_t> values;
            for (const auto& batch : log.batches) {
                for (const std::uint64_t value : batch) {
                    if (value > sequence) {
                        values.push_back(value);
                    }
                }
            }
            return values;
        }

        void handleOutgoing(const Json& message, const TransportCallbacks& callbacks) {
            const auto method = message.find("method");
            const auto id = message.find("id");
            if (method == message.end() || !method->is_string() || id == message.end()) {
                return;
            }

            const std::string methodName = method->get<std::string>();
            const Json params = message.value("params", Json::object());
            if (methodName == "thread/list") {
                ++threadListRequests;
                if (!params.contains("cursor")) {
                    if (params.value("limit", 0U) == 2U) {
                        ++boundedInitialRefreshes;
                    }
                    tests::codex::inject(callbacks,
                                         Json{{"id", *id},
                                              {"result",
                                               {{"data", Json::array({tests::codex::threadValue("thread-initial")})},
                                                {"nextCursor", "initial-next"},
                                                {"backwardsCursor", nullptr}}}});
                } else {
                    tests::codex::inject(callbacks,
                                         Json{{"id", *id},
                                              {"result",
                                               {{"data", Json::array({tests::codex::threadValue("thread-page")})},
                                                {"nextCursor", nullptr},
                                                {"backwardsCursor", "explicit-before"}}}});
                }
            } else if (methodName == "thread/start") {
                const std::string cwd = params.value("cwd", "");
                if (cwd == "/malformed") {
                    tests::codex::inject(callbacks, Json{{"id", *id}, {"result", Json{{"malformed", true}}}});
                } else if (cwd == "/deferred-close") {
                    deferredClosedCallbacks = callbacks;
                    deferredClosedId = *id;
                } else if (cwd == "/old-generation") {
                    staleCallbacks = callbacks;
                    staleId = *id;
                } else {
                    const std::string threadId = cwd == "/fresh"              ? "thread-fresh"
                                                 : cwd == "/unsolicited-stop" ? "thread-unsolicited-scheduled-completion"
                                                                              : "thread-success";
                    tests::codex::inject(callbacks, Json{{"id", *id}, {"result", tests::codex::threadOperationResult(threadId)}});
                }
            } else if (methodName == "thread/resume") {
                tests::codex::inject(
                    callbacks,
                    Json{{"id", *id},
                         {"error", {{"code", -32'010}, {"message", "deterministic remote failure"}, {"data", {{"source", "fake"}}}}}});
            } else if (methodName == "thread/read") {
                const Json turns = Json::array({tests::codex::turnValue(
                    "thread-read", "turn-read", "completed", Json::array({agentItemValue("item-read", "read text")}))});
                tests::codex::inject(callbacks,
                                     Json{{"id", *id}, {"result", {{"thread", tests::codex::threadValue("thread-read", turns)}}}});
            } else if (methodName == "turn/start") {
                const std::string turnId = params.value("input", Json::array()).empty() ? "turn-start" : "turn-reentrant";
                tests::codex::inject(callbacks, Json{{"id", *id}, {"result", tests::codex::turnOperationResult("thread-success", turnId)}});
            } else if (methodName == "turn/interrupt") {
                tests::codex::inject(callbacks, Json{{"id", *id}, {"result", Json::object()}});
            }
        }

        void verifyInitialHydrationAndSubmitOperations() {
            const backend::Snapshot snapshot = backendCore->snapshot();
            expect(snapshot.lifecycle == backend::BackendLifecycle::Ready && snapshot.threads.size() == 1 &&
                       snapshot.threads[0].id == "thread-initial" && snapshot.threadList.nextCursor == "initial-next",
                   "Ready performs one bounded initial list-page hydration and retains its cursor");
            expect(threadListRequests == 1 && boundedInitialRefreshes == 1,
                   "initial hydration submits exactly one list request with the configured limit");
            expect(controller.role() == backend::SessionRole::Controller && observer.role() == backend::SessionRole::Observer,
                   "explicit controller acquisition remains in force after backend startup");

            backend::ThreadStart start;
            start.options.cwd = "/success";
            expect(static_cast<bool>(controller.submit("start-success", std::move(start))), "controller submits thread/start");

            backend::ThreadResume resume;
            resume.threadId = typed::ThreadId{"thread-remote"};
            expect(static_cast<bool>(controller.submit("resume-remote", std::move(resume))), "controller submits thread/resume");

            backend::ThreadList list;
            list.options.cursor = "explicit";
            list.options.limit = 3;
            expect(static_cast<bool>(controller.submit("list-explicit", std::move(list))), "controller submits thread/list");

            backend::ThreadRead read;
            read.threadId = typed::ThreadId{"thread-read"};
            read.options.includeTurns = true;
            expect(static_cast<bool>(controller.submit("read-full", std::move(read))), "controller submits thread/read");

            backend::TurnStart startTurn;
            startTurn.threadId = typed::ThreadId{"thread-success"};
            startTurn.input = {typed::TextInput{"start a turn"}};
            expect(static_cast<bool>(controller.submit("turn-start", std::move(startTurn))), "controller submits turn/start");

            backend::TurnInterrupt interrupt;
            interrupt.threadId = typed::ThreadId{"thread-success"};
            interrupt.turnId = typed::TurnId{"turn-start"};
            expect(static_cast<bool>(controller.submit("turn-interrupt", std::move(interrupt))), "controller submits turn/interrupt");

            backend::ThreadStart malformed;
            malformed.options.cwd = "/malformed";
            expect(static_cast<bool>(controller.submit("decode-error", std::move(malformed))),
                   "malformed typed-result operation is accepted before typed decoding completion");

            transport->rejectNextSend = true;
            backend::ThreadStart localFailure;
            localFailure.options.cwd = "/local-enqueue-failure";
            expect(static_cast<bool>(controller.submit("local-error", std::move(localFailure))),
                   "transport enqueue failure remains an asynchronous correlated command completion");

            waitUntil(
                "all typed BackendCore command paths complete exactly once",
                [this]() {
                    static const std::vector<std::string> ids = {"start-success",
                                                                 "resume-remote",
                                                                 "list-explicit",
                                                                 "read-full",
                                                                 "turn-start",
                                                                 "turn-interrupt",
                                                                 "decode-error",
                                                                 "local-error",
                                                                 "turn-reentrant"};
                    return std::all_of(ids.begin(), ids.end(), [this](const std::string& id) {
                        return completionCount(id) == 1;
                    });
                },
                [this]() {
                    verifyOperationResultsAndStreamEvents();
                });
        }

        void verifyOperationResultsAndStreamEvents() {
            expect(hasSuccess("start-success") && hasSuccess("list-explicit") && hasSuccess("read-full") && hasSuccess("turn-start") &&
                       hasSuccess("turn-interrupt") && hasSuccess("turn-reentrant"),
                   "successful typed thread and turn operations produce one successful command response each");
            expect(hasError("resume-remote", backend::CommandErrorCode::RemoteAppServerError) &&
                       completion("resume-remote")->result.error->remoteCode == -32'010,
                   "remote App Server error preserves stable category and optional remote code");
            expect(hasError("decode-error", backend::CommandErrorCode::TypedDecodingFailure),
                   "malformed successful result maps to typed_decoding_failure");
            expect(hasError("local-error", backend::CommandErrorCode::LocalSubmissionFailure),
                   "transport enqueue rejection maps to local_submission_failure");

            const backend::Snapshot hydrated = backendCore->snapshot();
            const auto findThread = [&hydrated](const std::string& id) {
                return std::find_if(hydrated.threads.begin(), hydrated.threads.end(), [&id](const backend::ThreadSnapshot& value) {
                    return value.id == id;
                });
            };
            const auto started = findThread("thread-success");
            const auto read = findThread("thread-read");
            expect(started != hydrated.threads.end() && read != hydrated.threads.end() && read->fullyLoaded && read->turns.size() == 1 &&
                       read->turns[0].items.size() == 1,
                   "operation results hydrate thread/start and full thread/read state without waiting for notifications");
            expect(threadListRequests == 2 && hydrated.threadList.complete,
                   "explicit thread/list merges its page and updates completeness independently of initial hydration");

            streamStartSequence = hydrated.sequence.value();
            transport->inject({{"method", "thread/started"}, {"params", {{"thread", tests::codex::threadValue("thread-success")}}}});
            transport->inject({{"method", "thread/started"}, {"params", {{"thread", tests::codex::threadValue("thread-stream")}}}});
            transport->inject(
                {{"method", "turn/started"},
                 {"params", {{"threadId", "thread-stream"}, {"turn", tests::codex::turnValue("thread-stream", "turn-stream")}}}});
            transport->inject({{"method", "item/started"},
                               {"params",
                                {{"threadId", "thread-stream"},
                                 {"turnId", "turn-stream"},
                                 {"item", agentItemValue("item-stream")},
                                 {"startedAtMs", 10}}}});
            streamedText.clear();
            for (std::size_t index = 0; index < 100; ++index) {
                const std::string delta = std::to_string(index % 10);
                streamedText += delta;
                transport->inject(
                    {{"method", "item/agentMessage/delta"},
                     {"params", {{"threadId", "thread-stream"}, {"turnId", "turn-stream"}, {"itemId", "item-stream"}, {"delta", delta}}}});
            }
            transport->inject({{"method", "item/completed"},
                               {"params",
                                {{"threadId", "thread-stream"},
                                 {"turnId", "turn-stream"},
                                 {"item", agentItemValue("item-stream", streamedText)},
                                 {"completedAtMs", 20}}}});
            transport->inject(
                {{"method", "turn/completed"},
                 {"params",
                  {{"threadId", "thread-stream"}, {"turn", tests::codex::turnValue("thread-stream", "turn-stream", "completed")}}}});
            if (transport->callbacks.onDiagnostic) {
                transport->callbacks.onDiagnostic(Diagnostic{"deterministic backend diagnostic"});
            }

            waitUntil(
                "streamed typed events reduce into exact terminal state and drain to both sessions",
                [this]() {
                    const backend::Snapshot snapshot = backendCore->snapshot();
                    for (const backend::ThreadSnapshot& thread : snapshot.threads) {
                        if (thread.id == "thread-stream" && !thread.turns.empty() && thread.turns[0].terminal &&
                            !thread.turns[0].items.empty() && thread.turns[0].items[0].agentText == streamedText &&
                            snapshot.diagnostics.received != 0) {
                            return lastSequence(controllerEvents) >= snapshot.sequence.value() &&
                                   lastSequence(observerEvents) >= snapshot.sequence.value();
                        }
                    }
                    return false;
                },
                [this]() {
                    verifyStreamAndPendingRequest();
                });
        }

        void verifyStreamAndPendingRequest() {
            const backend::Snapshot snapshot = backendCore->snapshot();
            expect(snapshot.diagnostics.recent.back() == "deterministic backend diagnostic",
                   "transport diagnostics are summarized in canonical backend state");
            expect(std::count_if(snapshot.threads.begin(),
                                 snapshot.threads.end(),
                                 [](const backend::ThreadSnapshot& thread) {
                                     return thread.id == "thread-success";
                                 }) == 1,
                   "operation result plus authoritative notification upsert one thread without duplicate order entries");
            const std::vector<std::uint64_t> controllerStream = sequencesAfter(controllerEvents, streamStartSequence);
            const std::vector<std::uint64_t> observerStream = sequencesAfter(observerEvents, streamStartSequence);
            expect(controllerStream == observerStream && !controllerStream.empty(),
                   "controller and observer receive identical ordered event sequences for their common streamed interval");

            transport->inject({{"method", "item/commandExecution/requestApproval"},
                               {"id", "approval-1"},
                               {"params",
                                {{"threadId", "thread-stream"},
                                 {"turnId", "turn-stream"},
                                 {"itemId", "command-approval"},
                                 {"startedAtMs", 30},
                                 {"command", "make test"},
                                 {"cwd", "/tmp/project"}}}});
            waitUntil(
                "typed server request is retained and delivered promptly",
                [this]() {
                    const backend::Snapshot current = backendCore->snapshot();
                    return current.pendingRequests.size() == 1 && lastSequence(controllerEvents) >= current.sequence.value() &&
                           lastSequence(observerEvents) >= current.sequence.value();
                },
                [this]() {
                    disconnectControllerWithPendingRequest();
                });
        }

        void disconnectControllerWithPendingRequest() {
            const backend::Snapshot beforeClose = backendCore->snapshot();
            pendingApprovalId = beforeClose.pendingRequests.front().id;
            backend::ThreadStart pending;
            pending.options.cwd = "/deferred-close";
            expect(static_cast<bool>(controller.submit("closed-operation", std::move(pending))),
                   "controller starts an operation whose response will arrive after session close");
            controller.close("controller disconnected intentionally");
            expect(!backendCore->snapshot().controller && backendCore->snapshot().pendingRequests.size() == 1,
                   "controller disconnect releases ownership but retains unanswered server requests");
            expect(static_cast<bool>(observer.submit("observer-acquire", backend::ControllerAcquire{})),
                   "remaining observer explicitly acquires controller ownership");

            waitUntil(
                "remaining observer becomes controller",
                [this]() {
                    return hasSuccess("observer-acquire") && observer.role() == backend::SessionRole::Controller;
                },
                [this]() {
                    completeClosedSessionOperationAndAnswerPending();
                });
        }

        void completeClosedSessionOperationAndAnswerPending() {
            expect(deferredClosedCallbacks.has_value() && deferredClosedId.has_value(),
                   "fake transport retained the closed session operation response");
            if (deferredClosedCallbacks && deferredClosedId) {
                tests::codex::inject(
                    *deferredClosedCallbacks,
                    Json{{"id", *deferredClosedId}, {"result", tests::codex::threadOperationResult("thread-from-closed-session")}});
            }

            transport->rejectNextSend = true;
            expect(static_cast<bool>(observer.submit("approval-enqueue-fail",
                                                     backend::ApprovalRespond{pendingApprovalId, typed::ApprovalDecision::decline()})),
                   "new controller can attempt to answer the retained pending approval");
            waitUntil(
                "failed response enqueue retains request and closed session suppresses operation completion",
                [this]() {
                    const backend::Snapshot snapshot = backendCore->snapshot();
                    const bool hydrated = std::any_of(snapshot.threads.begin(), snapshot.threads.end(), [](const auto& thread) {
                        return thread.id == "thread-from-closed-session";
                    });
                    return hydrated && hasError("approval-enqueue-fail", backend::CommandErrorCode::LocalSubmissionFailure);
                },
                [this]() {
                    expect(completionCount("closed-operation") == 0 && backendCore->snapshot().pendingRequests.size() == 1,
                           "session close suppresses its later response without cancelling hydration; enqueue failure retains request");
                    expect(static_cast<bool>(observer.submit(
                               "approval-success", backend::ApprovalRespond{pendingApprovalId, typed::ApprovalDecision::decline()})),
                           "pending approval can be retried after local enqueue failure");
                    waitUntil(
                        "successful approval response removes pending request exactly once",
                        [this]() {
                            return hasSuccess("approval-success") && backendCore->snapshot().pendingRequests.empty();
                        },
                        [this]() {
                            beginUnsolicitedStopScenario();
                        });
                });
        }

        void beginUnsolicitedStopScenario() {
            backend::ThreadStart operation;
            operation.options.cwd = "/unsolicited-stop";
            expect(static_cast<bool>(observer.submit("unsolicited-stop-operation", std::move(operation))),
                   "controller submits an operation whose typed completion is scheduled but not yet delivered");

            const std::size_t refreshesBeforeRestart = boundedInitialRefreshes;
            appServerClient->stop();
            waitUntil(
                "unsolicited App Server Stopping and Stopped cancel the accepted operation exactly once",
                [this]() {
                    const auto hasLifecycle = [](backend::BackendLifecycle expected) {
                        return [expected](const backend::LifecycleChanged& value) {
                            return value.lifecycle == expected;
                        };
                    };
                    const auto stopping = std::find_if(observerEvents.lifecycles.begin(),
                                                       observerEvents.lifecycles.end(),
                                                       hasLifecycle(backend::BackendLifecycle::Stopping));
                    const auto stopped =
                        stopping == observerEvents.lifecycles.end()
                            ? observerEvents.lifecycles.end()
                            : std::find_if(stopping, observerEvents.lifecycles.end(), hasLifecycle(backend::BackendLifecycle::Stopped));
                    return backendCore->snapshot().lifecycle == backend::BackendLifecycle::Stopped &&
                           stopping != observerEvents.lifecycles.end() && stopped != observerEvents.lifecycles.end() &&
                           completionCount("unsolicited-stop-operation") == 1;
                },
                [this, refreshesBeforeRestart]() {
                    const backend::CommandCompletion* cancelled = completion("unsolicited-stop-operation");
                    expect(cancelled && cancelled->result.error && cancelled->result.error->code == backend::CommandErrorCode::Cancelled,
                           "an unsolicited orderly connection invalidation reports a stable cancelled command result");
                    expect(cancelled && cancelled->result.error &&
                               cancelled->result.error->message == "The App Server connection stopped before the operation completed.",
                           "BackendCore completes its accepted-operation ledger instead of relying on a suppressed typed callback");
                    const auto& threads = backendCore->snapshot().threads;
                    expect(std::none_of(threads.begin(),
                                        threads.end(),
                                        [](const backend::ThreadSnapshot& thread) {
                                            return thread.id == "thread-unsolicited-scheduled-completion";
                                        }),
                           "the invalidated scheduled typed completion does not mutate canonical state");

                    const std::uint64_t invalidatedGeneration = observerEvents.lifecycles.back().connectionGeneration;
                    backendCore->start();
                    waitUntil(
                        "BackendCore restarts after the unsolicited Stopped lifecycle in a fresh generation",
                        [this, refreshesBeforeRestart, invalidatedGeneration]() {
                            const bool observedFreshReady = std::any_of(observerEvents.lifecycles.begin(),
                                                                        observerEvents.lifecycles.end(),
                                                                        [invalidatedGeneration](const backend::LifecycleChanged& value) {
                                                                            return value.lifecycle == backend::BackendLifecycle::Ready &&
                                                                                   value.connectionGeneration > invalidatedGeneration;
                                                                        });
                            return backendCore->isReady() && boundedInitialRefreshes == refreshesBeforeRestart + 1 && observedFreshReady;
                        },
                        [this]() {
                            afterTicks(8, [this]() {
                                const auto& restartedThreads = backendCore->snapshot().threads;
                                const bool staleHydrated = std::any_of(
                                    restartedThreads.begin(), restartedThreads.end(), [](const backend::ThreadSnapshot& thread) {
                                        return thread.id == "thread-unsolicited-scheduled-completion";
                                    });
                                expect(!staleHydrated && completionCount("unsolicited-stop-operation") == 1,
                                       "restart generation suppresses the invalidated typed completion and duplicate response");
                                beginStopRestartGenerationScenario();
                            });
                        });
                });
        }

        void beginStopRestartGenerationScenario() {
            backend::ThreadStart stale;
            stale.options.cwd = "/old-generation";
            expect(static_cast<bool>(observer.submit("old-generation-operation", std::move(stale))),
                   "controller submits operation retained across explicit stop boundary");
            expect(static_cast<bool>(observer.submit("stop-now", backend::SnapshotGet{})),
                   "callback-stop scenario queues a read-only completion");

            waitUntil(
                "command callback stops backend and cancels active operation once",
                [this]() {
                    return backendCore->snapshot().lifecycle == backend::BackendLifecycle::Stopped &&
                           hasError("old-generation-operation", backend::CommandErrorCode::Cancelled) &&
                           completionCount("old-generation-operation") == 1;
                },
                [this]() {
                    expect(staleCallbacks.has_value() && staleId.has_value(),
                           "fake transport retained the prior-generation operation response");
                    backendCore->start();
                    waitUntil(
                        "backend explicitly restarts and performs one bounded refresh in the new generation",
                        [this]() {
                            return backendCore->isReady() && threadListRequests >= 3 && boundedInitialRefreshes >= 2;
                        },
                        [this]() {
                            verifyStaleCompletionSuppression();
                        });
                });
        }

        void verifyStaleCompletionSuppression() {
            if (staleCallbacks && staleId) {
                tests::codex::inject(*staleCallbacks,
                                     Json{{"id", *staleId}, {"result", tests::codex::threadOperationResult("thread-stale-generation")}});
            }
            afterTicks(8, [this]() {
                const backend::Snapshot snapshot = backendCore->snapshot();
                const bool staleHydrated = std::any_of(snapshot.threads.begin(), snapshot.threads.end(), [](const auto& thread) {
                    return thread.id == "thread-stale-generation";
                });
                expect(!staleHydrated && completionCount("old-generation-operation") == 1,
                       "prior-generation typed completion neither mutates state nor duplicates command completion");

                backend::ThreadStart fresh;
                fresh.options.cwd = "/fresh";
                expect(static_cast<bool>(observer.submit("fresh-generation-operation", std::move(fresh))),
                       "new generation accepts fresh typed operations");
                waitUntil(
                    "fresh generation typed operation completes",
                    [this]() {
                        return hasSuccess("fresh-generation-operation");
                    },
                    [this]() {
                        failConnectionWithPendingRequest();
                    });
            });
        }

        void failConnectionWithPendingRequest() {
            transport->inject({{"method", "future/pending"}, {"id", "unknown-pending"}, {"params", {{"future", true}}}});
            waitUntil(
                "unknown typed request is retained before connection invalidation",
                [this]() {
                    return backendCore->snapshot().pendingRequests.size() == 1;
                },
                [this]() {
                    if (transport->callbacks.onError) {
                        transport->callbacks.onError(Error{Error::Category::Transport, 91, "deterministic connection failure"});
                    }
                    waitUntil(
                        "connection failure clears pending ownership and exposes lifecycle failure",
                        [this]() {
                            const backend::Snapshot snapshot = backendCore->snapshot();
                            return snapshot.lifecycle == backend::BackendLifecycle::Failed && snapshot.pendingRequests.empty() &&
                                   snapshot.lastLifecycleError.has_value();
                        },
                        [this]() {
                            const backend::Snapshot failed = backendCore->snapshot();
                            expect(failed.lastLifecycleError->message == "deterministic connection failure",
                                   "BackendCore retains lifecycle failure details while clearing invalid request ownership");
                            backendCore->stop();
                            waitUntil(
                                "explicit stop leaves failed lifecycle ready for recovery",
                                [this]() {
                                    return backendCore->snapshot().lifecycle == backend::BackendLifecycle::Stopped;
                                },
                                [this]() {
                                    backendCore->start();
                                    waitUntil(
                                        "explicit start recovers lifecycle failure and reruns bounded refresh",
                                        [this]() {
                                            const backend::Snapshot snapshot = backendCore->snapshot();
                                            return backendCore->isReady() && !snapshot.lastLifecycleError && boundedInitialRefreshes >= 3;
                                        },
                                        [this]() {
                                            stopCleanly();
                                        });
                                });
                        });
                });
        }

        void stopCleanly() {
            backendCore->stop();
            waitUntil(
                "final BackendCore stop reaches Stopped",
                [this]() {
                    return backendCore->snapshot().lifecycle == backend::BackendLifecycle::Stopped;
                },
                [this]() {
                    observer.close("test complete");
                    backendCore.reset();
                    afterTicks(2, [this]() {
                        finish();
                    });
                });
        }

        void finish() {
            if (finished) {
                return;
            }
            finished = true;
            if (backendCore) {
                backendCore->stop();
                backendCore.reset();
            }
            core::SNodeC::stop();
        }

        tests::support::TestResult& result;
        std::shared_ptr<tests::codex::FakeTransportState> transport;
        tests::codex::FakeAppServerClient* appServerClient = nullptr;
        std::unique_ptr<backend::BackendCore> backendCore;
        backend::FrontendSession controller;
        backend::FrontendSession observer;
        EventLog controllerEvents;
        EventLog observerEvents;
        std::map<std::string, backend::CommandCompletion> completions;
        std::map<std::string, std::size_t> completionCounts;
        std::size_t threadListRequests = 0;
        std::size_t boundedInitialRefreshes = 0;
        std::size_t closedCallbacks = 0;
        std::uint64_t streamStartSequence = 0;
        std::string streamedText;
        backend::PendingRequestId pendingApprovalId;
        std::optional<TransportCallbacks> deferredClosedCallbacks;
        std::optional<Json> deferredClosedId;
        std::optional<TransportCallbacks> staleCallbacks;
        std::optional<Json> staleId;
        bool finished = false;
    };
} // namespace

int main(int argc, char* argv[]) {
    tests::support::TestResult result;
    int returnCode = tests::support::cTestSkipReturnCode;

    if (tests::support::shouldSkipRootWithoutSNodeCGroup()) {
        tests::support::printRootWithoutSNodeCGroupSkipMessage("CodexBackendCoreTest");
    } else {
        core::SNodeC::init(argc, argv);
        bool timedOut = false;
        BackendCoreRunner runner(result);
        [[maybe_unused]] core::timer::Timer watchdog = core::timer::Timer::singleshotTimer(
            [&timedOut]() {
                timedOut = true;
                core::SNodeC::stop();
            },
            utils::Timeval({10, 0}));

        runner.start();
        const int eventLoopResult = core::SNodeC::start(utils::Timeval({12, 0}));
        result.expectTrue(!timedOut, "BackendCore deterministic scenario finishes before watchdog");
        result.expectTrue(runner.isFinished(), "BackendCore deterministic scenario reaches clean terminal state");
        result.expectEqual(0, eventLoopResult, "BackendCore event loop exits cleanly");
        core::SNodeC::free();
        returnCode = result.processResult();
    }

    return returnCode;
}
