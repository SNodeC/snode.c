#ifndef CORE_SOCKET_STREAM_TLS_DETAIL_TLSLIFECYCLETESTACCESS_H
#define CORE_SOCKET_STREAM_TLS_DETAIL_TLSLIFECYCLETESTACCESS_H

#include "core/socket/stream/tls/TLSHandshake.h"
#include "core/socket/stream/tls/TLSShutdown.h"

#include <cerrno>
#include <deque>
#include <functional>
#include <openssl/ssl.h>
#include <string>

namespace core::socket::stream::tls::detail::test {

    struct OperationResult {
        int sslError = SSL_ERROR_NONE;
        int systemError = 0;
    };

    struct Counters {
        int constructed = 0;
        int destroyed = 0;
        int active = 0;
        int maxConcurrent = 0;
        int operationCalls = 0;
        int successes = 0;
        int timeouts = 0;
        int errors = 0;
        int releases = 0;
        int lastStatus = SSL_ERROR_NONE;
        int lastErrno = 0;
    };

    struct HelperStateBase {
        std::deque<OperationResult> operations;
        Counters counters;
        int failNextReadEnable = 0;
        int failNextWriteEnable = 0;

        void reset() {
            operations.clear();
            counters = {};
            failNextReadEnable = 0;
            failNextWriteEnable = 0;
        }
    };

    struct HandshakeState : HelperStateBase {
        TLSHandshake* last = nullptr;
    };

    struct ShutdownState : HelperStateBase {
        TLSShutdown* last = nullptr;
    };

    HandshakeState& handshakeState();
    ShutdownState& shutdownState();

} // namespace core::socket::stream::tls::detail::test

namespace core::socket::stream::tls::detail {

    struct TLSLifecycleTestAccess {
        static void resetHandshake() { test::handshakeState().reset(); }
        static void resetShutdown() { test::shutdownState().reset(); }

        static void enqueueHandshake(int sslError, int systemError = 0) { test::handshakeState().operations.push_back({sslError, systemError}); }
        static void enqueueShutdown(int sslError, int systemError = 0) { test::shutdownState().operations.push_back({sslError, systemError}); }

        static test::Counters handshakeCounters() { return test::handshakeState().counters; }
        static test::Counters shutdownCounters() { return test::shutdownState().counters; }

        static TLSHandshake* lastHandshake() { return test::handshakeState().last; }
        static TLSShutdown* lastShutdown() { return test::shutdownState().last; }

        static void failNextHandshakeReadEnable(int error) { test::handshakeState().failNextReadEnable = error; }
        static void failNextHandshakeWriteEnable(int error) { test::handshakeState().failNextWriteEnable = error; }
        static void failNextShutdownReadEnable(int error) { test::shutdownState().failNextReadEnable = error; }
        static void failNextShutdownWriteEnable(int error) { test::shutdownState().failNextWriteEnable = error; }

        static void doHandshakeForTest(const std::string& instanceName,
                                       int fd,
                                       const std::function<void()>& onSuccess,
                                       const std::function<void()>& onTimeout,
                                       const std::function<void(int)>& onStatus,
                                       const utils::Timeval& timeout,
                                       const std::function<void()>& onReleased) {
            TLSHandshake::doHandshakeForTest(instanceName, fd, onSuccess, onTimeout, onStatus, timeout, onReleased);
        }

        static void doShutdownForTest(const std::string& instanceName,
                                      int fd,
                                      const std::function<void()>& onSuccess,
                                      const std::function<void()>& onTimeout,
                                      const std::function<void(int)>& onStatus,
                                      const utils::Timeval& timeout,
                                      const std::function<void()>& onReleased) {
            TLSShutdown::doShutdownForTest(instanceName, fd, onSuccess, onTimeout, onStatus, timeout, onReleased);
        }

        static void readEvent(TLSHandshake* helper) { helper->readEvent(); }
        static void writeEvent(TLSHandshake* helper) { helper->writeEvent(); }
        static void readTimeout(TLSHandshake* helper) { helper->readTimeout(); }
        static void writeTimeout(TLSHandshake* helper) { helper->writeTimeout(); }
        static bool readEnabled(TLSHandshake* helper) { return helper->ReadEventReceiver::isEnabled(); }
        static bool writeEnabled(TLSHandshake* helper) { return helper->WriteEventReceiver::isEnabled(); }
        static bool readSuspended(TLSHandshake* helper) { return helper->ReadEventReceiver::isSuspended(); }
        static bool writeSuspended(TLSHandshake* helper) { return helper->WriteEventReceiver::isSuspended(); }
        static bool completed(TLSHandshake* helper) { return helper->completed; }

        static void readEvent(TLSShutdown* helper) { helper->readEvent(); }
        static void writeEvent(TLSShutdown* helper) { helper->writeEvent(); }
        static void readTimeout(TLSShutdown* helper) { helper->readTimeout(); }
        static void writeTimeout(TLSShutdown* helper) { helper->writeTimeout(); }
        static bool readEnabled(TLSShutdown* helper) { return helper->ReadEventReceiver::isEnabled(); }
        static bool writeEnabled(TLSShutdown* helper) { return helper->WriteEventReceiver::isEnabled(); }
        static bool readSuspended(TLSShutdown* helper) { return helper->ReadEventReceiver::isSuspended(); }
        static bool writeSuspended(TLSShutdown* helper) { return helper->WriteEventReceiver::isSuspended(); }
        static bool completed(TLSShutdown* helper) { return helper->completed; }
    };

} // namespace core::socket::stream::tls::detail

#endif
