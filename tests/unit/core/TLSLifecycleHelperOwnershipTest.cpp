#include "core/socket/stream/tls/detail/TLSLifecycleTestHooks.h"
#include "tests/support/TestResult.h"

#include <cerrno>
#include <iostream>
#include <string>

using core::socket::stream::tls::detail::test::Axis;
using core::socket::stream::tls::detail::test::Counters;
using core::socket::stream::tls::detail::test::LifecycleModel;
using core::socket::stream::tls::detail::test::Outcome;
using tests::support::TestResult;

namespace {

    void expectAccounting(TestResult& result, const Counters& counters, int helpers, const std::string& name) {
        result.expectEqual(helpers, counters.constructed, name + ": constructed count");
        result.expectEqual(helpers, counters.destroyed, name + ": destroyed count");
        result.expectEqual(0, counters.active, name + ": active count returns to zero");
        result.expectEqual(1, counters.maxConcurrent, name + ": max concurrent helpers");
        result.expectEqual(helpers, counters.releases, name + ": release count");
    }

    void synchronousSuccess(TestResult& result, const std::string& name) {
        Counters counters;
        auto* helper = new LifecycleModel(counters);
        helper->enqueue(Outcome::Success);
        helper->start();

        result.expectEqual(1, counters.successes, name + ": success callback once");
        result.expectEqual(0, counters.readEnables + counters.writeEnables, name + ": no receiver registered");
        expectAccounting(result, counters, 1, name);
    }

    void synchronousError(TestResult& result, Outcome outcome, const std::string& name) {
        Counters counters;
        auto* helper = new LifecycleModel(counters);
        helper->enqueue(outcome);
        helper->start();

        result.expectEqual(1, counters.errors, name + ": error callback once");
        result.expectEqual(0, counters.readEnables + counters.writeEnables, name + ": no receiver registered");
        expectAccounting(result, counters, 1, name);
    }

    void wantReadLifecycle(TestResult& result, const std::string& name) {
        Counters counters;
        auto* helper = new LifecycleModel(counters);
        helper->enqueue(Outcome::WantRead);
        helper->enqueue(Outcome::Success);
        helper->start();

        result.expectEqual(1, counters.readEnables, name + ": read receiver registered");
        result.expectTrue(helper->guardActive(), name + ": guard remains active while observed");
        helper->readEvent();
        result.expectEqual(1, counters.successes, name + ": success callback once");
        result.expectEqual(1, counters.readDisables, name + ": receiver disabled");
        result.expectTrue(helper->guardActive(), name + ": release deferred");
        helper->unobservedEvent();
        expectAccounting(result, counters, 1, name);
    }

    void wantWriteLifecycle(TestResult& result, const std::string& name) {
        Counters counters;
        auto* helper = new LifecycleModel(counters);
        helper->enqueue(Outcome::WantWrite);
        helper->enqueue(Outcome::Success);
        helper->start();

        result.expectEqual(1, counters.writeEnables, name + ": write receiver registered");
        helper->writeEvent();
        result.expectEqual(1, counters.successes, name + ": success callback once");
        result.expectEqual(1, counters.writeDisables, name + ": receiver disabled");
        result.expectTrue(helper->guardActive(), name + ": release deferred");
        helper->unobservedEvent();
        expectAccounting(result, counters, 1, name);
    }

    void switching(TestResult& result, Outcome first, Outcome second, Axis finalAxis, const std::string& name) {
        Counters counters;
        auto* helper = new LifecycleModel(counters);
        helper->enqueue(first);
        helper->enqueue(second);
        helper->enqueue(Outcome::Success);
        helper->start();
        if (first == Outcome::WantRead) {
            helper->readEvent();
        } else {
            helper->writeEvent();
        }
        result.expectTrue(helper->activeAxis() == finalAxis, name + ": switched to expected active axis");
        if (second == Outcome::WantRead) {
            helper->readEvent();
        } else {
            helper->writeEvent();
        }
        result.expectEqual(1, counters.successes, name + ": one success");
        result.expectEqual(1, counters.readEnables, name + ": no double read enable");
        result.expectEqual(1, counters.writeEnables, name + ": no double write enable");
        helper->unobservedEvent();
        expectAccounting(result, counters, 1, name);
    }

    void repeatedReadiness(TestResult& result, Outcome repeated, const std::string& name) {
        Counters counters;
        auto* helper = new LifecycleModel(counters);
        helper->enqueue(repeated);
        helper->enqueue(repeated);
        helper->enqueue(Outcome::Success);
        helper->start();
        repeated == Outcome::WantRead ? helper->readEvent() : helper->writeEvent();
        repeated == Outcome::WantRead ? helper->readEvent() : helper->writeEvent();
        result.expectEqual(1, repeated == Outcome::WantRead ? counters.readEnables : counters.writeEnables, name + ": one enable");
        result.expectEqual(1, repeated == Outcome::WantRead ? counters.readResumes : counters.writeResumes, name + ": one resume");
        result.expectEqual(1, counters.successes, name + ": success once");
        helper->unobservedEvent();
        expectAccounting(result, counters, 1, name);
    }

