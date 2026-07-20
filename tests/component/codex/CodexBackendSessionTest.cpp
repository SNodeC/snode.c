/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include "CodexBackendTestSupport.h"
#include "ai/openai/codex/backend/BackendCore.h"
#include "support/TestResult.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace {
    namespace backend = ai::openai::codex::backend;
    namespace typed = ai::openai::codex::typed;

    class ManualScheduler {
    public:
        void schedule(std::function<void()> callback) {
            callbacks.push_back(std::move(callback));
            ++scheduled;
        }

        bool runOne() {
            if (callbacks.empty()) {
                return false;
            }
            std::function<void()> callback = std::move(callbacks.front());
            callbacks.pop_front();
            callback();
            ++executed;
            return true;
        }

        void drain(std::size_t limit = 10'000) {
            std::size_t count = 0;
            while (runOne() && count < limit) {
                ++count;
            }
        }

        std::size_t pending() const noexcept {
            return callbacks.size();
        }

        std::size_t scheduled = 0;
        std::size_t executed = 0;

    private:
        std::deque<std::function<void()>> callbacks;
    };

    struct SessionObservations {
        std::vector<std::vector<backend::SequenceNumber>> eventBatches;
        std::vector<backend::CommandCompletion> completions;
        std::vector<backend::Snapshot> snapshots;
        std::vector<std::string> closeReasons;
    };

    backend::FrontendSessionCallbacks callbacksFor(SessionObservations& observations) {
        return backend::FrontendSessionCallbacks{[&observations](const std::vector<backend::SequencedBackendEvent>& events) {
                                                     std::vector<backend::SequenceNumber> sequences;
                                                     sequences.reserve(events.size());
                                                     for (const backend::SequencedBackendEvent& event : events) {
                                                         sequences.push_back(event.sequence);
                                                     }
                                                     observations.eventBatches.push_back(std::move(sequences));
                                                 },
                                                 [&observations](const backend::Snapshot& snapshot) {
                                                     observations.snapshots.push_back(snapshot);
                                                 },
                                                 [&observations](const backend::CommandCompletion& completion) {
                                                     observations.completions.push_back(completion);
                                                 },
                                                 [&observations](const std::string& reason) {
                                                     observations.closeReasons.push_back(reason);
                                                 }};
    }

    const backend::CommandCompletion* completion(const SessionObservations& observations, const std::string& requestId) {
        for (const backend::CommandCompletion& value : observations.completions) {
            if (value.requestId == requestId) {
                return &value;
            }
        }
        return nullptr;
    }

    bool hasError(const SessionObservations& observations, const std::string& requestId, backend::CommandErrorCode expected) {
        const backend::CommandCompletion* value = completion(observations, requestId);
        return value && value->result.error && value->result.error->code == expected;
    }

    std::vector<std::uint64_t> flattenSequences(const SessionObservations& observations) {
        std::vector<std::uint64_t> result;
        for (const auto& batch : observations.eventBatches) {
            for (const backend::SequenceNumber sequence : batch) {
                result.push_back(sequence.value());
            }
        }
        return result;
    }

    void testSessionPolicyAndSafety(tests::support::TestResult& result) {
        const auto transport = std::make_shared<tests::codex::FakeTransportState>();
        ManualScheduler scheduler;
        backend::BackendCoreOptions options;
        options.scheduler = [&scheduler](std::function<void()> callback) {
            scheduler.schedule(std::move(callback));
        };
        options.maxEventsPerCallback = 64;

        backend::BackendCore core(std::make_unique<tests::codex::FakeAppServerClient>(transport), options);
        SessionObservations firstObservations;
        SessionObservations secondObservations;
        backend::FrontendSession first = core.openSession(callbacksFor(firstObservations));
        backend::FrontendSession second = core.openSession(callbacksFor(secondObservations));

        result.expectTrue(first.isOpen() && second.isOpen() && first.id() != second.id() &&
                              first.role() == backend::SessionRole::Observer && second.role() == backend::SessionRole::Observer &&
                              !core.snapshot().controller,
                          "every frontend session starts as an explicit observer with a stable distinct id");

        const std::size_t scheduledBeforeAcquire = scheduler.scheduled;
        const backend::CommandSubmission acquired = first.submit("acquire", backend::ControllerAcquire{});
        result.expectTrue(acquired && first.role() == backend::SessionRole::Controller && second.role() == backend::SessionRole::Observer &&
                              core.snapshot().controller == first.id() && firstObservations.completions.empty() &&
                              scheduler.scheduled >= scheduledBeforeAcquire,
                          "controller acquisition mutates state synchronously but its completion is always asynchronous");

        const backend::CommandSubmission duplicate = first.submit("acquire", backend::ControllerAcquire{});
        result.expectTrue(!duplicate && duplicate.error == backend::SubmissionError::DuplicateRequestId,
                          "a still-pending request id is rejected within its frontend session");
        const backend::CommandSubmission sameIdOtherSession = second.submit("acquire", backend::ControllerAcquire{});
        result.expectTrue(static_cast<bool>(sameIdOtherSession), "request ids are independent between frontend sessions");

        backend::ThreadStart observerMutation;
        observerMutation.options.cwd = "/observer";
        result.expectTrue(static_cast<bool>(second.submit("observer-mutation", observerMutation)),
                          "permission failure for an accepted observer command is delivered as a correlated response");
        result.expectTrue(second.submit("observer-snapshot", backend::SnapshotGet{}) &&
                              second.submit("observer-replay", backend::ReplayAfter{backend::SequenceNumber{0}}),
                          "observers can submit read-only snapshot and replay commands");

        backend::ThreadStart unavailableMutation;
        unavailableMutation.options.cwd = "/not-ready";
        result.expectTrue(static_cast<bool>(first.submit("controller-not-ready", unavailableMutation)),
                          "controller mutation is accepted for asynchronous backend-availability completion");

        scheduler.drain();
        result.expectTrue(completion(firstObservations, "acquire") && !completion(firstObservations, "acquire")->result.error &&
                              hasError(secondObservations, "acquire", backend::CommandErrorCode::Conflict),
                          "first acquisition succeeds and a second controller acquisition reports conflict");
        result.expectTrue(hasError(secondObservations, "observer-mutation", backend::CommandErrorCode::PermissionDenied) &&
                              hasError(firstObservations, "controller-not-ready", backend::CommandErrorCode::BackendUnavailable),
                          "observer mutation and controller operation against a stopped backend use distinct stable errors");
        result.expectTrue(
            completion(secondObservations, "observer-snapshot") &&
                std::holds_alternative<backend::Snapshot>(completion(secondObservations, "observer-snapshot")->result.value) &&
                completion(secondObservations, "observer-replay") &&
                std::holds_alternative<backend::ReplayResult>(completion(secondObservations, "observer-replay")->result.value),
            "observer snapshot and replay commands complete successfully with typed values");

        const backend::CommandSubmission reacquire = first.submit("reacquire", backend::ControllerAcquire{});
        scheduler.drain();
        result.expectTrue(reacquire && completion(firstObservations, "reacquire") &&
                              !completion(firstObservations, "reacquire")->result.error && first.role() == backend::SessionRole::Controller,
                          "controller reacquisition by the same session is idempotent");

        const backend::CommandSubmission release = first.submit("release", backend::ControllerRelease{});
        scheduler.drain();
        result.expectTrue(release && completion(firstObservations, "release") && !completion(firstObservations, "release")->result.error &&
                              first.role() == backend::SessionRole::Observer && !core.snapshot().controller,
                          "only the current controller can explicitly release ownership");

        result.expectTrue(static_cast<bool>(first.submit("release-again", backend::ControllerRelease{})),
                          "an invalid release remains a correlated accepted command");
        scheduler.drain();
        result.expectTrue(hasError(firstObservations, "release-again", backend::CommandErrorCode::PermissionDenied),
                          "release by a non-controller reports permission_denied");

        result.expectTrue(static_cast<bool>(second.submit("second-acquire", backend::ControllerAcquire{})),
                          "a later observer can acquire the free controller role");
        scheduler.drain();
        result.expectTrue(second.role() == backend::SessionRole::Controller && core.snapshot().controller == second.id(),
                          "controller ownership moves only after explicit acquisition");

        const std::vector<std::uint64_t> firstSequences = flattenSequences(firstObservations);
        const std::vector<std::uint64_t> secondSequences = flattenSequences(secondObservations);
        result.expectTrue(!firstSequences.empty() && !secondSequences.empty() &&
                              std::is_sorted(firstSequences.begin(), firstSequences.end()) &&
                              std::is_sorted(secondSequences.begin(), secondSequences.end()),
                          "multiple sessions receive ordered reducer events in bounded asynchronous batches");

        SessionObservations throwingObservations;
        std::size_t throwingCallbacks = 0;
        backend::FrontendSession throwing =
            core.openSession(backend::FrontendSessionCallbacks{[&throwingCallbacks](const std::vector<backend::SequencedBackendEvent>&) {
                                                                   ++throwingCallbacks;
                                                                   throw std::runtime_error("intentional frontend callback failure");
                                                               },
                                                               {},
                                                               {},
                                                               [&throwingObservations](const std::string& reason) {
                                                                   throwingObservations.closeReasons.push_back(reason);
                                                               }});
        const std::size_t firstBatchCount = firstObservations.eventBatches.size();
        result.expectTrue(static_cast<bool>(second.submit("second-release", backend::ControllerRelease{})),
                          "controller release triggers an event for callback-isolation checks");
        scheduler.drain();
        result.expectTrue(throwingCallbacks != 0 && firstObservations.eventBatches.size() > firstBatchCount && throwing.isOpen(),
                          "one throwing session callback is exception-bounded and does not suppress another session");

        SessionObservations closingObservations;
        backend::FrontendSession closing;
        closing = core.openSession(backend::FrontendSessionCallbacks{[&closing](const std::vector<backend::SequencedBackendEvent>&) {
                                                                         closing.close("self close");
                                                                     },
                                                                     {},
                                                                     {},
                                                                     [&closingObservations](const std::string& reason) {
                                                                         closingObservations.closeReasons.push_back(reason);
                                                                     }});
        result.expectTrue(static_cast<bool>(first.submit("trigger-event", backend::ControllerAcquire{})),
                          "a reducer transition is available to exercise close-during-delivery safety");
        scheduler.drain();
        result.expectTrue(!closing.isOpen() && closingObservations.closeReasons == std::vector<std::string>({"self close"}) &&
                              first.isOpen() && second.isOpen(),
                          "a frontend may close itself during delivery without use-after-close or peer disruption");

        std::size_t suppressedCompletions = 0;
        backend::FrontendSession closingWithResponse =
            core.openSession(backend::FrontendSessionCallbacks{{},
                                                               {},
                                                               [&suppressedCompletions](const backend::CommandCompletion&) {
                                                                   ++suppressedCompletions;
                                                               },
                                                               {}});
        result.expectTrue(static_cast<bool>(closingWithResponse.submit("suppressed", backend::SnapshotGet{})),
                          "snapshot response is queued before explicit close suppression check");
        closingWithResponse.close("closed before response delivery");
        scheduler.drain();
        result.expectTrue(suppressedCompletions == 0, "closing a frontend session suppresses its already queued command response safely");

        const backend::SessionId secondId = second.id();
        result.expectTrue(first.role() == backend::SessionRole::Controller,
                          "first session owns the controller immediately before disconnect behavior check");
        first.close("controller disconnected");
        scheduler.drain();
        const backend::Snapshot afterDisconnect = core.snapshot();
        result.expectTrue(!afterDisconnect.controller && afterDisconnect.sessions.size() >= 2 && second.isOpen() &&
                              second.id() == secondId && second.role() == backend::SessionRole::Observer,
                          "controller disconnect releases ownership without closing remaining observer sessions");
    }

    void testEventlessCommandSessionDoesNotMirrorBackendEvents(tests::support::TestResult& result) {
        const auto transport = std::make_shared<tests::codex::FakeTransportState>();
        ManualScheduler scheduler;
        backend::BackendCoreOptions options;
        options.scheduler = [&scheduler](std::function<void()> callback) {
            scheduler.schedule(std::move(callback));
        };
        options.maxSessionQueueEntries = 2;

        backend::BackendCore core(std::make_unique<tests::codex::FakeAppServerClient>(transport), options);
        std::size_t eventlessCompletions = 0;
        backend::FrontendSession eventless =
            core.openSession(backend::FrontendSessionCallbacks{{},
                                                               {},
                                                               [&eventlessCompletions](const backend::CommandCompletion&) {
                                                                   ++eventlessCompletions;
                                                               },
                                                               {}});
        SessionObservations peerObservations;
        backend::FrontendSession peer = core.openSession(callbacksFor(peerObservations));
        result.expectTrue(static_cast<bool>(peer.submit("peer-acquire", backend::ControllerAcquire{})),
                          "peer emits another backend transition before queued delivery runs");

        result.expectTrue(eventless.isOpen(),
                          "a command-only session with no event callback does not mirror raw backend events into its outbound queue");
        result.expectTrue(static_cast<bool>(eventless.submit("eventless-snapshot", backend::SnapshotGet{})),
                          "eventless session remains available for commands while a global observer handles updates");
        scheduler.drain();
        result.expectTrue(eventlessCompletions == 1,
                          "eventless command session receives its correlated response without subscribing to backend events");
    }
} // namespace

int main() {
    tests::support::TestResult result;
    testSessionPolicyAndSafety(result);
    testEventlessCommandSessionDoesNotMirrorBackendEvents(result);
    return result.processResult();
}
