#ifndef CORE_SOCKET_STREAM_TLS_DETAIL_TLSLIFECYCLETESTACCESS_H
#define CORE_SOCKET_STREAM_TLS_DETAIL_TLSLIFECYCLETESTACCESS_H

#include "core/socket/stream/tls/TLSHandshake.h"
#include "core/socket/stream/tls/SocketConnection.h"
#include "core/socket/stream/tls/SocketReader.h"
#include "core/socket/stream/tls/SocketWriter.h"
#include "core/socket/stream/tls/TLSShutdown.h"
#include "core/socket/stream/tls/detail/TLSResult.h"

#include <cerrno>
#include <deque>
#include <functional>
#include <optional>
#include <openssl/ssl.h>
#include <string>

namespace core::socket::stream::tls::detail::test {

    struct OperationResult {
        int returnValue = 1;
        int sslError = SSL_ERROR_NONE;
        int systemError = 0;
        unsigned long openSslError = 0;
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
        std::optional<TlsShutdownSuccess> lastSuccess;

        void reset() {
            HelperStateBase::reset();
            last = nullptr;
            lastSuccess.reset();
        }
    };

    struct IoState : HelperStateBase {
    };

    HandshakeState& handshakeState();
    ShutdownState& shutdownState();
    IoState& readerState();
    IoState& writerState();

} // namespace core::socket::stream::tls::detail::test

namespace core::socket::stream::tls::detail {

    struct TLSLifecycleTestAccess {
        static void resetHandshake() { test::handshakeState().reset(); }
        static void resetShutdown() { test::shutdownState().reset(); }
        static void resetReader() { test::readerState().reset(); }
        static void resetWriter() { test::writerState().reset(); }

        static void enqueueHandshake(int sslError, int systemError = 0) { test::handshakeState().operations.push_back({sslError == SSL_ERROR_NONE ? 1 : -1, sslError, systemError, 0}); }
        static void enqueueHandshakeResult(int ret, int sslError, int systemError = 0, unsigned long openSslError = 0) { test::handshakeState().operations.push_back({ret, sslError, systemError, openSslError}); }
        static void enqueueShutdown(int sslError, int systemError = 0) { test::shutdownState().operations.push_back({sslError == SSL_ERROR_NONE ? 1 : -1, sslError, systemError, 0}); }
        static void enqueueShutdownResult(int ret, int sslError, int systemError = 0, unsigned long openSslError = 0) { test::shutdownState().operations.push_back({ret, sslError, systemError, openSslError}); }
        static void enqueueReaderResult(int ret, int sslError, int systemError = 0, unsigned long openSslError = 0) { test::readerState().operations.push_back({ret, sslError, systemError, openSslError}); }
        static void enqueueWriterResult(int ret, int sslError, int systemError = 0, unsigned long openSslError = 0) { test::writerState().operations.push_back({ret, sslError, systemError, openSslError}); }

        static test::Counters handshakeCounters() { return test::handshakeState().counters; }
        static test::Counters shutdownCounters() { return test::shutdownState().counters; }
        static test::Counters readerCounters() { return test::readerState().counters; }
        static test::Counters writerCounters() { return test::writerState().counters; }
        static std::optional<TlsShutdownSuccess> lastShutdownSuccess() { return test::shutdownState().lastSuccess; }

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
            auto* helper = new TLSHandshake(instanceName, nullptr, onSuccess, onTimeout, onStatus, timeout, onReleased, fd);
            helper->start();
        }

        static void doShutdownForTest(const std::string& instanceName,
                                      int fd,
                                      const std::function<void()>& onSuccess,
                                      const std::function<void()>& onTimeout,
                                      const std::function<void(int)>& onStatus,
                                      const utils::Timeval& timeout,
                                      const std::function<void()>& onReleased) {
            auto* helper = new TLSShutdown(instanceName, nullptr, onSuccess, onTimeout, onStatus, timeout, onReleased, fd);
            helper->start();
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

        template <typename PhysicalSocket, typename Config>
        static SSL* startSSL(SocketConnection<PhysicalSocket, Config>& connection, int fd, SSL_CTX* ctx) {
            return connection.startSSL(fd, ctx);
        }

        template <typename PhysicalSocket, typename Config>
        static void stopSSL(SocketConnection<PhysicalSocket, Config>& connection) {
            connection.stopSSL();
        }

        template <typename PhysicalSocket, typename Config>
        static bool doSSLHandshake(SocketConnection<PhysicalSocket, Config>& connection,
                                   const std::function<void()>& onSuccess,
                                   const std::function<void()>& onTimeout,
                                   const std::function<void(int)>& onStatus) {
            return connection.doSSLHandshake(onSuccess, onTimeout, onStatus);
        }

        template <typename PhysicalSocket, typename Config>
        static void doSSLShutdown(SocketConnection<PhysicalSocket, Config>& connection) {
            connection.doSSLShutdown();
        }

        template <typename PhysicalSocket, typename Config>
        static bool handshakeGuardActive(const SocketConnection<PhysicalSocket, Config>& connection) {
            return connection.sslHandshakeInProgress != nullptr && *connection.sslHandshakeInProgress;
        }

        template <typename PhysicalSocket, typename Config>
        static bool shutdownGuardActive(const SocketConnection<PhysicalSocket, Config>& connection) {
            return connection.sslShutdownInProgress != nullptr && *connection.sslShutdownInProgress;
        }

        template <typename PhysicalSocket, typename Config>
        static bool tlsFatalError(const SocketConnection<PhysicalSocket, Config>& connection) {
            return connection.tlsFatalError;
        }

        template <typename PhysicalSocket, typename Config>
        static void triggerReadEvent(SocketConnection<PhysicalSocket, Config>& connection) {
            static_cast<core::socket::stream::SocketReader&>(connection).readEvent();
        }

        template <typename PhysicalSocket, typename Config>
        static void triggerWriteEvent(SocketConnection<PhysicalSocket, Config>& connection) {
            static_cast<core::socket::stream::SocketWriter&>(connection).writeEvent();
        }

        template <typename PhysicalSocket, typename Config>
        static bool readEnabled(const SocketConnection<PhysicalSocket, Config>& connection) {
            return static_cast<const core::socket::stream::SocketReader&>(connection).isEnabled();
        }

        template <typename PhysicalSocket, typename Config>
        static bool writeEnabled(const SocketConnection<PhysicalSocket, Config>& connection) {
            return static_cast<const core::socket::stream::SocketWriter&>(connection).isEnabled();
        }

        template <typename PhysicalSocket, typename Config>
        static bool readSuspended(const SocketConnection<PhysicalSocket, Config>& connection) {
            return static_cast<const core::socket::stream::SocketReader&>(connection).isSuspended();
        }

        template <typename PhysicalSocket, typename Config>
        static bool writeSuspended(const SocketConnection<PhysicalSocket, Config>& connection) {
            return static_cast<const core::socket::stream::SocketWriter&>(connection).isSuspended();
        }
    };

} // namespace core::socket::stream::tls::detail

#endif
