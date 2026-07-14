#include "core/EventLoop.h"
#include "core/EventMultiplexer.h"
#include "core/DescriptorEventPublisher.h"
#include "core/socket/stream/tls/detail/TLSLifecycleTestAccess.h"
#include "tests/support/TestResult.h"

#include <cerrno>
#include <functional>
#include <openssl/ssl.h>
#include <string>
#include <unistd.h>

using core::socket::stream::tls::TLSHandshake;
using core::socket::stream::tls::TLSShutdown;
using core::socket::stream::tls::detail::TLSLifecycleTestAccess;
using core::socket::stream::tls::detail::test::Counters;
using tests::support::TestResult;

namespace {

    struct PipeFd {
        int fds[2] = {-1, -1};
        PipeFd() { pipe(fds); }
        ~PipeFd() {
            if (fds[0] >= 0) close(fds[0]);
            if (fds[1] >= 0) close(fds[1]);
        }
        int fd() const { return fds[0]; }
    };

    void releaseDisabledEvents() {
        const utils::Timeval now = utils::Timeval::currentTime();
        core::EventLoop::instance().getEventMultiplexer().getDescriptorEventPublisher(core::EventMultiplexer::DISP_TYPE::RD).releaseDisabledEvents(now);
        core::EventLoop::instance().getEventMultiplexer().getDescriptorEventPublisher(core::EventMultiplexer::DISP_TYPE::WR).releaseDisabledEvents(now);
    }

    struct CallbackCounts {
        int success = 0;
        int timeout = 0;
        int error = 0;
        int released = 0;
        int lastStatus = SSL_ERROR_NONE;
        int lastErrno = 0;
    };

    template <typename Helper>
    struct HelperOps;

    template <>
    struct HelperOps<TLSHandshake> {
        static void reset() { TLSLifecycleTestAccess::resetHandshake(); }
        static void enqueue(int sslError, int systemError = 0) { TLSLifecycleTestAccess::enqueueHandshake(sslError, systemError); }
        static Counters counters() { return TLSLifecycleTestAccess::handshakeCounters(); }
        static TLSHandshake* last() { return TLSLifecycleTestAccess::lastHandshake(); }
        static void start(int fd, CallbackCounts& callbacks) {
            TLSLifecycleTestAccess::doHandshakeForTest(
                "test", fd, [&] { callbacks.success++; }, [&] { callbacks.timeout++; },
                [&](int status) { callbacks.error++; callbacks.lastStatus = status; callbacks.lastErrno = errno; }, {1, 0}, [&] { callbacks.released++; });
        }
        static void readEvent(TLSHandshake* helper) { TLSLifecycleTestAccess::readEvent(helper); }
        static void writeEvent(TLSHandshake* helper) { TLSLifecycleTestAccess::writeEvent(helper); }
        static void readTimeout(TLSHandshake* helper) { TLSLifecycleTestAccess::readTimeout(helper); }
        static void writeTimeout(TLSHandshake* helper) { TLSLifecycleTestAccess::writeTimeout(helper); }
        static bool readEnabled(TLSHandshake* helper) { return TLSLifecycleTestAccess::readEnabled(helper); }
        static bool writeEnabled(TLSHandshake* helper) { return TLSLifecycleTestAccess::writeEnabled(helper); }
        static bool readSuspended(TLSHandshake* helper) { return TLSLifecycleTestAccess::readSuspended(helper); }
        static bool writeSuspended(TLSHandshake* helper) { return TLSLifecycleTestAccess::writeSuspended(helper); }
        static void failReadEnable(int err) { TLSLifecycleTestAccess::failNextHandshakeReadEnable(err); }
        static void failWriteEnable(int err) { TLSLifecycleTestAccess::failNextHandshakeWriteEnable(err); }
    };

