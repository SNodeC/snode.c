#include <sstream>
#define private public
#define protected public
#include "core/socket/stream/tls/SocketReader.h"
#include "core/socket/stream/tls/SocketWriter.h"
#include "core/socket/stream/tls/TLSShutdown.h"
#include "core/socket/stream/tls/detail/TLSShutdownTestHooks.h"
#undef protected
#undef private

#include "support/TestResult.h"
#include "utils/Timeval.h"

#include <cerrno>
#include <cstring>
#include <openssl/ssl.h>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

namespace {
    class TestTlsReader : public core::socket::stream::tls::SocketReader {
    public:
        explicit TestTlsReader(std::vector<int>& statuses)
            : SocketReader("tls-lifecycle-reader", [&statuses](int status) { statuses.push_back(status); }, utils::Timeval{}, 32, utils::Timeval{}) {}

        using core::socket::stream::SocketReader::doRead;
        using core::socket::stream::SocketReader::readFromPeer;
        using core::eventreceiver::ReadEventReceiver::enable;
        using core::eventreceiver::ReadEventReceiver::disable;
        using core::socket::stream::tls::SocketReader::read;
        using core::socket::stream::tls::SocketReader::ssl;

        bool onReadShutdown(int statusCode) override {
            readShutdownStatuses.push_back(statusCode);
            return reportCleanEof;
        }

        bool doSSLHandshake(const std::function<void()>& onSuccess,
                            const std::function<void()>&,
                            const std::function<void(int)>&) override {
            onSuccess();
            return true;
        }

        void onReceivedFromPeer(std::size_t available) override {
            receivedCallbacks++;
            char buffer[64] = {};
            deliveredBytes += readFromPeer(buffer, available);
        }

        void unobservedEvent() override {}

        bool reportCleanEof = true;
        int receivedCallbacks = 0;
        std::size_t deliveredBytes = 0;
        std::vector<int> readShutdownStatuses;
    };

    class TestTlsWriter : public core::socket::stream::tls::SocketWriter {
    public:
        explicit TestTlsWriter(std::vector<int>& statuses)
            : SocketWriter("tls-lifecycle-writer", [&statuses](int status) { statuses.push_back(status); }, utils::Timeval{}, 32, utils::Timeval{}) {}

        using core::eventreceiver::WriteEventReceiver::enable;
        using core::eventreceiver::WriteEventReceiver::disable;
        using core::socket::stream::tls::SocketWriter::ssl;
        using core::socket::stream::tls::SocketWriter::write;

        bool doSSLHandshake(const std::function<void()>& onSuccess,
                            const std::function<void()>&,
                            const std::function<void(int)>&) override {
            onSuccess();
            return true;
        }

        bool onSignal(int) override { return false; }
        void doWriteShutdown(const std::function<void()>& onShutdown) override { onShutdown(); }
        void unobservedEvent() override {}
    };

    struct SocketPair {
        int fds[2] = {-1, -1};
        SocketPair() { ::socketpair(AF_UNIX, SOCK_STREAM, 0, fds); }
        ~SocketPair() {
            if (fds[0] >= 0) ::close(fds[0]);
            if (fds[1] >= 0) ::close(fds[1]);
        }
    };

    struct SslHandle {
        SSL_CTX* ctx = SSL_CTX_new(TLS_method());
        SSL* ssl = SSL_new(ctx);
        ~SslHandle() {
            SSL_free(ssl);
            SSL_CTX_free(ctx);
        }
    };
}

