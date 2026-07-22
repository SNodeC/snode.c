#include "core/DescriptorEventPublisher.h"
#include "core/EventLoop.h"
#include "core/EventMultiplexer.h"
#include "core/socket/SocketAddress.h"
#include "core/socket/stream/SocketContext.h"
#include "core/socket/stream/tls/SocketConnection.hpp"
#include "core/socket/stream/tls/detail/TLSLifecycleTestAccess.h"
#include "net/config/ConfigInstance.h"
#include "tests/support/TestResult.h"

#include <array>
#include <cerrno>
#include <fcntl.h>
#include <initializer_list>
#include <memory>
#include <openssl/err.h>
#include <openssl/rsa.h>
#include <openssl/ssl.h>
#include <openssl/x509.h>
#include <optional>
#include <string>
#include <sys/socket.h>
#include <unistd.h>
#include <utility>
#include <vector>

using core::socket::stream::tls::TLSHandshake;
using core::socket::stream::tls::TLSShutdown;
using core::socket::stream::tls::detail::TLSLifecycleTestAccess;
using tests::support::TestResult;

namespace {

    struct PipeFd {
        int fds[2] = {-1, -1};
        PipeFd() {
            socketpair(AF_UNIX, SOCK_STREAM, 0, fds);
        }
        ~PipeFd() {
            if (fds[0] >= 0)
                close(fds[0]);
            if (fds[1] >= 0)
                close(fds[1]);
        }
        int fd() const {
            return fds[0];
        }
        int writeFd() const {
            return fds[1];
        }
    };

    void releaseDisabledEvents() {
        const utils::Timeval now = utils::Timeval::currentTime();
        core::EventLoop::instance()
            .getEventMultiplexer()
            .getDescriptorEventPublisher(core::EventMultiplexer::DISP_TYPE::RD)
            .releaseDisabledEvents(now);
        core::EventLoop::instance()
            .getEventMultiplexer()
            .getDescriptorEventPublisher(core::EventMultiplexer::DISP_TYPE::WR)
            .releaseDisabledEvents(now);
    }

    class TestSocketAddress : public core::socket::SocketAddress {
    public:
        using SockAddr = sockaddr_storage;
        using SockLen = socklen_t;
        std::string toString(bool = true) const override {
            return "tls-transport-state-machine-test";
        }
    };

    struct TestPhysicalCounters {
        int shutdownWr = 0;
        int shutdownRd = 0;
        int shutdownRdWr = 0;
    };

    class TestPhysicalSocket {
    public:
        using SocketAddress = TestSocketAddress;
        enum class SHUT { RD, WR, RDWR };

        TestPhysicalSocket(int fd, const std::shared_ptr<TestPhysicalCounters>& counters)
            : fd(fd)
            , counters(counters) {
        }
        TestPhysicalSocket(TestPhysicalSocket&& other) noexcept
            : fd(other.fd)
            , counters(std::move(other.counters)) {
            other.fd = -1;
        }
        TestPhysicalSocket& operator=(TestPhysicalSocket&& other) noexcept {
            fd = other.fd;
            counters = std::move(other.counters);
            other.fd = -1;
            return *this;
        }

        int getFd() const {
            return fd;
        }
        int getSockName(sockaddr_storage&, socklen_t& len) {
            len = sizeof(sockaddr_storage);
            return 0;
        }
        int getPeerName(sockaddr_storage&, socklen_t& len) {
            len = sizeof(sockaddr_storage);
            return 0;
        }
        const TestSocketAddress& getBindAddress() const {
            return address;
        }
        int shutdown(SHUT how) {
            if (counters) {
                if (how == SHUT::WR)
                    counters->shutdownWr++;
                if (how == SHUT::RD)
                    counters->shutdownRd++;
                if (how == SHUT::RDWR)
                    counters->shutdownRdWr++;
            }
            return 0;
        }

    private:
        int fd = -1;
        std::shared_ptr<TestPhysicalCounters> counters;
        TestSocketAddress address;
    };

    struct TestLocalConfig {
        static TestSocketAddress getSocketAddress(const sockaddr_storage&, socklen_t) {
            return {};
        }
    };
    struct TestRemoteConfig {
        static TestSocketAddress getSocketAddress(const sockaddr_storage&, socklen_t) {
            return {};
        }
    };

    class TestConfig
        : public net::config::ConfigInstance
        , public TestLocalConfig
        , public TestRemoteConfig {
    public:
        using Local = TestLocalConfig;
        using Remote = TestRemoteConfig;
        explicit TestConfig(bool closeNotifyIsEof = true, std::size_t readBlockSize = 1024)
            : ConfigInstance(nextInstanceName(), Role::CLIENT)
            , closeNotifyIsEof(closeNotifyIsEof)
            , readBlockSize(readBlockSize) {
        }
        utils::Timeval getInitTimeout() const {
            return {1, 0};
        }
        utils::Timeval getShutdownTimeout() const {
            return {1, 0};
        }
        bool getNoCloseNotifyIsEOF() const {
            return !closeNotifyIsEof;
        }
        utils::Timeval getReadTimeout() const {
            return {1, 0};
        }
        utils::Timeval getWriteTimeout() const {
            return {1, 0};
        }
        utils::Timeval getTerminateTimeout() const {
            return {1, 0};
        }
        std::size_t getReadBlockSize() const {
            return readBlockSize;
        }
        std::size_t getWriteBlockSize() const {
            return 1024;
        }

    private:
        static std::string nextInstanceName() {
            static int nextId = 0;
            return "tls-transport-state-machine-" + std::to_string(++nextId);
        }
        bool closeNotifyIsEof = true;
        std::size_t readBlockSize = 1024;
    };

    using TestConnection = core::socket::stream::tls::SocketConnection<TestPhysicalSocket, TestConfig>;

    struct TestFixture {
        PipeFd pipeFd;
        SSL_CTX* ctx = SSL_CTX_new(TLS_method());
        std::shared_ptr<TestPhysicalCounters> counters = std::make_shared<TestPhysicalCounters>();
        std::shared_ptr<TestConfig> config;
        TestConnection* connection = nullptr;
        int disconnects = 0;
        int stateAtDisconnect = -1;
        bool writerBlockedAtDisconnect = false;
        bool sslAttachedAtDisconnect = true;
        bool lifecycleHasSslAtDisconnect = true;
        bool shutdownGuardAtDisconnect = true;
        int shutdownFailureErrnoAtDisconnect = 0;
        std::size_t handoffBufferSizeAtDisconnect = 0;
        std::string handoffBufferContentsAtDisconnect;
        bool writeOnDisconnect = false;
        int postCloseWriteAttempts = 0;

        explicit TestFixture(bool closeNotifyIsEof = true, std::size_t readBlockSize = 1024)
            : config(std::make_shared<TestConfig>(closeNotifyIsEof, readBlockSize)) {
            connection = new TestConnection(
                TestPhysicalSocket(pipeFd.fd(), counters),
                [this](TestConnection* disconnectedConnection) {
                    ++disconnects;
                    stateAtDisconnect = TLSLifecycleTestAccess::transportState(*disconnectedConnection);
                    writerBlockedAtDisconnect = TLSLifecycleTestAccess::writerActivationBlocked(*disconnectedConnection);
                    sslAttachedAtDisconnect = TLSLifecycleTestAccess::sslAttached(*disconnectedConnection);
                    lifecycleHasSslAtDisconnect = TLSLifecycleTestAccess::lifecycleHasSSL(*disconnectedConnection);
                    shutdownGuardAtDisconnect = TLSLifecycleTestAccess::shutdownGuardActive(*disconnectedConnection);
                    shutdownFailureErrnoAtDisconnect = TLSLifecycleTestAccess::shutdownFailureErrno(*disconnectedConnection);
                    handoffBufferSizeAtDisconnect = TLSLifecycleTestAccess::handoffBufferSize(*disconnectedConnection);
                    handoffBufferContentsAtDisconnect = TLSLifecycleTestAccess::handoffBufferContents(*disconnectedConnection);
                    if (writeOnDisconnect) {
                        ++postCloseWriteAttempts;
                        disconnectedConnection->sendToPeer("after-close", 11);
                    }
                    connection = nullptr;
                },
                1,
                config);
            TLSLifecycleTestAccess::startSSL(*connection, pipeFd.fd(), ctx);
        }
        ~TestFixture() {
            if (connection != nullptr) {
                connection->close();
                releaseDisabledEvents();
                if (connection != nullptr) {
                    delete connection;
                    connection = nullptr;
                }
            }
            SSL_CTX_free(ctx);
        }

        void destroyConnection() {
            if (connection != nullptr) {
                connection->close();
                releaseDisabledEvents();
                if (connection != nullptr) {
                    delete connection;
                    connection = nullptr;
                }
            }
        }
    };

    EVP_PKEY* makeKey() {
        EVP_PKEY* pkey = EVP_PKEY_new();
        RSA* rsa = RSA_new();
        BIGNUM* e = BN_new();
        BN_set_word(e, RSA_F4);
        RSA_generate_key_ex(rsa, 2048, e, nullptr);
        EVP_PKEY_assign_RSA(pkey, rsa);
        BN_free(e);
        return pkey;
    }