    template <>
    struct HelperOps<TLSShutdown> {
        static void reset() { TLSLifecycleTestAccess::resetShutdown(); }
        static void enqueue(int sslError, int systemError = 0) { TLSLifecycleTestAccess::enqueueShutdown(sslError, systemError); }
        static Counters counters() { return TLSLifecycleTestAccess::shutdownCounters(); }
        static TLSShutdown* last() { return TLSLifecycleTestAccess::lastShutdown(); }
        static void start(int fd, CallbackCounts& callbacks) {
            TLSLifecycleTestAccess::doShutdownForTest(
                "test", fd, [&] { callbacks.success++; }, [&] { callbacks.timeout++; },
                [&](int status) { callbacks.error++; callbacks.lastStatus = status; callbacks.lastErrno = errno; }, {1, 0}, [&] { callbacks.released++; });
        }
        static void readEvent(TLSShutdown* helper) { TLSLifecycleTestAccess::readEvent(helper); }
        static void writeEvent(TLSShutdown* helper) { TLSLifecycleTestAccess::writeEvent(helper); }
        static void readTimeout(TLSShutdown* helper) { TLSLifecycleTestAccess::readTimeout(helper); }
        static void writeTimeout(TLSShutdown* helper) { TLSLifecycleTestAccess::writeTimeout(helper); }
        static bool readEnabled(TLSShutdown* helper) { return TLSLifecycleTestAccess::readEnabled(helper); }
        static bool writeEnabled(TLSShutdown* helper) { return TLSLifecycleTestAccess::writeEnabled(helper); }
        static bool readSuspended(TLSShutdown* helper) { return TLSLifecycleTestAccess::readSuspended(helper); }
        static bool writeSuspended(TLSShutdown* helper) { return TLSLifecycleTestAccess::writeSuspended(helper); }
        static void failReadEnable(int err) { TLSLifecycleTestAccess::failNextShutdownReadEnable(err); }
        static void failWriteEnable(int err) { TLSLifecycleTestAccess::failNextShutdownWriteEnable(err); }
    };

    template <typename Helper>
    void expectAccounting(TestResult& result, const std::string& name, int helpers) {
        const Counters counters = HelperOps<Helper>::counters();
        result.expectEqual(helpers, counters.constructed, name + ": constructed");
        result.expectEqual(helpers, counters.destroyed, name + ": destroyed");
        result.expectEqual(0, counters.active, name + ": active returns to zero");
        result.expectEqual(helpers, counters.releases, name + ": releases");
        result.expectTrue(counters.maxConcurrent <= 1, name + ": max concurrent <= 1");
    }

    template <typename Helper>
    void synchronousSuccess(TestResult& result, const std::string& name) {
        PipeFd pipeFd;
        CallbackCounts callbacks;
        HelperOps<Helper>::reset();
        HelperOps<Helper>::enqueue(SSL_ERROR_NONE);
        HelperOps<Helper>::start(pipeFd.fd(), callbacks);
        result.expectEqual(1, callbacks.success, name + ": success callback once");
        result.expectEqual(1, callbacks.released, name + ": release callback once");
        result.expectTrue(HelperOps<Helper>::last() == nullptr, name + ": destroyed synchronously");
        expectAccounting<Helper>(result, name, 1);
    }

    template <typename Helper>
    void synchronousError(TestResult& result, int sslError, const std::string& name) {
        PipeFd pipeFd;
        CallbackCounts callbacks;
        HelperOps<Helper>::reset();
        HelperOps<Helper>::enqueue(sslError, EPROTO);
        HelperOps<Helper>::start(pipeFd.fd(), callbacks);
        result.expectEqual(1, callbacks.error, name + ": error callback once");
        result.expectEqual(sslError, callbacks.lastStatus, name + ": status retained");
        result.expectEqual(EPROTO, callbacks.lastErrno, name + ": errno retained");
        result.expectEqual(1, callbacks.released, name + ": release callback once");
        expectAccounting<Helper>(result, name, 1);
    }

    template <typename Helper>
    void wantReadWrite(TestResult& result, int initial, const std::string& name) {
        PipeFd pipeFd;
        CallbackCounts callbacks;
        HelperOps<Helper>::reset();
        HelperOps<Helper>::enqueue(initial);
        HelperOps<Helper>::enqueue(SSL_ERROR_NONE);
        HelperOps<Helper>::start(pipeFd.fd(), callbacks);
        auto* helper = HelperOps<Helper>::last();
        result.expectTrue(helper != nullptr, name + ": helper observed");
        if (initial == SSL_ERROR_WANT_READ) {
            result.expectTrue(HelperOps<Helper>::readEnabled(helper), name + ": read enabled");
            HelperOps<Helper>::readEvent(helper);
        } else {
            result.expectTrue(HelperOps<Helper>::writeEnabled(helper), name + ": write enabled");
            HelperOps<Helper>::writeEvent(helper);
        }
        result.expectEqual(1, callbacks.success, name + ": success once");
        result.expectEqual(0, callbacks.released, name + ": not released before publisher cleanup");
        result.expectEqual(1, HelperOps<Helper>::counters().active, name + ": alive before cleanup");
        releaseDisabledEvents();
        result.expectEqual(1, callbacks.released, name + ": released by publisher cleanup");
        expectAccounting<Helper>(result, name, 1);
    }