    void timeoutAfterObservation(TestResult& result, Outcome readiness, const std::string& name) {
        Counters counters;
        auto* helper = new LifecycleModel(counters);
        helper->enqueue(readiness);
        helper->start();
        readiness == Outcome::WantRead ? helper->readTimeout() : helper->writeTimeout();
        result.expectEqual(1, counters.timeouts, name + ": timeout callback once");
        result.expectTrue(helper->guardActive(), name + ": still guarded before release");
        helper->unobservedEvent();
        expectAccounting(result, counters, 1, name);
    }

    void registrationFailure(TestResult& result, bool afterObservation, const std::string& name) {
        Counters counters;
        auto* helper = new LifecycleModel(counters);
        if (afterObservation) {
            helper->enqueue(Outcome::WantRead);
            helper->enqueue(Outcome::WantWrite);
            helper->failNextWriteEnable(ENOSPC);
            helper->start();
            helper->readEvent();
            result.expectEqual(1, counters.errors, name + ": error callback once");
            result.expectEqual(ENOSPC, counters.lastErrno, name + ": registration errno retained");
            result.expectTrue(helper->guardActive(), name + ": deferred release after partial registration");
            helper->unobservedEvent();
        } else {
            helper->enqueue(Outcome::WantRead);
            helper->failNextReadEnable(EMFILE);
            helper->start();
            result.expectEqual(1, counters.errors, name + ": error callback once");
            result.expectEqual(EMFILE, counters.lastErrno, name + ": registration errno retained");
        }
        expectAccounting(result, counters, 1, name);
    }

    void queuedEventAfterCompletion(TestResult& result, const std::string& name) {
        Counters counters;
        auto* helper = new LifecycleModel(counters);
        helper->enqueue(Outcome::WantRead);
        helper->enqueue(Outcome::Success);
        helper->start();
        helper->readEvent();
        const int callsAfterCompletion = counters.operationCalls;
        helper->readEvent();
        helper->writeEvent();
        helper->readTimeout();
        result.expectEqual(callsAfterCompletion, counters.operationCalls, name + ": no operation after completion");
        result.expectEqual(1, counters.successes, name + ": no second callback");
        helper->unobservedEvent();
        expectAccounting(result, counters, 1, name);
    }

    void sequentialHelpers(TestResult& result, const std::string& name) {
        Counters counters;
        auto* first = new LifecycleModel(counters);
        first->enqueue(Outcome::WantRead);
        first->enqueue(Outcome::Success);
        first->start();
        result.expectTrue(first->guardActive(), name + ": first guard active");
        const bool secondRejected = first->guardActive();
        result.expectTrue(secondRejected, name + ": concurrent helper rejected by active guard");
        first->readEvent();
        first->unobservedEvent();

        auto* second = new LifecycleModel(counters);
        second->enqueue(Outcome::Success);
        second->start();
        result.expectEqual(2, counters.successes, name + ": sequential helper ran");
        result.expectEqual(2, counters.constructed, name + ": second helper constructed after release");
        result.expectEqual(2, counters.destroyed, name + ": both helpers destroyed");
        result.expectEqual(0, counters.active, name + ": active count clear");
        result.expectEqual(1, counters.maxConcurrent, name + ": maximum concurrent remains one");
        result.expectEqual(2, counters.releases, name + ": release callback per helper");
    }

} // namespace

int main() {
    TestResult result;

    for (const std::string helper : {"handshake", "shutdown"}) {
        synchronousSuccess(result, helper + " synchronous success");
        synchronousError(result, Outcome::Error, helper + " synchronous generic error");
        synchronousError(result, Outcome::Error, helper + " synchronous zero-return-equivalent error");
        wantReadLifecycle(result, helper + " WANT_READ lifecycle");
        wantWriteLifecycle(result, helper + " WANT_WRITE lifecycle");
        switching(result, Outcome::WantRead, Outcome::WantWrite, Axis::Write, helper + " WANT_READ to WANT_WRITE");
        switching(result, Outcome::WantWrite, Outcome::WantRead, Axis::Read, helper + " WANT_WRITE to WANT_READ");
        repeatedReadiness(result, Outcome::WantRead, helper + " repeated WANT_READ");
        repeatedReadiness(result, Outcome::WantWrite, helper + " repeated WANT_WRITE");
        timeoutAfterObservation(result, Outcome::WantRead, helper + " read timeout");
        timeoutAfterObservation(result, Outcome::WantWrite, helper + " write timeout");
        registrationFailure(result, false, helper + " registration failure before observation");
        registrationFailure(result, true, helper + " partial registration failure after observation");
        queuedEventAfterCompletion(result, helper + " queued event after completion");
        sequentialHelpers(result, helper + " sequential/concurrent guard behavior");
    }

    return result.processResult();
}
