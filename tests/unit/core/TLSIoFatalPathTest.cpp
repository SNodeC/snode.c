#include "core/EventLoop.h"
#include "core/EventMultiplexer.h"
#include "core/DescriptorEventPublisher.h"
#include "core/socket/SocketAddress.h"
#include "core/socket/stream/SocketContext.h"
#include "core/socket/stream/tls/SocketConnection.hpp"
#include "core/socket/stream/tls/detail/TLSLifecycleTestAccess.h"
#include "net/config/ConfigInstance.h"
#include "tests/support/TestResult.h"

#include <cerrno>
#include <cstdint>
#include <functional>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>

using core::socket::stream::tls::TLSHandshake;
using core::socket::stream::tls::TLSShutdown;
using core::socket::stream::tls::detail::TLSLifecycleTestAccess;
using core::socket::stream::tls::detail::TlsShutdownSuccess;
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

    class TestSocketAddress : public core::socket::SocketAddress {
    public:
        using SockAddr = sockaddr_storage;
        using SockLen = socklen_t;
        std::string toString(bool = true) const override { return "tls-io-fatal-test-address"; }
    };

    class TestPhysicalSocket {
    public:
        using SocketAddress = TestSocketAddress;
        enum class SHUT { RD, WR, RDWR };
        explicit TestPhysicalSocket(int fd)
            : fd(fd) {}
        TestPhysicalSocket(TestPhysicalSocket&& other) noexcept
            : fd(other.fd) { other.fd = -1; }
        TestPhysicalSocket& operator=(TestPhysicalSocket&& other) noexcept {
            fd = other.fd;
            other.fd = -1;
            return *this;
        }
        int getFd() const { return fd; }
        int getSockName(sockaddr_storage&, socklen_t& len) { len = sizeof(sockaddr_storage); return 0; }
        int getPeerName(sockaddr_storage&, socklen_t& len) { len = sizeof(sockaddr_storage); return 0; }
        const TestSocketAddress& getBindAddress() const { return address; }
        int shutdown(SHUT) { return 0; }

    private:
        int fd = -1;
        TestSocketAddress address;
    };

    struct TestLocalConfig { static TestSocketAddress getSocketAddress(const sockaddr_storage&, socklen_t) { return {}; } };
    struct TestRemoteConfig { static TestSocketAddress getSocketAddress(const sockaddr_storage&, socklen_t) { return {}; } };

    class TestConfig : public net::config::ConfigInstance, public TestLocalConfig, public TestRemoteConfig {
    public:
        using Local = TestLocalConfig;
        using Remote = TestRemoteConfig;
        TestConfig()
            : ConfigInstance("tls-io-fatal-connection", Role::CLIENT) {}
        utils::Timeval getInitTimeout() const { return {1, 0}; }
        utils::Timeval getShutdownTimeout() const { return {1, 0}; }
        bool getNoCloseNotifyIsEOF() const { return false; }
        utils::Timeval getReadTimeout() const { return {1, 0}; }
        utils::Timeval getWriteTimeout() const { return {1, 0}; }
        utils::Timeval getTerminateTimeout() const { return {1, 0}; }
        std::size_t getReadBlockSize() const { return 1024; }
        std::size_t getWriteBlockSize() const { return 1024; }
    };

    using TestConnection = core::socket::stream::tls::SocketConnection<TestPhysicalSocket, TestConfig>;

    struct SslContext {
        SSL_CTX* ctx = SSL_CTX_new(TLS_method());
        ~SslContext() { SSL_CTX_free(ctx); }
    };

    class CountingContext : public core::socket::stream::SocketContext {
    public:
        explicit CountingContext(TestConnection* connection)
            : core::socket::stream::SocketContext(connection) {}

        int readErrors = 0;
        int writeErrors = 0;
        int lastReadError = 0;
        int lastWriteError = 0;
        int connected = 0;
        int disconnected = 0;
        int received = 0;

    private:
        void onConnected() override { connected++; }
        void onDisconnected() override { disconnected++; }
        std::size_t onReceivedFromPeer() override {
            received++;
            char buffer[32] = {};
            return readFromPeer(buffer, sizeof(buffer));
        }
        bool onSignal(int) override { return false; }
        void onReadError(int errnum) override {
            readErrors++;
            lastReadError = errnum;
            core::socket::stream::SocketContext::onReadError(errnum);
        }
        void onWriteError(int errnum) override {
            writeErrors++;
            lastWriteError = errnum;
            core::socket::stream::SocketContext::onWriteError(errnum);
        }
    };

    struct ConnectionFixture {
        PipeFd pipeFd;
        SslContext sslContext;
        TestConnection* connection = nullptr;
        CountingContext* context = nullptr;

        ConnectionFixture() {
            auto config = std::make_shared<TestConfig>();
            connection = new TestConnection(TestPhysicalSocket(pipeFd.fd()), [this](TestConnection*) {
                connection = nullptr;
                context = nullptr;
            }, std::uint64_t{1}, config);
            TLSLifecycleTestAccess::startSSL(*connection, pipeFd.fd(), sslContext.ctx);
            TLSLifecycleTestAccess::transitionTo(*connection, 2);
            TLSLifecycleTestAccess::transitionTo(*connection, 3);
            context = new CountingContext(connection);
            connection->setSocketContext(context);
        }
        ~ConnectionFixture() {
            if (connection != nullptr) {
                TLSLifecycleTestAccess::stopSSL(*connection);
                connection->close();
                releaseDisabledEvents();
                if (connection != nullptr) {
                    delete connection;
                    connection = nullptr;
                    context = nullptr;
                }
            }
        }
    };

    void resetTlsState() {
        TLSLifecycleTestAccess::resetReader();
        TLSLifecycleTestAccess::resetWriter();
        TLSLifecycleTestAccess::resetHandshake();
        TLSLifecycleTestAccess::resetShutdown();
    }

    void expectFatalRead(TestResult& result, const std::string& name, int ret, int sslError, int systemError, unsigned long openSslError, int expectedErrno) {
        resetTlsState();
        ConnectionFixture fixture;
        TLSLifecycleTestAccess::enqueueReaderResult(ret, sslError, systemError, openSslError);
        TLSLifecycleTestAccess::triggerReadEvent(*fixture.connection);
        result.expectEqual(1, fixture.context->readErrors, name + ": one read error");
        result.expectEqual(expectedErrno, fixture.context->lastReadError, name + ": mapped read errno");
        result.expectEqual(0, fixture.context->writeErrors, name + ": no writer error callback");
        result.expectTrue(TLSLifecycleTestAccess::tlsFatalError(*fixture.connection), name + ": fatal marker set");
        result.expectTrue(!TLSLifecycleTestAccess::readEnabled(*fixture.connection), name + ": read disabled");
        result.expectTrue(!TLSLifecycleTestAccess::writeEnabled(*fixture.connection), name + ": write disabled");
        result.expectEqual(0, TLSLifecycleTestAccess::shutdownCounters().constructed, name + ": no TLSShutdown helper");
        result.expectEqual(0, SSL_get_shutdown(fixture.connection->getSSL()), name + ": no fake shutdown bits");
    }

    void expectFatalWrite(TestResult& result, const std::string& name, int ret, int sslError, int systemError, unsigned long openSslError, int expectedErrno) {
        resetTlsState();
        ConnectionFixture fixture;
        fixture.connection->sendToPeer("x", 1);
        TLSLifecycleTestAccess::enqueueWriterResult(ret, sslError, systemError, openSslError);
        TLSLifecycleTestAccess::triggerWriteEvent(*fixture.connection);
        result.expectEqual(1, fixture.context->writeErrors, name + ": one write error");
        result.expectEqual(expectedErrno, fixture.context->lastWriteError, name + ": mapped write errno");
        result.expectEqual(0, fixture.context->readErrors, name + ": no reader error callback");
        result.expectTrue(TLSLifecycleTestAccess::tlsFatalError(*fixture.connection), name + ": fatal marker set");
        result.expectTrue(!TLSLifecycleTestAccess::readEnabled(*fixture.connection), name + ": read disabled");
        result.expectTrue(!TLSLifecycleTestAccess::writeEnabled(*fixture.connection), name + ": write disabled");
        result.expectEqual(0, TLSLifecycleTestAccess::shutdownCounters().constructed, name + ": no TLSShutdown helper");
        result.expectEqual(0, SSL_get_shutdown(fixture.connection->getSSL()), name + ": no fake shutdown bits");
    }

    void nonFatalIoPaths(TestResult& result) {
        resetTlsState();
        {
            ConnectionFixture fixture;
            TLSLifecycleTestAccess::enqueueReaderResult(3, SSL_ERROR_NONE);
            TLSLifecycleTestAccess::triggerReadEvent(*fixture.connection);
            result.expectEqual(1, fixture.context->received, "reader success delivered to context");
            result.expectEqual(0, fixture.context->readErrors, "reader success no error");
        }
        {
            ConnectionFixture fixture;
            TLSLifecycleTestAccess::enqueueReaderResult(-1, SSL_ERROR_WANT_READ);
            TLSLifecycleTestAccess::triggerReadEvent(*fixture.connection);
            result.expectEqual(0, fixture.context->readErrors, "reader WANT_READ no error");
            result.expectTrue(TLSLifecycleTestAccess::readEnabled(*fixture.connection), "reader WANT_READ remains enabled");
        }
        {
            ConnectionFixture fixture;
            TLSLifecycleTestAccess::enqueueReaderResult(-1, SSL_ERROR_WANT_WRITE);
            TLSLifecycleTestAccess::enqueueHandshakeResult(-1, SSL_ERROR_WANT_READ);
            TLSLifecycleTestAccess::triggerReadEvent(*fixture.connection);
            result.expectEqual(1, TLSLifecycleTestAccess::handshakeCounters().constructed, "reader WANT_WRITE starts one handshake");
            TLSLifecycleTestAccess::readTimeout(TLSLifecycleTestAccess::lastHandshake());
            releaseDisabledEvents();
        }
    }

    void laterHandshakePaths(TestResult& result) {
        resetTlsState();
        {
            ConnectionFixture fixture;
            TLSLifecycleTestAccess::enqueueReaderResult(-1, SSL_ERROR_WANT_WRITE);
            TLSLifecycleTestAccess::enqueueHandshakeResult(-1, SSL_ERROR_WANT_READ);
            TLSLifecycleTestAccess::triggerReadEvent(*fixture.connection);
            result.expectEqual(1, TLSLifecycleTestAccess::handshakeCounters().constructed, "reader later handshake timeout constructed");
            result.expectTrue(TLSLifecycleTestAccess::handshakeGuardActive(*fixture.connection), "reader later handshake guard active");
            TLSLifecycleTestAccess::readTimeout(TLSLifecycleTestAccess::lastHandshake());
            result.expectEqual(1, TLSLifecycleTestAccess::handshakeCounters().timeouts, "reader later handshake timeout callback");
            result.expectTrue(!TLSLifecycleTestAccess::readEnabled(*fixture.connection), "reader later handshake timeout closes read");
            result.expectTrue(!TLSLifecycleTestAccess::writeEnabled(*fixture.connection), "reader later handshake timeout closes write");
            releaseDisabledEvents();
            fixture.connection = nullptr;
        }
        resetTlsState();
        {
            ConnectionFixture fixture;
            TLSLifecycleTestAccess::enqueueReaderResult(-1, SSL_ERROR_WANT_WRITE);
            TLSLifecycleTestAccess::enqueueHandshakeResult(-1, SSL_ERROR_SSL, 0, ERR_PACK(ERR_LIB_SSL, 0, SSL_R_BAD_RECORD_TYPE));
            TLSLifecycleTestAccess::triggerReadEvent(*fixture.connection);
            result.expectEqual(1, TLSLifecycleTestAccess::handshakeCounters().constructed, "reader later handshake fatal constructed");
            result.expectEqual(1, TLSLifecycleTestAccess::handshakeCounters().errors, "reader later handshake fatal callback");
            result.expectTrue(!TLSLifecycleTestAccess::readEnabled(*fixture.connection), "reader later handshake fatal closes read");
            result.expectTrue(!TLSLifecycleTestAccess::writeEnabled(*fixture.connection), "reader later handshake fatal closes write");
            releaseDisabledEvents();
            fixture.connection = nullptr;
        }
        resetTlsState();
        {
            ConnectionFixture fixture;
            TLSLifecycleTestAccess::enqueueWriterResult(-1, SSL_ERROR_WANT_READ);
            TLSLifecycleTestAccess::enqueueHandshakeResult(-1, SSL_ERROR_WANT_READ);
            fixture.connection->sendToPeer("x", 1);
            TLSLifecycleTestAccess::triggerWriteEvent(*fixture.connection);
            result.expectEqual(1, TLSLifecycleTestAccess::handshakeCounters().constructed, "writer later handshake timeout constructed");
            result.expectTrue(TLSLifecycleTestAccess::handshakeGuardActive(*fixture.connection), "writer later handshake timeout guard active");
            TLSLifecycleTestAccess::readTimeout(TLSLifecycleTestAccess::lastHandshake());
            result.expectEqual(1, TLSLifecycleTestAccess::handshakeCounters().timeouts, "writer later handshake timeout callback");
            result.expectTrue(!TLSLifecycleTestAccess::readEnabled(*fixture.connection), "writer later handshake timeout closes read");
            result.expectTrue(!TLSLifecycleTestAccess::writeEnabled(*fixture.connection), "writer later handshake timeout closes write");
            releaseDisabledEvents();
            fixture.connection = nullptr;
        }
        resetTlsState();
        {
            ConnectionFixture fixture;
            TLSLifecycleTestAccess::enqueueWriterResult(-1, SSL_ERROR_WANT_READ);
            TLSLifecycleTestAccess::enqueueHandshakeResult(-1, SSL_ERROR_SSL, 0, ERR_PACK(ERR_LIB_SSL, 0, SSL_R_BAD_RECORD_TYPE));
            fixture.connection->sendToPeer("x", 1);
            TLSLifecycleTestAccess::triggerWriteEvent(*fixture.connection);
            result.expectEqual(1, TLSLifecycleTestAccess::handshakeCounters().constructed, "writer later handshake fatal constructed");
            result.expectEqual(1, TLSLifecycleTestAccess::handshakeCounters().errors, "writer later handshake fatal callback");
            result.expectTrue(!TLSLifecycleTestAccess::readEnabled(*fixture.connection), "writer later handshake fatal closes read");
            result.expectTrue(!TLSLifecycleTestAccess::writeEnabled(*fixture.connection), "writer later handshake fatal closes write");
            releaseDisabledEvents();
            fixture.connection = nullptr;
        }
    }

    struct HelperCallbacks {
        int success = 0;
        int error = 0;
        int released = 0;
        int lastStatus = 0;
        int lastErrno = 0;
    };

    void handshakeErrorResult(TestResult& result, const std::string& name, int ret, int sslError, int systemError, unsigned long openSslError, int expectedErrno) {
        PipeFd pipeFd;
        HelperCallbacks callbacks;
        TLSLifecycleTestAccess::resetHandshake();
        TLSLifecycleTestAccess::enqueueHandshakeResult(ret, sslError, systemError, openSslError);
        TLSLifecycleTestAccess::doHandshakeForTest("handshake-test", pipeFd.fd(), [&] { callbacks.success++; }, [] {}, [&](int status) {
            callbacks.error++;
            callbacks.lastStatus = status;
            callbacks.lastErrno = errno;
        }, {1, 0}, [&] { callbacks.released++; });
        result.expectEqual(0, callbacks.success, name + ": no success");
        result.expectEqual(1, callbacks.error, name + ": error once");
        result.expectEqual(sslError, callbacks.lastStatus, name + ": original ssl error");
        result.expectEqual(expectedErrno, callbacks.lastErrno, name + ": mapped errno");
        result.expectEqual(1, callbacks.released, name + ": release once");
    }

    void shutdownSuccessResults(TestResult& result) {
        PipeFd pipeFd;
        HelperCallbacks callbacks;
        TLSLifecycleTestAccess::resetShutdown();
        TLSLifecycleTestAccess::enqueueShutdownResult(0, SSL_ERROR_NONE);
        TLSLifecycleTestAccess::doShutdownForTest("shutdown-test", pipeFd.fd(), [&] { callbacks.success++; }, [] {}, [&](int) { callbacks.error++; }, {1, 0}, [&] { callbacks.released++; });
        result.expectEqual(1, callbacks.success, "shutdown ret 0 success callback");
        result.expectTrue(TLSLifecycleTestAccess::lastShutdownSuccess().has_value(), "shutdown ret 0 success captured");
        result.expectTrue(*TLSLifecycleTestAccess::lastShutdownSuccess() == TlsShutdownSuccess::CloseNotifySent, "shutdown ret 0 CloseNotifySent");

        callbacks = {};
        TLSLifecycleTestAccess::resetShutdown();
        TLSLifecycleTestAccess::enqueueShutdownResult(1, SSL_ERROR_NONE);
        TLSLifecycleTestAccess::doShutdownForTest("shutdown-test", pipeFd.fd(), [&] { callbacks.success++; }, [] {}, [&](int) { callbacks.error++; }, {1, 0}, [&] { callbacks.released++; });
        result.expectEqual(1, callbacks.success, "shutdown ret 1 success callback");
        result.expectTrue(TLSLifecycleTestAccess::lastShutdownSuccess().has_value(), "shutdown ret 1 success captured");
        result.expectTrue(*TLSLifecycleTestAccess::lastShutdownSuccess() == TlsShutdownSuccess::FullShutdownComplete, "shutdown ret 1 FullShutdownComplete");
    }

} // namespace