    template <typename Helper>
    void switching(TestResult& result, int first, int second, const std::string& name) {
        PipeFd pipeFd;
        CallbackCounts callbacks;
        HelperOps<Helper>::reset();
        HelperOps<Helper>::enqueue(first);
        HelperOps<Helper>::enqueue(second);
        HelperOps<Helper>::enqueue(SSL_ERROR_NONE);
        HelperOps<Helper>::start(pipeFd.fd(), callbacks);
        auto* helper = HelperOps<Helper>::last();
        first == SSL_ERROR_WANT_READ ? HelperOps<Helper>::readEvent(helper) : HelperOps<Helper>::writeEvent(helper);
        result.expectTrue(HelperOps<Helper>::readEnabled(helper), name + ": read remains registered after switch if registered");
        result.expectTrue(HelperOps<Helper>::writeEnabled(helper), name + ": write remains registered after switch if registered");
        if (second == SSL_ERROR_WANT_READ) {
            result.expectTrue(HelperOps<Helper>::writeSuspended(helper), name + ": write suspended");
            HelperOps<Helper>::readEvent(helper);
        } else {
            result.expectTrue(HelperOps<Helper>::readSuspended(helper), name + ": read suspended");
            HelperOps<Helper>::writeEvent(helper);
        }
        result.expectEqual(1, callbacks.success, name + ": success once");
        releaseDisabledEvents();
        expectAccounting<Helper>(result, name, 1);
    }

    template <typename Helper>
    void repeatedReadiness(TestResult& result, int readiness, const std::string& name) {
        PipeFd pipeFd;
        CallbackCounts callbacks;
        HelperOps<Helper>::reset();
        HelperOps<Helper>::enqueue(readiness);
        HelperOps<Helper>::enqueue(readiness);
        HelperOps<Helper>::enqueue(SSL_ERROR_NONE);
        HelperOps<Helper>::start(pipeFd.fd(), callbacks);
        auto* helper = HelperOps<Helper>::last();
        readiness == SSL_ERROR_WANT_READ ? HelperOps<Helper>::readEvent(helper) : HelperOps<Helper>::writeEvent(helper);
        readiness == SSL_ERROR_WANT_READ ? HelperOps<Helper>::readEvent(helper) : HelperOps<Helper>::writeEvent(helper);
        result.expectEqual(1, callbacks.success, name + ": success once");
        releaseDisabledEvents();
        expectAccounting<Helper>(result, name, 1);
    }

    template <typename Helper>
    void timeoutAfterObservation(TestResult& result, int readiness, const std::string& name) {
        PipeFd pipeFd;
        CallbackCounts callbacks;
        HelperOps<Helper>::reset();
        HelperOps<Helper>::enqueue(readiness);
        HelperOps<Helper>::start(pipeFd.fd(), callbacks);
        auto* helper = HelperOps<Helper>::last();
        readiness == SSL_ERROR_WANT_READ ? HelperOps<Helper>::readTimeout(helper) : HelperOps<Helper>::writeTimeout(helper);
        result.expectEqual(1, callbacks.timeout, name + ": timeout once");
        result.expectEqual(0, callbacks.released, name + ": deferred release");
        releaseDisabledEvents();
        expectAccounting<Helper>(result, name, 1);
    }

    template <typename Helper>
    void registrationFailure(TestResult& result, bool afterObservation, const std::string& name) {
        PipeFd pipeFd;
        CallbackCounts callbacks;
        HelperOps<Helper>::reset();
        if (afterObservation) {
            HelperOps<Helper>::enqueue(SSL_ERROR_WANT_READ);
            HelperOps<Helper>::enqueue(SSL_ERROR_WANT_WRITE);
            HelperOps<Helper>::failWriteEnable(ENOSPC);
            HelperOps<Helper>::start(pipeFd.fd(), callbacks);
            auto* helper = HelperOps<Helper>::last();
            HelperOps<Helper>::readEvent(helper);
            result.expectEqual(0, callbacks.released, name + ": release deferred after partial registration");
            releaseDisabledEvents();
        } else {
            HelperOps<Helper>::enqueue(SSL_ERROR_WANT_READ);
            HelperOps<Helper>::failReadEnable(EMFILE);
            HelperOps<Helper>::start(pipeFd.fd(), callbacks);
        }
        result.expectEqual(1, callbacks.error, name + ": error once");
        result.expectEqual(afterObservation ? ENOSPC : EMFILE, callbacks.lastErrno, name + ": registration errno retained");
        expectAccounting<Helper>(result, name, 1);
    }