int main() {
    tests::support::TestResult result;

    {
        std::vector<int> statuses;
        SocketPair sockets;
        SslHandle ssl;
        TestTlsReader reader(statuses);
        reader.ssl = ssl.ssl;
        SSL_set_fd(ssl.ssl, sockets.fds[0]);
        SSL_set_shutdown(ssl.ssl, SSL_RECEIVED_SHUTDOWN);
        reader.enable(sockets.fds[0]);

        const char raw[] = "RAW-after-close-notify";
        ::send(sockets.fds[1], raw, sizeof(raw), 0);

        errno = 0;
        const std::size_t available = reader.doRead();
        result.expectEqual(0, static_cast<int>(available), "TLS reader reports no application data after SSL_RECEIVED_SHUTDOWN");
        result.expectEqual(1, static_cast<int>(statuses.size()), "clean close_notify produces exactly one read status");
        result.expectEqual(0, statuses.front(), "clean close_notify reports EOF status 0");
        result.expectEqual(0, reader.receivedCallbacks, "clean close_notify does not call data callback");

        char buffer[sizeof(raw)] = {};
        const ssize_t rawRead = ::recv(sockets.fds[0], buffer, sizeof(buffer), MSG_DONTWAIT);
        result.expectEqual(static_cast<int>(sizeof(raw)), static_cast<int>(rawRead), "raw bytes after close_notify are not consumed as TLS application data");
        reader.disable();
    }

    {
        std::vector<int> statuses;
        SocketPair sockets;
        SslHandle ssl;
        TestTlsWriter writer(statuses);
        writer.ssl = ssl.ssl;
        SSL_set_fd(ssl.ssl, sockets.fds[0]);
        SSL_set_shutdown(ssl.ssl, SSL_SENT_SHUTDOWN);
        writer.enable(sockets.fds[0]);

        const char payload[] = "must-not-be-raw";
        errno = 0;
        const ssize_t written = writer.write(payload, sizeof(payload));
        result.expectEqual(-1, static_cast<int>(written), "TLS writer rejects application write after SSL_SENT_SHUTDOWN");
        result.expectEqual(EPIPE, errno, "TLS writer reports EPIPE after SSL_SENT_SHUTDOWN");

        char buffer[sizeof(payload)] = {};
        const ssize_t rawRead = ::recv(sockets.fds[1], buffer, sizeof(buffer), MSG_DONTWAIT);
        result.expectEqual(-1, static_cast<int>(rawRead), "no raw application bytes are sent after TLS write shutdown");
        writer.disable();
    }

    {
        using namespace core::socket::stream::tls::detail::test;
        resetTlsShutdownCounters();
        SslHandle ssl;
        int success = 0;
        int timeout = 0;
        int status = 0;
        forceNextTlsShutdownResult({0, SSL_ERROR_NONE, 0, 0});
        core::socket::stream::tls::TLSShutdown::doShutdown(
            "tls-shutdown-return-0",
            ssl.ssl,
            [&success]() { success++; },
            [&timeout]() { timeout++; },
            [&status](int, int) { status++; },
            utils::Timeval{});
        result.expectEqual(1, success, "SSL_shutdown return 0 completes unidirectional TLS shutdown synchronously");
        result.expectEqual(0, timeout, "SSL_shutdown return 0 does not time out");
        result.expectEqual(0, status, "SSL_shutdown return 0 is not an error");
        result.expectEqual(1, static_cast<int>(tlsShutdownConstructedCount()), "return 0 constructs exactly one TLSShutdown");
        result.expectEqual(0, static_cast<int>(tlsShutdownActiveCount()), "return 0 leaves no active TLSShutdown");
        result.expectEqual(1, static_cast<int>(tlsShutdownMaxActiveCount()), "return 0 active TLSShutdown count never exceeds one");
    }

    {
        using namespace core::socket::stream::tls::detail::test;
        resetTlsShutdownCounters();
        SslHandle ssl;
        int success = 0;
        int status = 0;
        forceNextTlsShutdownResult({1, SSL_ERROR_NONE, 0, 0});
        core::socket::stream::tls::TLSShutdown::doShutdown(
            "tls-shutdown-return-1",
            ssl.ssl,
            [&success]() { success++; },
            []() {},
            [&status](int, int) { status++; },
            utils::Timeval{});
        result.expectEqual(1, success, "SSL_shutdown return 1 completes TLS shutdown synchronously");
        result.expectEqual(0, status, "SSL_shutdown return 1 is not an error");
        result.expectEqual(1, static_cast<int>(tlsShutdownConstructedCount()), "return 1 constructs exactly one TLSShutdown");
        result.expectEqual(0, static_cast<int>(tlsShutdownActiveCount()), "return 1 leaves no active TLSShutdown");
        result.expectEqual(1, static_cast<int>(tlsShutdownMaxActiveCount()), "return 1 active TLSShutdown count never exceeds one");
    }

    {
        using namespace core::socket::stream::tls::detail::test;
        resetTlsShutdownCounters();
        SslHandle ssl;
        int status = 0;
        int statusSsl = 0;
        int statusSystem = 0;
        forceNextTlsShutdownResult({-1, SSL_ERROR_WANT_READ, EBADF, 0});
        core::socket::stream::tls::TLSShutdown::doShutdown(
            "tls-shutdown-registration-failure",
            ssl.ssl,
            []() {},
            []() {},
            [&status, &statusSsl, &statusSystem](int sslErr, int systemErr) {
                status++;
                statusSsl = sslErr;
                statusSystem = systemErr;
            },
            utils::Timeval{});
        result.expectEqual(1, status, "TLSShutdown registration failure reports status exactly once");
        result.expectEqual(SSL_ERROR_WANT_READ, statusSsl, "TLSShutdown registration failure preserves SSL readiness error");
        result.expectEqual(EBADF, statusSystem, "TLSShutdown registration failure preserves registration errno");
        result.expectEqual(1, static_cast<int>(tlsShutdownConstructedCount()), "registration failure constructs exactly one TLSShutdown");
        result.expectEqual(0, static_cast<int>(tlsShutdownActiveCount()), "registration failure destroys TLSShutdown");
    }

    return result.processResult();
}