    X509* makeCert(EVP_PKEY* pkey) {
        X509* cert = X509_new();
        ASN1_INTEGER_set(X509_get_serialNumber(cert), 1);
        X509_gmtime_adj(X509_get_notBefore(cert), 0);
        X509_gmtime_adj(X509_get_notAfter(cert), 60 * 60);
        X509_set_pubkey(cert, pkey);
        X509_NAME* name = X509_get_subject_name(cert);
        X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC, reinterpret_cast<const unsigned char*>("snodec-test"), -1, -1, 0);
        X509_set_issuer_name(cert, name);
        X509_sign(cert, pkey, EVP_sha256());
        return cert;
    }

    struct TlsPair {
        SSL_CTX* clientCtx = SSL_CTX_new(TLS_method());
        SSL_CTX* serverCtx = SSL_CTX_new(TLS_method());
        SSL* client = nullptr;
        SSL* server = nullptr;
        BIO* serverToClient = nullptr;

        TlsPair() {
            SSL_CTX_set_verify(clientCtx, SSL_VERIFY_NONE, nullptr);
            EVP_PKEY* pkey = makeKey();
            X509* cert = makeCert(pkey);
            SSL_CTX_use_certificate(serverCtx, cert);
            SSL_CTX_use_PrivateKey(serverCtx, pkey);
            X509_free(cert);
            EVP_PKEY_free(pkey);
            client = SSL_new(clientCtx);
            server = SSL_new(serverCtx);
            SSL_set_connect_state(client);
            SSL_set_accept_state(server);
            BIO *clientRead = nullptr, *clientWrite = nullptr, *serverRead = nullptr, *serverWrite = nullptr;
            BIO_new_bio_pair(&clientRead, 0, &serverWrite, 0);
            BIO_new_bio_pair(&serverRead, 0, &clientWrite, 0);
            serverToClient = serverWrite;
            SSL_set_bio(client, clientRead, clientWrite);
            SSL_set_bio(server, serverRead, serverWrite);
            SSL_set_read_ahead(client, 0);
            SSL_set_read_ahead(server, 0);
        }

        ~TlsPair() {
            SSL_free(client);
            SSL_free(server);
            SSL_CTX_free(clientCtx);
            SSL_CTX_free(serverCtx);
        }

        bool handshake() {
            bool clientDone = false;
            bool serverDone = false;
            for (int i = 0; i < 1000 && (!clientDone || !serverDone); ++i) {
                if (!clientDone) {
                    const int ret = SSL_do_handshake(client);
                    if (ret == 1) {
                        clientDone = true;
                    } else if (SSL_get_error(client, ret) != SSL_ERROR_WANT_READ && SSL_get_error(client, ret) != SSL_ERROR_WANT_WRITE) {
                        return false;
                    }
                }
                if (!serverDone) {
                    const int ret = SSL_do_handshake(server);
                    if (ret == 1) {
                        serverDone = true;
                    } else if (SSL_get_error(server, ret) != SSL_ERROR_WANT_READ && SSL_get_error(server, ret) != SSL_ERROR_WANT_WRITE) {
                        return false;
                    }
                }
            }
            return clientDone && serverDone;
        }

        bool writeFromServer(const std::string& payload) {
            return SSL_write(server, payload.data(), static_cast<int>(payload.size())) == static_cast<int>(payload.size());
        }

        bool makeClientPending() {
            char c = 0;
            const int ret = SSL_peek(client, &c, 1);
            return ret == 1 && SSL_pending(client) > 0;
        }
    };

    struct SocketPeerTls {
        SSL_CTX* ctx = SSL_CTX_new(TLS_method());
        SSL* ssl = nullptr;

        explicit SocketPeerTls(int fd) {
            EVP_PKEY* pkey = makeKey();
            X509* cert = makeCert(pkey);
            SSL_CTX_use_certificate(ctx, cert);
            SSL_CTX_use_PrivateKey(ctx, pkey);
            X509_free(cert);
            EVP_PKEY_free(pkey);
            ssl = SSL_new(ctx);
            SSL_set_fd(ssl, fd);
            SSL_set_accept_state(ssl);
            SSL_set_read_ahead(ssl, 0);
        }

        void releaseSSL() {
            if (ssl != nullptr) {
                SSL_free(ssl);
                ssl = nullptr;
            }
        }

        ~SocketPeerTls() {
            releaseSSL();
            SSL_CTX_free(ctx);
        }
    };

    void makeNonBlocking(int fd) {
        const int flags = fcntl(fd, F_GETFL, 0);
        if (flags >= 0) {
            fcntl(fd, F_SETFL, flags | O_NONBLOCK);
        }
    }

    bool
    driveProductionHandshake(TestFixture& f, SSL* peerSsl, int& callbacks, std::vector<logger::LogRecord>* readinessRecords = nullptr) {
        SSL* connectionSsl = f.connection->getSSL();
        SSL_set_connect_state(connectionSsl);
        bool peerDone = false;
        bool connectionDone = false;
        if (!TLSLifecycleTestAccess::doSSLHandshake(
                *f.connection,
                [&] {
                    ++callbacks;
                    connectionDone = true;
                    if (readinessRecords != nullptr) {
                        f.connection
                            ->log([readinessRecords](logger::LogRecord record) {
                                readinessRecords->push_back(std::move(record));
                            })
                            .info("transport ready");
                    }
                },
                [] {
                },
                [](int) {
                })) {
            return false;
        }
        for (int i = 0; i < 1000 && (!peerDone || !connectionDone || TLSLifecycleTestAccess::handshakeGuardActive(*f.connection)); ++i) {
            if (!peerDone) {
                const int ret = SSL_do_handshake(peerSsl);
                if (ret == 1) {
                    peerDone = true;
                } else {
                    const int err = SSL_get_error(peerSsl, ret);
                    if (err != SSL_ERROR_WANT_READ && err != SSL_ERROR_WANT_WRITE) {
                        return false;
                    }
                }
            }
            TLSHandshake* helper = TLSLifecycleTestAccess::lastHandshake();
            if (helper != nullptr) {
                TLSLifecycleTestAccess::readEvent(helper);
                TLSLifecycleTestAccess::writeEvent(helper);
            }
            releaseDisabledEvents();
        }
        return peerDone && connectionDone && callbacks == 1 && !TLSLifecycleTestAccess::handshakeGuardActive(*f.connection);
    }

    bool sslWriteAll(SSL* ssl, const char* data, int size) {
        int written = 0;
        for (int i = 0; i < 1000 && written < size; ++i) {
            const int ret = SSL_write(ssl, data + written, size - written);
            if (ret > 0) {
                written += ret;
                continue;
            }
            const int err = SSL_get_error(ssl, ret);
            if (err != SSL_ERROR_WANT_READ && err != SSL_ERROR_WANT_WRITE) {
                return false;
            }
        }
        return written == size;
    }

    bool drivePeerCloseNotifySent(SSL* peerSsl) {
        for (int i = 0; i < 1000; ++i) {
            const int ret = SSL_shutdown(peerSsl);
            if (ret == 1 || ret == 0 || (SSL_get_shutdown(peerSsl) & SSL_SENT_SHUTDOWN) != 0) {
                return (SSL_get_shutdown(peerSsl) & SSL_SENT_SHUTDOWN) != 0;
            }
            const int err = SSL_get_error(peerSsl, ret);
            if (err != SSL_ERROR_WANT_READ && err != SSL_ERROR_WANT_WRITE) {
                return false;
            }
        }
        return false;
    }

    bool drivePeerFullShutdown(SSL* peerSsl) {
        for (int i = 0; i < 1000; ++i) {
            ERR_clear_error();
            errno = 0;

            const int ret = SSL_shutdown(peerSsl);
            if (ret == 1) {
                return true;
            }
            if (ret == 0) {
                continue;
            }

            const int err = SSL_get_error(peerSsl, ret);
            if (err != SSL_ERROR_WANT_READ && err != SSL_ERROR_WANT_WRITE) {
                return false;
            }
        }
        return false;
    }

    bool driveProductionShutdownToQuiescence(TestFixture& f) {
        TLSLifecycleTestAccess::onReadShutdown(*f.connection);
        for (int i = 0; i < 1000 && f.connection != nullptr && TLSLifecycleTestAccess::shutdownGuardActive(*f.connection); ++i) {
            TLSShutdown* helper = TLSLifecycleTestAccess::lastShutdown();
            if (helper != nullptr) {
                TLSLifecycleTestAccess::readEvent(helper);
                TLSLifecycleTestAccess::writeEvent(helper);
            }
            releaseDisabledEvents();
        }
        releaseDisabledEvents();
        return f.connection == nullptr || !TLSLifecycleTestAccess::shutdownGuardActive(*f.connection);
    }

    struct CleanEofState {
        int cleanEof = 0;
        int readErrors = 0;
        int disconnected = 0;
        bool closeOnEof = false;
        bool shutdownWriteOnEof = false;
    };

    class CleanEofContext : public core::socket::stream::SocketContext {
    public:
        CleanEofContext(TestConnection* connection, CleanEofState& state)
            : SocketContext(connection)
            , state(state) {
        }
        ~CleanEofContext() override = default;

    private:
        void onConnected() override {
        }
        void onDisconnected() override {
            state.disconnected++;
        }
        std::size_t onReceivedFromPeer() override {
            return 0;
        }
        bool onSignal(int) override {
            return false;
        }
        void onWriteError(int) override {
        }
        void onReadError(int errnum) override {
            if (errnum == 0) {
                ++state.cleanEof;
                if (state.closeOnEof) {
                    close();
                } else if (state.shutdownWriteOnEof) {
                    shutdownWrite();
                }
            } else {
                ++state.readErrors;
            }
        }

        CleanEofState& state;
    };

    void makeTlsActive(TestFixture& f) {
        TLSLifecycleTestAccess::transitionTo(*f.connection, 2);
        TLSLifecycleTestAccess::transitionTo(*f.connection, 3);
    }

    void resetTlsTestState() {
        TLSLifecycleTestAccess::resetHandshake();
        TLSLifecycleTestAccess::resetShutdown();
        TLSLifecycleTestAccess::resetReader();
        TLSLifecycleTestAccess::resetWriter();
    }

    void handshakeLifetime(TestResult& result) {
        resetTlsTestState();
        {
            TestFixture f;
            TLSLifecycleTestAccess::enqueueHandshakeResult(-1, SSL_ERROR_WANT_READ);
            int success = 0, timeout = 0, status = 0;
            result.expectTrue(TLSLifecycleTestAccess::doSSLHandshake(
                                  *f.connection,
                                  [&] {
                                      success++;
                                  },
                                  [&] {
                                      timeout++;
                                  },
                                  [&](int) {
                                      status++;
                                  }),
                              "handshake starts");
            result.expectTrue(TLSLifecycleTestAccess::handshakeGuardActive(*f.connection), "handshake guard active while waiting");
            TLSHandshake* helper = TLSLifecycleTestAccess::lastHandshake();
            result.expectTrue(helper != nullptr, "handshake helper exists before connection destruction");
            f.destroyConnection();
            const bool helperRetained = helper != nullptr && TLSLifecycleTestAccess::lastHandshake() == helper &&
                                        TLSLifecycleTestAccess::handshakeCounters().active == 1;
            result.expectTrue(helperRetained, "connection destruction retains the observed handshake helper");
            TLSLifecycleTestAccess::enqueueHandshakeResult(1, SSL_ERROR_NONE);
            if (helperRetained) {
                TLSLifecycleTestAccess::readEvent(helper);
            }
            result.expectEqual(0, success, "destroyed connection suppresses success callback");
            result.expectEqual(0, timeout, "destroyed connection suppresses timeout callback");
            result.expectEqual(0, status, "destroyed connection suppresses status callback");
            result.expectEqual(
                1, TLSLifecycleTestAccess::handshakeCounters().active, "completed stale handshake remains alive until publisher cleanup");
            releaseDisabledEvents();
            result.expectTrue(TLSLifecycleTestAccess::lastHandshake() == nullptr, "publisher cleanup releases the stale handshake helper");
            result.expectEqual(
                0, TLSLifecycleTestAccess::handshakeCounters().active, "publisher cleanup destroys the stale handshake helper");
        }

        resetTlsTestState();
        {
            TestFixture f;
            TLSLifecycleTestAccess::enqueueHandshakeResult(-1, SSL_ERROR_WANT_WRITE);
            result.expectTrue(TLSLifecycleTestAccess::doSSLHandshake(
                                  *f.connection,
                                  [] {
                                  },
                                  [] {
                                  },
                                  [](int) {
                                  }),
                              "handshake write-wait starts");
            TLSHandshake* helper = TLSLifecycleTestAccess::lastHandshake();
            TLSLifecycleTestAccess::stopSSL(*f.connection);
            result.expectTrue(TLSLifecycleTestAccess::pendingStopSSL(*f.connection), "stopSSL marks release during handshake");
            TLSLifecycleTestAccess::enqueueHandshakeResult(1, SSL_ERROR_NONE);
            TLSLifecycleTestAccess::writeEvent(helper);
            releaseDisabledEvents();
            result.expectTrue(!TLSLifecycleTestAccess::lifecycleHasSSL(*f.connection),
                              "stopSSL releases lifecycle SSL after helper release");
        }
    }

    void shutdownFinalization(TestResult& result) {
        resetTlsTestState();
        {
            TestFixture f(true);
            makeTlsActive(f);
            TLSLifecycleTestAccess::enqueueShutdownResult(1, SSL_ERROR_NONE);
            TLSLifecycleTestAccess::onReadShutdown(*f.connection);
            result.expectEqual(1, f.counters->shutdownWr, "peer EOF close performs one SHUT_WR");
            result.expectEqual(0, TLSLifecycleTestAccess::shutdownCounters().active, "peer EOF shutdown helper released");
        }

        resetTlsTestState();
        {
            TestFixture f(false);
            makeTlsActive(f);
            TLSLifecycleTestAccess::enqueueShutdownResult(1, SSL_ERROR_NONE);
            TLSLifecycleTestAccess::onReadShutdown(*f.connection);
            result.expectEqual(0, f.counters->shutdownWr, "plaintext continuation performs zero SHUT_WR");
            result.expectTrue(!TLSLifecycleTestAccess::sslAttached(*f.connection), "plaintext continuation detaches SSL");
        }

        resetTlsTestState();
        {
            TestFixture f(true);
            makeTlsActive(f);
            TLSLifecycleTestAccess::enqueueShutdownResult(1, SSL_ERROR_NONE);
            f.connection->shutdownWrite();
            f.connection->shutdownWrite();
            result.expectEqual(1, f.counters->shutdownWr, "repeated local shutdown performs one SHUT_WR");
        }

        resetTlsTestState();
        {
            TestFixture f(true);
            makeTlsActive(f);
            int callbackCount = 0;
            TLSLifecycleTestAccess::enqueueShutdownResult(1, SSL_ERROR_NONE);
            TLSLifecycleTestAccess::doWriteShutdown(*f.connection, [&] {
                ++callbackCount;
                f.connection->shutdownWrite();
            });
            result.expectEqual(1, f.counters->shutdownWr, "completion callback shutdownWrite reentry keeps one SHUT_WR");
            result.expectEqual(1, callbackCount, "completion callback with shutdownWrite reentry runs once");
            result.expectEqual(1, TLSLifecycleTestAccess::shutdownCounters().constructed, "completion reentry uses one shutdown helper");
            result.expectEqual(1, TLSLifecycleTestAccess::shutdownCounters().releases, "completion reentry releases one shutdown helper");
            result.expectEqual(1, TLSLifecycleTestAccess::shutdownCounters().destroyed, "completion reentry destroys one shutdown helper");
            result.expectEqual(0, TLSLifecycleTestAccess::shutdownCounters().active, "completion reentry leaves no active shutdown helper");
            result.expectEqual(8, TLSLifecycleTestAccess::transportState(*f.connection), "completion reentry ends Closed");
        }

        resetTlsTestState();
        {
            TestFixture f(true);
            makeTlsActive(f);
            int callbackCount = 0;
            TLSLifecycleTestAccess::enqueueShutdownResult(1, SSL_ERROR_NONE);
            TLSLifecycleTestAccess::doWriteShutdown(*f.connection, [&] {
                ++callbackCount;
                f.connection->close();
            });
            result.expectEqual(1, f.counters->shutdownWr, "completion callback close reentry keeps one SHUT_WR");
            result.expectEqual(1, callbackCount, "completion callback with close reentry runs once");
            result.expectEqual(1, TLSLifecycleTestAccess::shutdownCounters().constructed, "close reentry uses one shutdown helper");
            result.expectEqual(1, TLSLifecycleTestAccess::shutdownCounters().releases, "close reentry releases one shutdown helper");
            result.expectEqual(1, TLSLifecycleTestAccess::shutdownCounters().destroyed, "close reentry destroys one shutdown helper");
            result.expectEqual(0, TLSLifecycleTestAccess::shutdownCounters().active, "close reentry leaves no active shutdown helper");
            result.expectEqual(8, TLSLifecycleTestAccess::transportState(*f.connection), "close reentry ends Closed");
        }
    }

    void cleanEofReentrancy(TestResult& result) {
        resetTlsTestState();
        {
            TestFixture f(true);
            makeTlsActive(f);
            CleanEofState state;
            state.closeOnEof = true;
            auto* context = new CleanEofContext(f.connection, state);
            f.connection->setSocketContext(context);
            TLSLifecycleTestAccess::enqueueShutdownResult(1, SSL_ERROR_NONE);
            TLSLifecycleTestAccess::onReadShutdown(*f.connection);
            releaseDisabledEvents();
            result.expectEqual(1, state.cleanEof, "clean EOF callback reaches context and reenters close once");
            result.expectEqual(1, f.counters->shutdownWr, "clean EOF close reentry keeps one SHUT_WR");
            result.expectEqual(
                1, TLSLifecycleTestAccess::shutdownCounters().constructed, "clean EOF close reentry uses one shutdown helper");
            result.expectEqual(
                1, TLSLifecycleTestAccess::shutdownCounters().releases, "clean EOF close reentry releases one shutdown helper");
            result.expectEqual(
                1, TLSLifecycleTestAccess::shutdownCounters().destroyed, "clean EOF close reentry destroys one shutdown helper");
            result.expectEqual(
                0, TLSLifecycleTestAccess::shutdownCounters().active, "clean EOF close reentry leaves no active shutdown helper");
            result.expectEqual(1, f.disconnects, "clean EOF close reentry disconnects once");
            result.expectTrue(f.connection == nullptr || TLSLifecycleTestAccess::transportState(*f.connection) == 8,
                              "clean EOF close reentry ends Closed");
        }

        resetTlsTestState();
        {
            TestFixture f(true);
            makeTlsActive(f);
            CleanEofState state;
            state.shutdownWriteOnEof = true;
            auto* context = new CleanEofContext(f.connection, state);
            f.connection->setSocketContext(context);
            TLSLifecycleTestAccess::enqueueShutdownResult(1, SSL_ERROR_NONE);
            TLSLifecycleTestAccess::onReadShutdown(*f.connection);
            releaseDisabledEvents();
            result.expectEqual(1, state.cleanEof, "clean EOF callback reaches context and reenters shutdownWrite once");
            result.expectEqual(1, f.counters->shutdownWr, "clean EOF shutdownWrite reentry keeps one SHUT_WR");
            result.expectEqual(
                1, TLSLifecycleTestAccess::shutdownCounters().constructed, "clean EOF shutdownWrite reentry uses one shutdown helper");
            result.expectEqual(
                1, TLSLifecycleTestAccess::shutdownCounters().releases, "clean EOF shutdownWrite reentry releases one shutdown helper");
            result.expectEqual(
                1, TLSLifecycleTestAccess::shutdownCounters().destroyed, "clean EOF shutdownWrite reentry destroys one shutdown helper");
            result.expectEqual(
                0, TLSLifecycleTestAccess::shutdownCounters().active, "clean EOF shutdownWrite reentry leaves no active shutdown helper");
            result.expectEqual(1, f.disconnects, "clean EOF shutdownWrite reentry disconnects once");
            result.expectTrue(f.connection == nullptr || TLSLifecycleTestAccess::transportState(*f.connection) == 8,
                              "clean EOF shutdownWrite reentry ends Closed");
        }
    }

    void shutdownFailureCallbacks(TestResult& result) {
        resetTlsTestState();
        {
            TestFixture f(true);
            makeTlsActive(f);
            int callbackCount = 0;
            TLSLifecycleTestAccess::enqueueShutdownResult(-1, SSL_ERROR_SYSCALL, EPIPE);
            TLSLifecycleTestAccess::doWriteShutdown(*f.connection, [&] {
                callbackCount++;
            });
            result.expectEqual(1, f.counters->shutdownWr, "shutdown syscall error fallback performs one SHUT_WR");
            result.expectEqual(1, callbackCount, "shutdown syscall error drains retained callback");
        }
    }

    void shutdownTimeoutAndFailureMatrix(TestResult& result) {
        resetTlsTestState();
        {
            TestFixture f(true);
            makeTlsActive(f);
            int callbackCount = 0;
            TLSLifecycleTestAccess::enqueueShutdownResult(-1, SSL_ERROR_WANT_READ);
            TLSLifecycleTestAccess::doWriteShutdown(*f.connection, [&] {
                ++callbackCount;
            });
            TLSShutdown* helper = TLSLifecycleTestAccess::lastShutdown();
            result.expectEqual(
                1, TLSLifecycleTestAccess::shutdownCounters().constructed, "timeout finalization constructs one TLSShutdown helper");
            result.expectEqual(
                1, TLSLifecycleTestAccess::shutdownCounters().maxConcurrent, "timeout finalization max active shutdown helper is one");
            result.expectTrue(TLSLifecycleTestAccess::shutdownGuardActive(*f.connection),
                              "timeout finalization guard active while helper waits");
            result.expectTrue(TLSLifecycleTestAccess::lifecycleHasSSL(*f.connection),
                              "timeout finalization retains SSL until helper release");
            TLSLifecycleTestAccess::readTimeout(helper);
            result.expectEqual(1, TLSLifecycleTestAccess::shutdownCounters().timeouts, "timeout finalization records one semantic timeout");
            result.expectTrue(TLSLifecycleTestAccess::shutdownGuardActive(*f.connection),
                              "timeout finalization guard remains active before release");
            releaseDisabledEvents();
            result.expectEqual(1, TLSLifecycleTestAccess::shutdownCounters().releases, "timeout finalization releases helper once");
            result.expectEqual(1, TLSLifecycleTestAccess::shutdownCounters().destroyed, "timeout finalization destroys helper once");
            result.expectEqual(
                0, TLSLifecycleTestAccess::shutdownCounters().active, "timeout finalization returns active helper count to zero");
            result.expectEqual(1, f.counters->shutdownWr, "timeout finalization performs one physical SHUT_WR");
            result.expectEqual(1, callbackCount, "timeout finalization drains completion callback once");
            result.expectEqual(1, f.disconnects, "timeout finalization closes once");
            result.expectEqual(8, f.stateAtDisconnect, "timeout finalization terminal state Closed");
            result.expectTrue(f.writerBlockedAtDisconnect, "timeout finalization leaves writer blocked");
            result.expectEqual(0, TLSLifecycleTestAccess::readerCounters().operationCalls, "timeout finalization performs no raw read");
            result.expectEqual(0, TLSLifecycleTestAccess::writerCounters().operationCalls, "timeout finalization performs no raw write");
        }

        struct FailureCase {
            const char* name;
            int ret;
            int sslError;
            int systemError;
            unsigned long openSslError;
            int expectedErrno;
            bool failRegistration;
            bool afterCloseNotify;
        };
        const FailureCase cases[] = {
            {"EPIPE", -1, SSL_ERROR_SYSCALL, EPIPE, 0, EPIPE, false, false},
            {"ECONNRESET", -1, SSL_ERROR_SYSCALL, ECONNRESET, 0, ECONNRESET, false, false},
            {"SSL_ERROR_SSL", -1, SSL_ERROR_SSL, 0, ERR_PACK(ERR_LIB_SSL, 0, SSL_R_BAD_RECORD_TYPE), EPROTO, false, false},
            {"unknown error", -1, 12345, 0, 0, EPROTO, false, false},
            {"registration failure", -1, SSL_ERROR_WANT_READ, 0, 0, EIO, true, false},
            {"registration failure after CloseNotifySent", 0, SSL_ERROR_NONE, 0, 0, EIO, true, true},
        };
        for (const auto& testCase : cases) {
            resetTlsTestState();
            TestFixture f(true);
            makeTlsActive(f);
            int callbackCount = 0;
            if (testCase.afterCloseNotify) {
                TLSLifecycleTestAccess::enqueueShutdownResult(0, SSL_ERROR_NONE);
                TLSLifecycleTestAccess::failNextShutdownReadEnable(testCase.expectedErrno);
            } else if (testCase.failRegistration) {
                TLSLifecycleTestAccess::enqueueShutdownResult(testCase.ret, testCase.sslError, testCase.systemError, testCase.openSslError);
                TLSLifecycleTestAccess::failNextShutdownReadEnable(testCase.expectedErrno);
            } else {
                TLSLifecycleTestAccess::enqueueShutdownResult(testCase.ret, testCase.sslError, testCase.systemError, testCase.openSslError);
            }
            TLSLifecycleTestAccess::doWriteShutdown(*f.connection, [&] {
                ++callbackCount;
            });
            releaseDisabledEvents();
            result.expectEqual(
                1, TLSLifecycleTestAccess::shutdownCounters().constructed, std::string(testCase.name) + ": one helper constructed");
            result.expectEqual(1, TLSLifecycleTestAccess::shutdownCounters().errors, std::string(testCase.name) + ": one semantic failure");
            result.expectEqual(1, TLSLifecycleTestAccess::shutdownCounters().releases, std::string(testCase.name) + ": one release");
            result.expectEqual(1, TLSLifecycleTestAccess::shutdownCounters().destroyed, std::string(testCase.name) + ": one destruction");
            result.expectEqual(
                0, TLSLifecycleTestAccess::shutdownCounters().active, std::string(testCase.name) + ": zero active helpers afterward");
            result.expectEqual(1, f.counters->shutdownWr, std::string(testCase.name) + ": one physical SHUT_WR");
            result.expectEqual(8, f.stateAtDisconnect, std::string(testCase.name) + ": terminal Closed state");
            result.expectEqual(testCase.expectedErrno,
                               TLSLifecycleTestAccess::shutdownCounters().lastErrno,
                               std::string(testCase.name) + ": correct errno preserved");
            result.expectEqual(1, callbackCount, std::string(testCase.name) + ": callback executes exactly once");
            result.expectEqual(1, f.disconnects, std::string(testCase.name) + ": no duplicate close");
        }
    }

    void shutdownCoalescingAndHandshakeBoundary(TestResult& result) {
        resetTlsTestState();
        {
            TestFixture f(true);
            makeTlsActive(f);
            std::string order;
            TLSLifecycleTestAccess::enqueueShutdownResult(-1, SSL_ERROR_WANT_READ);
            TLSLifecycleTestAccess::onReadShutdown(*f.connection);
            TLSLifecycleTestAccess::doWriteShutdown(*f.connection, [&] {
                order += 'A';
            });
            TLSLifecycleTestAccess::doWriteShutdown(*f.connection, [&] {
                order += 'B';
            });
            TLSLifecycleTestAccess::doSSLShutdown(*f.connection);
            TLSLifecycleTestAccess::doWriteShutdown(*f.connection, [&] {
                order += 'C';
            });
            result.expectEqual(1, TLSLifecycleTestAccess::shutdownCounters().constructed, "callback coalescing uses one helper");
            result.expectEqual(1, TLSLifecycleTestAccess::shutdownIntent(*f.connection), "callback coalescing escalates to CloseTransport");
            TLSShutdown* helper = TLSLifecycleTestAccess::lastShutdown();
            TLSLifecycleTestAccess::enqueueShutdownResult(1, SSL_ERROR_NONE);
            TLSLifecycleTestAccess::readEvent(helper);
            releaseDisabledEvents();
            result.expectTrue(order == "ABC", "callback coalescing preserves callback order A, B, C");
            result.expectEqual(1, f.counters->shutdownWr, "callback coalescing performs one SHUT_WR");
            result.expectEqual(8, f.stateAtDisconnect, "callback coalescing terminal Closed");
        }

        resetTlsTestState();
        {
            TestFixture f(true);
            TLSLifecycleTestAccess::enqueueHandshakeResult(-1, SSL_ERROR_WANT_READ);
            TLSLifecycleTestAccess::doSSLHandshake(
                *f.connection,
                [] {
                },
                [] {
                },
                [](int) {
                });
            TLSHandshake* handshake = TLSLifecycleTestAccess::lastHandshake();
            TLSLifecycleTestAccess::doSSLShutdown(*f.connection);
            result.expectTrue(TLSLifecycleTestAccess::pendingShutdownAfterHandshake(*f.connection),
                              "pending shutdown after handshake is recorded");
            result.expectEqual(0,
                               TLSLifecycleTestAccess::shutdownCounters().constructed,
                               "shutdown helper does not start while handshake helper is active");
            TLSLifecycleTestAccess::enqueueHandshakeResult(1, SSL_ERROR_NONE);
            TLSLifecycleTestAccess::readEvent(handshake);
            result.expectEqual(
                0, TLSLifecycleTestAccess::shutdownCounters().constructed, "shutdown helper still waits until handshake helper release");
            releaseDisabledEvents();
            result.expectEqual(
                1, TLSLifecycleTestAccess::handshakeCounters().constructed, "pending shutdown boundary used one handshake helper");
            result.expectEqual(
                1, TLSLifecycleTestAccess::shutdownCounters().constructed, "pending shutdown starts one shutdown helper after release");
            result.expectEqual(1, TLSLifecycleTestAccess::handshakeCounters().maxConcurrent, "handshake helper active count bounded");
            result.expectEqual(1, TLSLifecycleTestAccess::shutdownCounters().maxConcurrent, "shutdown helper active count bounded");
            result.expectTrue(f.connection == nullptr || TLSLifecycleTestAccess::writerActivationBlocked(*f.connection),
                              "writer blocked across handshake/shutdown boundary");
        }

        resetTlsTestState();
        {
            TestFixture f(true);
            int successCallbacks = 0;
            std::vector<logger::LogRecord> readinessRecords;
            TLSLifecycleTestAccess::enqueueHandshakeResult(-1, SSL_ERROR_WANT_READ);
            TLSLifecycleTestAccess::doSSLHandshake(
                *f.connection,
                [&] {
                    ++successCallbacks;
                    f.connection
                        ->log([&readinessRecords](logger::LogRecord record) {
                            readinessRecords.push_back(std::move(record));
                        })
                        .info("transport ready");
                },
                [] {
                },
                [](int) {
                });
            TLSHandshake* handshake = TLSLifecycleTestAccess::lastHandshake();
            TLSLifecycleTestAccess::doSSLShutdown(*f.connection);
            TLSLifecycleTestAccess::enqueueHandshakeResult(-1, SSL_ERROR_SSL, 0, ERR_PACK(ERR_LIB_SSL, 0, SSL_R_BAD_RECORD_TYPE));
            TLSLifecycleTestAccess::readEvent(handshake);
            releaseDisabledEvents();
            result.expectEqual(0,
                               TLSLifecycleTestAccess::shutdownCounters().constructed,
                               "handshake failure with pending shutdown starts no shutdown helper");
            result.expectEqual(0, successCallbacks, "failed TLS handshake has no readiness callback");
            result.expectTrue(readinessRecords.empty(), "failed TLS handshake emits no transport ready record");
            result.expectEqual(1, f.disconnects, "handshake failure with pending shutdown closes once");
        }
    }

    void handshakeCallbackReentrancy(TestResult& result) {
        resetTlsTestState();
        {
            TestFixture f(true);
            TLSLifecycleTestAccess::enqueueHandshakeResult(1, SSL_ERROR_NONE);
            int callbacks = 0;
            TLSLifecycleTestAccess::doSSLHandshake(
                *f.connection,
                [&] {
                    ++callbacks;
                    f.connection->close();
                },
                [] {
                },
                [](int) {
                });
            releaseDisabledEvents();
            result.expectEqual(1, callbacks, "handshake-success close reentrancy callback once");
            result.expectEqual(
                1, TLSLifecycleTestAccess::handshakeCounters().constructed, "handshake-success close reentrancy no duplicate helper");
            result.expectEqual(1, f.disconnects, "handshake-success close reentrancy no duplicate close");
        }
        resetTlsTestState();
        {
            TestFixture f(true);
            TLSLifecycleTestAccess::enqueueHandshakeResult(1, SSL_ERROR_NONE);
            int callbacks = 0;
            TLSLifecycleTestAccess::doSSLHandshake(
                *f.connection,
                [&] {
                    ++callbacks;
                    f.connection->shutdownWrite();
                },
                [] {
                },
                [](int) {
                });
            releaseDisabledEvents();
            result.expectEqual(1, callbacks, "handshake-success shutdownWrite reentrancy callback once");
            result.expectTrue(TLSLifecycleTestAccess::shutdownCounters().constructed <= 1,
                              "handshake-success shutdownWrite reentrancy no duplicate shutdown helper");
            result.expectTrue(f.counters->shutdownWr <= 1, "handshake-success shutdownWrite reentrancy no duplicate SHUT_WR");
        }
        resetTlsTestState();
        {
            TestFixture f(true);
            TLSLifecycleTestAccess::enqueueHandshakeResult(-1, SSL_ERROR_WANT_READ);
            TLSLifecycleTestAccess::doSSLHandshake(
                *f.connection,
                [] {
                },
                [&] {
                    f.connection->close();
                },
                [](int) {
                });
            TLSHandshake* helper = TLSLifecycleTestAccess::lastHandshake();
            TLSLifecycleTestAccess::readTimeout(helper);
            releaseDisabledEvents();
            result.expectEqual(1, TLSLifecycleTestAccess::handshakeCounters().timeouts, "handshake-timeout close reentrancy callback once");
            result.expectEqual(1, f.disconnects, "handshake-timeout close reentrancy no duplicate close");
        }
        resetTlsTestState();
        {
            TestFixture f(true);
            TLSLifecycleTestAccess::enqueueHandshakeResult(-1, SSL_ERROR_WANT_READ);
            TLSLifecycleTestAccess::doSSLHandshake(
                *f.connection,
                [] {
                },
                [] {
                },
                [&](int) {
                    f.connection->close();
                });
            TLSHandshake* helper = TLSLifecycleTestAccess::lastHandshake();
            TLSLifecycleTestAccess::enqueueHandshakeResult(-1, SSL_ERROR_SSL, 0, ERR_PACK(ERR_LIB_SSL, 0, SSL_R_BAD_RECORD_TYPE));
            TLSLifecycleTestAccess::readEvent(helper);
            releaseDisabledEvents();
            result.expectEqual(1, TLSLifecycleTestAccess::handshakeCounters().errors, "handshake-error close reentrancy callback once");
            result.expectEqual(1, f.disconnects, "handshake-error close reentrancy no duplicate close");
        }
    }

    void staleWriteGate(TestResult& result) {
        resetTlsTestState();
        {
            TestFixture f;
            const std::string payload = "queued";
            f.connection->sendToPeer(payload.data(), payload.size());
            result.expectEqual(static_cast<int>(payload.size()),
                               static_cast<int>(TLSLifecycleTestAccess::queuedWriteBytes(*f.connection)),
                               "write queued before handshake");
            TLSLifecycleTestAccess::enqueueHandshakeResult(-1, SSL_ERROR_WANT_READ);
            TLSLifecycleTestAccess::doSSLHandshake(
                *f.connection,
                [] {
                },
                [] {
                },
                [](int) {
                });
            TLSLifecycleTestAccess::triggerWriteEvent(*f.connection);
            result.expectEqual(
                0, TLSLifecycleTestAccess::writerCounters().operationCalls, "stale write during handshake performs no TLS write");
            result.expectEqual(static_cast<int>(payload.size()),
                               static_cast<int>(TLSLifecycleTestAccess::queuedWriteBytes(*f.connection)),
                               "blocked handshake write leaves queue intact");
            TLSHandshake* helper = TLSLifecycleTestAccess::lastHandshake();
            result.expectTrue(helper != nullptr, "stale-write gate retains its handshake helper");
            TLSLifecycleTestAccess::enqueueHandshakeResult(1, SSL_ERROR_NONE);
            if (helper != nullptr) {
                TLSLifecycleTestAccess::readEvent(helper);
            }
            releaseDisabledEvents();
            result.expectEqual(0, TLSLifecycleTestAccess::handshakeCounters().active, "stale-write gate releases its handshake helper");
        }

        resetTlsTestState();
        {
            TestFixture f(false);
            makeTlsActive(f);
            const std::string payload = "plain-after-tls";
            TLSLifecycleTestAccess::enqueueShutdownResult(-1, SSL_ERROR_WANT_READ);
            TLSLifecycleTestAccess::onReadShutdown(*f.connection);
            f.connection->sendToPeer(payload.data(), payload.size());
            TLSLifecycleTestAccess::triggerWriteEvent(*f.connection);
            result.expectEqual(
                0, TLSLifecycleTestAccess::writerCounters().operationCalls, "stale write during shutdown performs no TLS write");
            TLSShutdown* helper = TLSLifecycleTestAccess::lastShutdown();
            TLSLifecycleTestAccess::enqueueShutdownResult(1, SSL_ERROR_NONE);
            TLSLifecycleTestAccess::readEvent(helper);
        }
    }

    void writerOrderingFinalProof(TestResult& result) {
        resetTlsTestState();
        {
            TestFixture f;
            TlsPair pair;
            result.expectTrue(pair.handshake(), "TLS writer ordering: memory BIO peer handshake completes");
            TLSLifecycleTestAccess::replaceSSL(*f.connection, pair.client);
            pair.client = nullptr;
            f.connection->sendToPeer("one", 3);
            f.connection->sendToPeer("-", 1);
            f.connection->sendToPeer("two", 3);
            result.expectTrue(TLSLifecycleTestAccess::transitionTo(*f.connection, 2),
                              "TLS writer ordering: enters Handshaking with queued output");
            TLSLifecycleTestAccess::triggerWriteEvent(*f.connection);
            result.expectEqual(0,
                               TLSLifecycleTestAccess::writerCounters().operationCalls,
                               "TLS writer ordering: stale events before handshake completion produce no SSL_write");
            result.expectEqual(7,
                               static_cast<int>(TLSLifecycleTestAccess::queuedWriteBytes(*f.connection)),
                               "TLS writer ordering: queued bytes remain ordered as one-two before TlsActive");
            char peerBefore[16] = {};
            result.expectTrue(SSL_read(pair.server, peerBefore, sizeof(peerBefore)) <= 0,
                              "TLS writer ordering: peer receives no bytes before handshake completion");
            char rawBefore[16] = {};
            result.expectTrue(::recv(f.pipeFd.writeFd(), rawBefore, sizeof(rawBefore), MSG_DONTWAIT) < 0,
                              "TLS writer ordering: raw peer receives no bytes before handshake completion");
            result.expectTrue(TLSLifecycleTestAccess::transitionTo(*f.connection, 3),
                              "TLS writer ordering: simulated handshake completion enters TlsActive");
            result.expectEqual(3,
                               TLSLifecycleTestAccess::transportState(*f.connection),
                               "TLS writer ordering: reaches TlsActive before output is allowed");
            TLSLifecycleTestAccess::triggerWriteEvent(*f.connection);
            char peerOut[32] = {};
            const int peerBytes = SSL_read(pair.server, peerOut, sizeof(peerOut));
            result.expectTrue(peerBytes == 7 && std::string(peerOut, peerOut + peerBytes) == "one-two",
                              "TLS writer ordering: TLS peer receives exactly one-two");
            result.expectEqual(1,
                               TLSLifecycleTestAccess::writerCounters().operationCalls,
                               "TLS writer ordering: queued one-two leaves through SSL_write operation");
            result.expectEqual(
                0, static_cast<int>(TLSLifecycleTestAccess::queuedWriteBytes(*f.connection)), "TLS writer ordering: queue empty afterward");
            char rawAfter[16] = {};
            result.expectTrue(::recv(f.pipeFd.writeFd(), rawAfter, sizeof(rawAfter), MSG_DONTWAIT) < 0,
                              "TLS writer ordering: zero raw bytes after TLS write");
        }

        resetTlsTestState();
        {
            TestFixture f(false);
            makeTlsActive(f);
            TLSLifecycleTestAccess::enqueueShutdownResult(-1, SSL_ERROR_WANT_READ);
            TLSLifecycleTestAccess::onReadShutdown(*f.connection);
            f.connection->sendToPeer("plain", 5);
            f.connection->sendToPeer("-", 1);
            f.connection->sendToPeer("after", 5);
            TLSLifecycleTestAccess::triggerWriteEvent(*f.connection);
            result.expectEqual(0,
                               TLSLifecycleTestAccess::writerCounters().operationCalls,
                               "plaintext writer ordering: no SSL_write after shutdown ownership starts");
            result.expectEqual(11,
                               static_cast<int>(TLSLifecycleTestAccess::queuedWriteBytes(*f.connection)),
                               "plaintext writer ordering: queue remains intact during shutdown");
            char early[16] = {};
            result.expectTrue(::recv(f.pipeFd.writeFd(), early, sizeof(early), MSG_DONTWAIT) < 0,
                              "plaintext writer ordering: no raw byte before SSL release");
            TLSShutdown* helper = TLSLifecycleTestAccess::lastShutdown();
            TLSLifecycleTestAccess::enqueueShutdownResult(1, SSL_ERROR_NONE);
            TLSLifecycleTestAccess::readEvent(helper);
            releaseDisabledEvents();
            TLSLifecycleTestAccess::triggerWriteEvent(*f.connection);
            char out[32] = {};
            const ssize_t n = ::recv(f.pipeFd.writeFd(), out, sizeof(out), MSG_DONTWAIT);
            result.expectTrue(n == 11 && std::string(out, out + n) == "plain-after",
                              "plaintext writer ordering: raw peer receives exactly plain-after after SSL release");
            result.expectEqual(0,
                               static_cast<int>(TLSLifecycleTestAccess::queuedWriteBytes(*f.connection)),
                               "plaintext writer ordering: queue empty afterward");
        }

        resetTlsTestState();
        {
            TestFixture f(true);
            makeTlsActive(f);
            TLSLifecycleTestAccess::enqueueShutdownResult(-1, SSL_ERROR_WANT_READ);
            TLSLifecycleTestAccess::doSSLShutdown(*f.connection);
            f.writeOnDisconnect = true;
            f.connection->sendToPeer("before-close", 12);
            TLSShutdown* helper = TLSLifecycleTestAccess::lastShutdown();
            TLSLifecycleTestAccess::enqueueShutdownResult(1, SSL_ERROR_NONE);
            TLSLifecycleTestAccess::readEvent(helper);
            releaseDisabledEvents();
            result.expectEqual(1, f.disconnects, "CloseTransport writer policy: terminal close finalizes once");
            result.expectEqual(8, f.stateAtDisconnect, "CloseTransport writer policy: terminal state Closed");
            result.expectEqual(0,
                               TLSLifecycleTestAccess::writerCounters().operationCalls,
                               "CloseTransport writer policy: no TLS write after shutdown ownership begins");
            result.expectEqual(
                1, f.postCloseWriteAttempts, "CloseTransport writer policy: one post-close write attempt is ignored consistently");
            char out[32] = {};
            result.expectTrue(::recv(f.pipeFd.writeFd(), out, sizeof(out), MSG_DONTWAIT) < 0,
                              "CloseTransport writer policy: no raw write after Closing begins");
        }
    }

    void handoffBuffer(TestResult& result) {
        resetTlsTestState();
        TestFixture f(false);
        const std::string handoff = "abcdef";
        TLSLifecycleTestAccess::appendHandoffBytes(*f.connection, handoff.data(), handoff.size());
        result.expectEqual(6, static_cast<int>(TLSLifecycleTestAccess::handoffBufferSize(*f.connection)), "handoff buffer records bytes");
    }

    std::string readAvailable(TestConnection& connection, std::size_t maxBytes = 64) {
        std::string out;
        std::array<char, 16> buffer{};
        while (out.size() < maxBytes) {
            const ssize_t n = TLSLifecycleTestAccess::readFromConnection(connection, buffer.data(), buffer.size());
            if (n <= 0) {
                break;
            }
            out.append(buffer.data(), buffer.data() + n);
            if (static_cast<std::size_t>(n) < buffer.size()) {
                break;
            }
        }
        return out;
    }

    void transitionMatrix(TestResult& result) {
        resetTlsTestState();
        TestFixture f;
        bool expected[9][9] = {};
        auto allow = [&expected](int from, std::initializer_list<int> toStates) {
            for (int to : toStates) {
                expected[from][to] = true;
            }
        };
        for (int state = 0; state < 9; ++state) {
            expected[state][state] = true; // idempotent owner re-entry is part of the intended state contract.
        }

        // Plaintext: raw transport, startSSL preparation, or terminal cleanup.
        allow(0, {1, 6, 8});
        // TlsPrepared: setup can roll back, enter handshake, be stopped, fail, or start a queued shutdown owner.
        allow(1, {0, 2, 4, 6, 7});
        // Handshaking: only handshake success, fatal handshake, or explicit close/stopSSL cleanup.
        allow(2, {3, 6, 7});
        // TlsActive: later handshake, graceful shutdown, fatal I/O, direct stopSSL plaintext teardown, or close.
        allow(3, {0, 2, 4, 6, 7});
        // ShutdownInProgress: helper completion, fatal shutdown, or explicit close while helper owns the transition.
        allow(4, {5, 6, 7});
        // ShutdownCompleteAwaitingRelease: helper release selects plaintext continuation, close finalization, or fail-closed.
        allow(5, {0, 6, 7});
        // Closing: terminal close is expected; fatal is tolerated during cleanup diagnostics.
        allow(6, {7, 8});
        // Fatal: fatal paths only move into final close states, never back to plaintext.
        allow(7, {6, 8});
        // Closed: no resurrection; only self-transition was enabled above.

        for (int from = 0; from < 9; ++from) {
            for (int to = 0; to < 9; ++to) {
                result.expectEqual(expected[from][to] ? 1 : 0,
                                   TLSLifecycleTestAccess::isLegalTransition(*f.connection, from, to) ? 1 : 0,
                                   "9x9 TLS transition matrix entry");
            }
        }
    }

    void stopSslActiveHelpers(TestResult& result) {
        resetTlsTestState();
        {
            TestFixture f;
            TLSLifecycleTestAccess::enqueueHandshakeResult(-1, SSL_ERROR_WANT_WRITE);
            result.expectTrue(TLSLifecycleTestAccess::doSSLHandshake(
                                  *f.connection,
                                  [] {
                                  },
                                  [] {
                                  },
                                  [](int) {
                                  }),
                              "handshake starts before stopSSL");
            TLSHandshake* helper = TLSLifecycleTestAccess::lastHandshake();
            TLSLifecycleTestAccess::stopSSL(*f.connection);
            result.expectEqual(6, TLSLifecycleTestAccess::transportState(*f.connection), "stopSSL during handshake enters Closing");
            result.expectTrue(TLSLifecycleTestAccess::writerActivationBlocked(*f.connection),
                              "writer remains blocked during handshake cleanup");
            result.expectTrue(TLSLifecycleTestAccess::lifecycleHasSSL(*f.connection), "helper keeps SSL alive until handshake release");
            TLSLifecycleTestAccess::enqueueHandshakeResult(1, SSL_ERROR_NONE);
            TLSLifecycleTestAccess::writeEvent(helper);
            result.expectTrue(TLSLifecycleTestAccess::pendingStopSSL(*f.connection),
                              "handshake stopSSL keeps release requested until deferred cleanup");
            result.expectTrue(TLSLifecycleTestAccess::handshakeGuardActive(*f.connection),
                              "handshake guard remains active until deferred release");
            releaseDisabledEvents();
            result.expectEqual(1, TLSLifecycleTestAccess::handshakeCounters().releases, "handshake helper release observed");
            result.expectEqual(1, TLSLifecycleTestAccess::handshakeCounters().destroyed, "handshake helper destroyed after release");
            result.expectTrue(!TLSLifecycleTestAccess::handshakeGuardActive(*f.connection), "handshake guard clears after release");
            result.expectTrue(!TLSLifecycleTestAccess::lifecycleHasSSL(*f.connection),
                              "stopSSL releases lifecycle SSL after handshake helper release");
            result.expectEqual(
                6, TLSLifecycleTestAccess::transportState(*f.connection), "stopSSL during handshake remains Closing after release");
            result.expectTrue(!TLSLifecycleTestAccess::sslAttached(*f.connection),
                              "stopSSL during handshake keeps connection SSL detached");
            result.expectTrue(TLSLifecycleTestAccess::writerActivationBlocked(*f.connection),
                              "raw I/O remains blocked after handshake stopSSL release");
        }

        resetTlsTestState();
        {
            TestFixture f(true);
            makeTlsActive(f);
            TLSLifecycleTestAccess::enqueueShutdownResult(-1, SSL_ERROR_WANT_READ);
            TLSLifecycleTestAccess::doSSLShutdown(*f.connection);
            TLSShutdown* helper = TLSLifecycleTestAccess::lastShutdown();
            TLSLifecycleTestAccess::stopSSL(*f.connection);
            result.expectEqual(
                1, TLSLifecycleTestAccess::shutdownIntent(*f.connection), "stopSSL during shutdown escalates to CloseTransport");
            result.expectEqual(4,
                               TLSLifecycleTestAccess::transportState(*f.connection),
                               "stopSSL during shutdown keeps ShutdownInProgress until helper completes");
            result.expectTrue(!TLSLifecycleTestAccess::sslAttached(*f.connection),
                              "stopSSL during shutdown detaches connection SSL pointers");
            result.expectTrue(TLSLifecycleTestAccess::writerActivationBlocked(*f.connection),
                              "writer remains blocked during shutdown cleanup");
            result.expectTrue(TLSLifecycleTestAccess::lifecycleHasSSL(*f.connection), "shutdown helper keeps SSL alive until release");
            result.expectTrue(!TLSLifecycleTestAccess::closeEofPending(*f.connection),
                              "stopSSL during shutdown clears clean EOF notification");
            TLSLifecycleTestAccess::enqueueShutdownResult(1, SSL_ERROR_NONE);
            TLSLifecycleTestAccess::readEvent(helper);
            result.expectTrue(TLSLifecycleTestAccess::shutdownGuardActive(*f.connection),
                              "shutdown guard remains active until deferred release");
            releaseDisabledEvents();
            result.expectEqual(1, TLSLifecycleTestAccess::shutdownCounters().releases, "shutdown helper release observed");
            result.expectEqual(1, TLSLifecycleTestAccess::shutdownCounters().destroyed, "shutdown helper destroyed after release");
            result.expectEqual(1, f.disconnects, "stopSSL during shutdown terminal close disconnects once");
            result.expectTrue(!f.shutdownGuardAtDisconnect, "shutdown guard clears before terminal disconnect");
            result.expectTrue(!f.lifecycleHasSslAtDisconnect, "stopSSL during shutdown releases lifecycle SSL after helper release");
            result.expectTrue(!f.sslAttachedAtDisconnect, "stopSSL during shutdown leaves connection SSL detached");
            result.expectEqual(1, f.counters->shutdownWr, "stopSSL during shutdown performs one SHUT_WR");
            result.expectEqual(8, f.stateAtDisconnect, "stopSSL during shutdown ends Closed");
            result.expectTrue(f.writerBlockedAtDisconnect, "writer remains blocked after shutdown close");
            result.expectEqual(0, TLSLifecycleTestAccess::readerCounters().operationCalls, "stopSSL during shutdown performs no raw read");
            result.expectEqual(0, TLSLifecycleTestAccess::writerCounters().operationCalls, "stopSSL during shutdown performs no raw write");
        }
    }

    void realHandoff(TestResult& result) {
        resetTlsTestState();
        {
            TestFixture f(false);
            makeTlsActive(f);
            TLSLifecycleTestAccess::enqueueShutdownResult(1, SSL_ERROR_NONE);
            TLSLifecycleTestAccess::onReadShutdown(*f.connection);
            result.expectEqual(
                0, static_cast<int>(TLSLifecycleTestAccess::handoffBufferSize(*f.connection)), "zero buffered handoff remains empty");
            result.expectTrue(!TLSLifecycleTestAccess::sslAttached(*f.connection), "zero buffered handoff releases SSL");
        }

        resetTlsTestState();
        {
            TestFixture f(false, 2);
            result.expectEqual(2, static_cast<int>(f.config->getReadBlockSize()), "same-socket handoff uses a two-byte candidate limit");
            makeNonBlocking(f.pipeFd.fd());
            makeNonBlocking(f.pipeFd.writeFd());
            SocketPeerTls peer(f.pipeFd.writeFd());
            SSL* connectionSsl = f.connection->getSSL();
            result.expectTrue(connectionSsl != nullptr, "connection SSL exists for same-socket proof");
            result.expectEqual(f.pipeFd.fd(), SSL_get_fd(connectionSsl), "connection SSL uses fixture socket fd");
            result.expectEqual(f.pipeFd.writeFd(), SSL_get_fd(peer.ssl), "peer SSL uses fixture peer fd");
            int handshakeCallbacks = 0;
            std::vector<logger::LogRecord> readinessRecords;
            result.expectTrue(driveProductionHandshake(f, peer.ssl, handshakeCallbacks, &readinessRecords),
                              "same-socket production TLS handshake completes");
            result.expectEqual(1, handshakeCallbacks, "connection handshake callback occurs once");
            result.expectTrue(readinessRecords.size() == 1 && readinessRecords.front().message == "transport ready" &&
                                  readinessRecords.front().level == logger::LogLevel::Info &&
                                  readinessRecords.front().origin == logger::LogOrigin::Framework &&
                                  readinessRecords.front().boundary == logger::LogBoundary::Connection &&
                                  readinessRecords.front().component == "core.socket.stream" &&
                                  readinessRecords.front().role == std::optional<logger::LogRole>(logger::LogRole::Client) &&
                                  readinessRecords.front().connection == std::optional<std::string>("1"),
                              "successful production TLS handshake emits one connection-bound Info readiness record");
            result.expectEqual(3, TLSLifecycleTestAccess::transportState(*f.connection), "connection is TlsActive after real handshake");
            result.expectTrue(TLSLifecycleTestAccess::sslAttached(*f.connection), "SSL remains attached after real handshake");
            const std::string tlsVersion = SSL_get_version(connectionSsl);
            result.expectTrue(!tlsVersion.empty(), "negotiated TLS version recorded");

            result.expectTrue(sslWriteAll(peer.ssl, "tls", 3), "peer writes TLS payload tls");
            char firstTlsByte = 0;
            int firstTlsRead = -1;
            for (int i = 0; i < 1000 && firstTlsRead != 1; ++i) {
                firstTlsRead = SSL_read(connectionSsl, &firstTlsByte, 1);
                if (firstTlsRead <= 0) {
                    const int err = SSL_get_error(connectionSsl, firstTlsRead);
                    if (err != SSL_ERROR_WANT_READ && err != SSL_ERROR_WANT_WRITE) {
                        break;
                    }
                }
            }
            result.expectEqual(1, firstTlsRead, "first TLS byte read through connection SSL");
            result.expectTrue(firstTlsByte == 't', "first observed TLS byte is t");
            result.expectEqual(2, SSL_pending(connectionSsl), "candidate exactly at the configured limit is pending before shutdown");

            result.expectTrue(drivePeerCloseNotifySent(peer.ssl), "peer close_notify sent on same socket");
            result.expectTrue((SSL_get_shutdown(peer.ssl) & SSL_SENT_SHUTDOWN) != 0, "peer SSL_SENT_SHUTDOWN flag is set");

            result.expectTrue(driveProductionShutdownToQuiescence(f), "production ContinuePlaintext shutdown reaches quiescence");
            result.expectTrue(f.connection != nullptr, "exact-limit handoff preserves the connection");

            if (f.connection != nullptr) {
                result.expectEqual(
                    0, TLSLifecycleTestAccess::transportState(*f.connection), "connection returns to Plaintext after helper release");
                result.expectTrue(!TLSLifecycleTestAccess::sslAttached(*f.connection), "connection SSL detached after helper release");
                result.expectTrue(!TLSLifecycleTestAccess::shutdownGuardActive(*f.connection), "shutdown helper guard released");
                result.expectEqual(2,
                                   static_cast<int>(TLSLifecycleTestAccess::handoffBufferSize(*f.connection)),
                                   "handoff committed exactly two decrypted TLS bytes");
                result.expectTrue(TLSLifecycleTestAccess::handoffBufferContents(*f.connection) == "ls", "handoff contains exactly ls");

                result.expectTrue(drivePeerFullShutdown(peer.ssl), "peer receives local close_notify and completes TLS shutdown");
                result.expectTrue((SSL_get_shutdown(peer.ssl) & SSL_RECEIVED_SHUTDOWN) != 0, "peer received the local close_notify");

                peer.releaseSSL();

                const std::string rawBytes = "-raw";
                result.expectTrue(::write(f.pipeFd.writeFd(), rawBytes.data(), rawBytes.size()) == static_cast<ssize_t>(rawBytes.size()),
                                  "peer writes raw bytes after both TLS sides released SSL");

                std::string connectionOutput;
                char out[8] = {};

                ssize_t n = TLSLifecycleTestAccess::readFromConnection(*f.connection, out, 1);
                result.expectTrue(n == 1 && std::string(out, out + n) == "l", "first connection read returns l");
                if (n > 0) {
                    connectionOutput.append(out, out + n);
                }

                n = TLSLifecycleTestAccess::readFromConnection(*f.connection, out, 1);
                result.expectTrue(n == 1 && std::string(out, out + n) == "s", "second connection read returns s");
                if (n > 0) {
                    connectionOutput.append(out, out + n);
                }

                n = TLSLifecycleTestAccess::readFromConnection(*f.connection, out, 4);
                result.expectTrue(n == 4 && std::string(out, out + n) == "-raw", "third connection read returns raw continuation");
                if (n > 0) {
                    connectionOutput.append(out, out + n);
                }

                result.expectTrue(connectionOutput == "ls-raw", "connection output is ls-raw");

                std::string observedStream;
                observedStream.push_back(firstTlsByte);
                observedStream += connectionOutput;
                result.expectTrue(observedStream == "tls-raw", "observed complete stream is tls-raw");
                result.expectTrue(TLSLifecycleTestAccess::transportState(*f.connection) == 0,
                                  "candidate exactly at the configured limit succeeds");
            }
        }

        resetTlsTestState();
        {
            TestFixture f(false, 1);
            result.expectEqual(1, static_cast<int>(f.config->getReadBlockSize()), "overflow proof uses a one-byte candidate limit");
            makeNonBlocking(f.pipeFd.fd());
            makeNonBlocking(f.pipeFd.writeFd());
            SocketPeerTls peer(f.pipeFd.writeFd());
            SSL* connectionSsl = f.connection->getSSL();
            result.expectTrue(connectionSsl != nullptr, "overflow proof connection SSL exists");
            result.expectEqual(f.pipeFd.fd(), SSL_get_fd(connectionSsl), "overflow proof connection SSL uses fixture socket fd");
            result.expectEqual(f.pipeFd.writeFd(), SSL_get_fd(peer.ssl), "overflow proof peer SSL uses fixture peer fd");
            int handshakeCallbacks = 0;
            result.expectTrue(driveProductionHandshake(f, peer.ssl, handshakeCallbacks), "overflow proof real TLS handshake completes");
            result.expectEqual(1, handshakeCallbacks, "overflow proof handshake callback occurs once");
            result.expectTrue(sslWriteAll(peer.ssl, "tls", 3), "overflow proof peer writes TLS payload tls");
            char firstTlsByte = 0;
            int firstTlsRead = -1;
            for (int i = 0; i < 1000 && firstTlsRead != 1; ++i) {
                firstTlsRead = SSL_read(connectionSsl, &firstTlsByte, 1);
                if (firstTlsRead <= 0) {
                    const int err = SSL_get_error(connectionSsl, firstTlsRead);
                    if (err != SSL_ERROR_WANT_READ && err != SSL_ERROR_WANT_WRITE) {
                        break;
                    }
                }
            }
            result.expectEqual(1, firstTlsRead, "overflow proof first TLS byte read through connection SSL");
            result.expectTrue(firstTlsByte == 't', "overflow proof first observed TLS byte is t");
            result.expectEqual(2, SSL_pending(connectionSsl), "overflow proof has two real decrypted bytes pending before shutdown");
            result.expectEqual(2, SSL_pending(connectionSsl), "two pending bytes exceed the candidate limit by one");
            result.expectTrue(drivePeerCloseNotifySent(peer.ssl), "overflow proof peer close_notify sent on same socket");
            result.expectTrue(driveProductionShutdownToQuiescence(f), "overflow shutdown reaches terminal quiescence");
            result.expectTrue(f.connection == nullptr, "candidate overflow destroys the connection");
            result.expectEqual(1, f.disconnects, "candidate overflow closes the transport exactly once");
            result.expectEqual(8, f.stateAtDisconnect, "candidate overflow reaches Closed");
            result.expectEqual(EPROTO, f.shutdownFailureErrnoAtDisconnect, "candidate overflow fails with EPROTO");
            result.expectEqual(0, static_cast<int>(f.handoffBufferSizeAtDisconnect), "candidate overflow commits no handoff bytes");
            result.expectTrue(f.handoffBufferContentsAtDisconnect.empty(), "candidate overflow exposes no partial handoff contents");
            result.expectTrue(f.writerBlockedAtDisconnect, "candidate overflow never enables raw writing");
            result.expectTrue(!f.sslAttachedAtDisconnect, "candidate overflow releases attached SSL");
            result.expectTrue(!f.lifecycleHasSslAtDisconnect, "candidate overflow releases lifecycle SSL after helper release");
            result.expectEqual(0, TLSLifecycleTestAccess::readerCounters().operationCalls, "candidate overflow never enables raw reading");
            result.expectEqual(0, TLSLifecycleTestAccess::writerCounters().operationCalls, "candidate overflow performs no raw write");
            result.expectTrue(f.counters->shutdownWr <= 1, "candidate overflow performs at most one terminal write shutdown");
        }
    }

} // namespace

int main() {
    TestResult result;
    SSL_library_init();
    handshakeLifetime(result);
    stopSslActiveHelpers(result);
    shutdownFinalization(result);
    cleanEofReentrancy(result);
    shutdownFailureCallbacks(result);
    shutdownTimeoutAndFailureMatrix(result);
    shutdownCoalescingAndHandshakeBoundary(result);
    handshakeCallbackReentrancy(result);
    staleWriteGate(result);
    writerOrderingFinalProof(result);
    handoffBuffer(result);
    transitionMatrix(result);
    realHandoff(result);
    return result.processResult();
}