int main() {
    TestResult result;

    nonFatalIoPaths(result);

    expectFatalRead(result, "reader 111 eof", 0, SSL_ERROR_SYSCALL, 0, 0, EPROTO);
#if defined(SSL_R_UNEXPECTED_EOF_WHILE_READING)
    const unsigned long unexpectedEof = ERR_PACK(ERR_LIB_SSL, 0, SSL_R_UNEXPECTED_EOF_WHILE_READING);
    expectFatalRead(result, "reader 3 eof", -1, SSL_ERROR_SSL, 0, unexpectedEof, EPROTO);
#endif
    expectFatalRead(result, "reader reset", -1, SSL_ERROR_SYSCALL, ECONNRESET, 0, ECONNRESET);
    expectFatalRead(result, "reader protocol", -1, SSL_ERROR_SSL, 0, ERR_PACK(ERR_LIB_SSL, 0, SSL_R_BAD_RECORD_TYPE), EPROTO);
    expectFatalRead(result, "reader unknown", -1, 12345, 0, 0, EIO);

    expectFatalWrite(result, "writer epipe", -1, SSL_ERROR_SYSCALL, EPIPE, 0, EPIPE);
    expectFatalWrite(result, "writer reset", -1, SSL_ERROR_SYSCALL, ECONNRESET, 0, ECONNRESET);
    expectFatalWrite(result, "writer 111 eof", 0, SSL_ERROR_SYSCALL, 0, 0, EPROTO);
#if defined(SSL_R_UNEXPECTED_EOF_WHILE_READING)
    expectFatalWrite(result, "writer 3 eof", -1, SSL_ERROR_SSL, 0, unexpectedEof, EPROTO);
#endif
    expectFatalWrite(result, "writer protocol", -1, SSL_ERROR_SSL, 0, ERR_PACK(ERR_LIB_SSL, 0, SSL_R_BAD_RECORD_TYPE), EPROTO);
    expectFatalWrite(result, "writer unknown", -1, 12345, 0, 0, EIO);

    laterHandshakePaths(result);

    handshakeErrorResult(result, "handshake zero return", -1, SSL_ERROR_ZERO_RETURN, 0, 0, EPROTO);
    handshakeErrorResult(result, "handshake 111 eof", 0, SSL_ERROR_SYSCALL, 0, 0, EPROTO);
#if defined(SSL_R_UNEXPECTED_EOF_WHILE_READING)
    handshakeErrorResult(result, "handshake 3 eof", -1, SSL_ERROR_SSL, 0, unexpectedEof, EPROTO);
#endif
    handshakeErrorResult(result, "handshake reset", -1, SSL_ERROR_SYSCALL, ECONNRESET, 0, ECONNRESET);
    handshakeErrorResult(result, "handshake protocol", -1, SSL_ERROR_SSL, 0, ERR_PACK(ERR_LIB_SSL, 0, SSL_R_BAD_RECORD_TYPE), EPROTO);
    handshakeErrorResult(result, "handshake unknown", -1, 12345, 0, 0, EIO);

    shutdownSuccessResults(result);

    return result.processResult();
}