    template <typename Helper>
    void queuedEventAfterCompletion(TestResult& result, const std::string& name) {
        PipeFd pipeFd;
        CallbackCounts callbacks;
        HelperOps<Helper>::reset();
        HelperOps<Helper>::enqueue(SSL_ERROR_WANT_READ);
        HelperOps<Helper>::enqueue(SSL_ERROR_NONE);
        HelperOps<Helper>::start(pipeFd.fd(), callbacks);
        auto* helper = HelperOps<Helper>::last();
        HelperOps<Helper>::readEvent(helper);
        const int callsAfterCompletion = HelperOps<Helper>::counters().operationCalls;
        HelperOps<Helper>::readEvent(helper);
        HelperOps<Helper>::writeEvent(helper);
        HelperOps<Helper>::readTimeout(helper);
        result.expectEqual(callsAfterCompletion, HelperOps<Helper>::counters().operationCalls, name + ": no operation after completion");
        result.expectEqual(1, callbacks.success, name + ": one semantic callback");
        releaseDisabledEvents();
        expectAccounting<Helper>(result, name, 1);
    }

    template <typename Helper>
    void sequentialGuard(TestResult& result, const std::string& name) {
        PipeFd pipeFd;
        CallbackCounts first;
        HelperOps<Helper>::reset();
        HelperOps<Helper>::enqueue(SSL_ERROR_WANT_READ);
        HelperOps<Helper>::enqueue(SSL_ERROR_NONE);
        HelperOps<Helper>::start(pipeFd.fd(), first);
        auto* helper = HelperOps<Helper>::last();
        result.expectEqual(1, HelperOps<Helper>::counters().constructed, name + ": first constructed");
        result.expectEqual(1, HelperOps<Helper>::counters().active, name + ": guard-active equivalent while observed");
        HelperOps<Helper>::readEvent(helper);
        result.expectEqual(1, HelperOps<Helper>::counters().active, name + ": still active before cleanup");
        releaseDisabledEvents();
        CallbackCounts second;
        HelperOps<Helper>::enqueue(SSL_ERROR_NONE);
        HelperOps<Helper>::start(pipeFd.fd(), second);
        result.expectEqual(2, HelperOps<Helper>::counters().constructed, name + ": sequential helper constructed after release");
        result.expectEqual(1, second.success, name + ": second helper completed");
        expectAccounting<Helper>(result, name, 2);
    }

    template <typename Helper>
    void runHelperSuite(TestResult& result, const std::string& name) {
        synchronousSuccess<Helper>(result, name + " synchronous success");
        synchronousError<Helper>(result, SSL_ERROR_SSL, name + " synchronous generic error");
        synchronousError<Helper>(result, SSL_ERROR_ZERO_RETURN, name + " synchronous zero return error");
        wantReadWrite<Helper>(result, SSL_ERROR_WANT_READ, name + " WANT_READ to success");
        wantReadWrite<Helper>(result, SSL_ERROR_WANT_WRITE, name + " WANT_WRITE to success");
        switching<Helper>(result, SSL_ERROR_WANT_READ, SSL_ERROR_WANT_WRITE, name + " WANT_READ to WANT_WRITE");
        switching<Helper>(result, SSL_ERROR_WANT_WRITE, SSL_ERROR_WANT_READ, name + " WANT_WRITE to WANT_READ");
        repeatedReadiness<Helper>(result, SSL_ERROR_WANT_READ, name + " repeated WANT_READ");
        repeatedReadiness<Helper>(result, SSL_ERROR_WANT_WRITE, name + " repeated WANT_WRITE");
        timeoutAfterObservation<Helper>(result, SSL_ERROR_WANT_READ, name + " read timeout");
        timeoutAfterObservation<Helper>(result, SSL_ERROR_WANT_WRITE, name + " write timeout");
        registrationFailure<Helper>(result, false, name + " registration failure before observation");
        registrationFailure<Helper>(result, true, name + " partial registration failure after observation");
        queuedEventAfterCompletion<Helper>(result, name + " queued event after completion");
        sequentialGuard<Helper>(result, name + " sequential helper after release");
    }

} // namespace

int main() {
    TestResult result;

    runHelperSuite<TLSHandshake>(result, "TLSHandshake");
    runHelperSuite<TLSShutdown>(result, "TLSShutdown");

    return result.processResult();
}
