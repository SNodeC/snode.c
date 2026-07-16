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
#include <array>
#include <initializer_list>
#include <memory>
#include <openssl/rsa.h>
#include <openssl/ssl.h>
#include <openssl/x509.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>

using core::socket::stream::tls::TLSHandshake;
using core::socket::stream::tls::TLSShutdown;
using core::socket::stream::tls::detail::TLSLifecycleTestAccess;
using tests::support::TestResult;

namespace {

struct PipeFd {
    int fds[2] = {-1, -1};
    PipeFd() { socketpair(AF_UNIX, SOCK_STREAM, 0, fds); }
    ~PipeFd() { if (fds[0] >= 0) close(fds[0]); if (fds[1] >= 0) close(fds[1]); }
    int fd() const { return fds[0]; }
    int writeFd() const { return fds[1]; }
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
    std::string toString(bool = true) const override { return "tls-transport-state-machine-test"; }
};

struct TestPhysicalCounters { int shutdownWr = 0; int shutdownRd = 0; int shutdownRdWr = 0; };

class TestPhysicalSocket {
public:
    using SocketAddress = TestSocketAddress;
    enum class SHUT { RD, WR, RDWR };

    TestPhysicalSocket(int fd, const std::shared_ptr<TestPhysicalCounters>& counters) : fd(fd), counters(counters) {}
    TestPhysicalSocket(TestPhysicalSocket&& other) noexcept : fd(other.fd), counters(std::move(other.counters)) { other.fd = -1; }
    TestPhysicalSocket& operator=(TestPhysicalSocket&& other) noexcept { fd = other.fd; counters = std::move(other.counters); other.fd = -1; return *this; }

    int getFd() const { return fd; }
    int getSockName(sockaddr_storage&, socklen_t& len) { len = sizeof(sockaddr_storage); return 0; }
    int getPeerName(sockaddr_storage&, socklen_t& len) { len = sizeof(sockaddr_storage); return 0; }
    const TestSocketAddress& getBindAddress() const { return address; }
    int shutdown(SHUT how) {
        if (counters) {
            if (how == SHUT::WR) counters->shutdownWr++;
            if (how == SHUT::RD) counters->shutdownRd++;
            if (how == SHUT::RDWR) counters->shutdownRdWr++;
        }
        return 0;
    }
private:
    int fd = -1;
    std::shared_ptr<TestPhysicalCounters> counters;
    TestSocketAddress address;
};

struct TestLocalConfig { static TestSocketAddress getSocketAddress(const sockaddr_storage&, socklen_t) { return {}; } };
struct TestRemoteConfig { static TestSocketAddress getSocketAddress(const sockaddr_storage&, socklen_t) { return {}; } };

class TestConfig : public net::config::ConfigInstance, public TestLocalConfig, public TestRemoteConfig {
public:
    using Local = TestLocalConfig;
    using Remote = TestRemoteConfig;
    explicit TestConfig(bool closeNotifyIsEof = true) : ConfigInstance(nextInstanceName(), Role::CLIENT), closeNotifyIsEof(closeNotifyIsEof) {}
    utils::Timeval getInitTimeout() const { return {1, 0}; }
    utils::Timeval getShutdownTimeout() const { return {1, 0}; }
    bool getNoCloseNotifyIsEOF() const { return !closeNotifyIsEof; }
    utils::Timeval getReadTimeout() const { return {1, 0}; }
    utils::Timeval getWriteTimeout() const { return {1, 0}; }
    utils::Timeval getTerminateTimeout() const { return {1, 0}; }
    std::size_t getReadBlockSize() const { return 1024; }
    std::size_t getWriteBlockSize() const { return 1024; }
private:
    static std::string nextInstanceName() {
        static int nextId = 0;
        return "tls-transport-state-machine-" + std::to_string(++nextId);
    }
    bool closeNotifyIsEof = true;
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

    explicit TestFixture(bool closeNotifyIsEof = true) : config(std::make_shared<TestConfig>(closeNotifyIsEof)) {
        connection = new TestConnection(TestPhysicalSocket(pipeFd.fd(), counters), [this](TestConnection* disconnectedConnection) {
            ++disconnects;
            stateAtDisconnect = TLSLifecycleTestAccess::transportState(*disconnectedConnection);
            writerBlockedAtDisconnect = TLSLifecycleTestAccess::writerActivationBlocked(*disconnectedConnection);
            sslAttachedAtDisconnect = TLSLifecycleTestAccess::sslAttached(*disconnectedConnection);
            lifecycleHasSslAtDisconnect = TLSLifecycleTestAccess::lifecycleHasSSL(*disconnectedConnection);
            shutdownGuardAtDisconnect = TLSLifecycleTestAccess::shutdownGuardActive(*disconnectedConnection);
            connection = nullptr;
        }, 1, config);
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
        , state(state) {}
    ~CleanEofContext() override = default;

private:
    void onConnected() override {}
    void onDisconnected() override { state.disconnected++; }
    std::size_t onReceivedFromPeer() override { return 0; }
    bool onSignal(int) override { return false; }
    void onWriteError(int) override {}
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
        result.expectTrue(TLSLifecycleTestAccess::doSSLHandshake(*f.connection, [&] { success++; }, [&] { timeout++; }, [&](int) { status++; }), "handshake starts");
        result.expectTrue(TLSLifecycleTestAccess::handshakeGuardActive(*f.connection), "handshake guard active while waiting");
        TLSHandshake* helper = TLSLifecycleTestAccess::lastHandshake();
        f.destroyConnection();
        TLSLifecycleTestAccess::enqueueHandshakeResult(1, SSL_ERROR_NONE);
        TLSLifecycleTestAccess::readEvent(helper);
        result.expectEqual(0, success, "destroyed connection suppresses success callback");
    }

    resetTlsTestState();
    {
        TestFixture f;
        TLSLifecycleTestAccess::enqueueHandshakeResult(-1, SSL_ERROR_WANT_WRITE);
        result.expectTrue(TLSLifecycleTestAccess::doSSLHandshake(*f.connection, [] {}, [] {}, [](int) {}), "handshake write-wait starts");
        TLSHandshake* helper = TLSLifecycleTestAccess::lastHandshake();
        TLSLifecycleTestAccess::stopSSL(*f.connection);
        result.expectTrue(TLSLifecycleTestAccess::pendingStopSSL(*f.connection), "stopSSL marks release during handshake");
        TLSLifecycleTestAccess::enqueueHandshakeResult(1, SSL_ERROR_NONE);
        TLSLifecycleTestAccess::writeEvent(helper);
        releaseDisabledEvents();
        result.expectTrue(!TLSLifecycleTestAccess::lifecycleHasSSL(*f.connection), "stopSSL releases lifecycle SSL after helper release");
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
        result.expectEqual(1, TLSLifecycleTestAccess::shutdownCounters().constructed, "clean EOF close reentry uses one shutdown helper");
        result.expectEqual(1, TLSLifecycleTestAccess::shutdownCounters().releases, "clean EOF close reentry releases one shutdown helper");
        result.expectEqual(1, TLSLifecycleTestAccess::shutdownCounters().destroyed, "clean EOF close reentry destroys one shutdown helper");
        result.expectEqual(0, TLSLifecycleTestAccess::shutdownCounters().active, "clean EOF close reentry leaves no active shutdown helper");
        result.expectEqual(1, f.disconnects, "clean EOF close reentry disconnects once");
        result.expectTrue(f.connection == nullptr || TLSLifecycleTestAccess::transportState(*f.connection) == 8, "clean EOF close reentry ends Closed");
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
        result.expectEqual(1, TLSLifecycleTestAccess::shutdownCounters().constructed, "clean EOF shutdownWrite reentry uses one shutdown helper");
        result.expectEqual(1, TLSLifecycleTestAccess::shutdownCounters().releases, "clean EOF shutdownWrite reentry releases one shutdown helper");
        result.expectEqual(1, TLSLifecycleTestAccess::shutdownCounters().destroyed, "clean EOF shutdownWrite reentry destroys one shutdown helper");
        result.expectEqual(0, TLSLifecycleTestAccess::shutdownCounters().active, "clean EOF shutdownWrite reentry leaves no active shutdown helper");
        result.expectEqual(1, f.disconnects, "clean EOF shutdownWrite reentry disconnects once");
        result.expectTrue(f.connection == nullptr || TLSLifecycleTestAccess::transportState(*f.connection) == 8, "clean EOF shutdownWrite reentry ends Closed");
    }
}

void shutdownFailureCallbacks(TestResult& result) {
    resetTlsTestState();
    {
        TestFixture f(true);
        makeTlsActive(f);
        int callbackCount = 0;
        TLSLifecycleTestAccess::enqueueShutdownResult(-1, SSL_ERROR_SYSCALL, EPIPE);
        TLSLifecycleTestAccess::doWriteShutdown(*f.connection, [&] { callbackCount++; });
        result.expectEqual(1, f.counters->shutdownWr, "shutdown syscall error fallback performs one SHUT_WR");
        result.expectEqual(1, callbackCount, "shutdown syscall error drains retained callback");
    }
}

void staleWriteGate(TestResult& result) {
    resetTlsTestState();
    {
        TestFixture f;
        const std::string payload = "queued";
        f.connection->sendToPeer(payload.data(), payload.size());
        result.expectEqual(static_cast<int>(payload.size()), static_cast<int>(TLSLifecycleTestAccess::queuedWriteBytes(*f.connection)), "write queued before handshake");
        TLSLifecycleTestAccess::enqueueHandshakeResult(-1, SSL_ERROR_WANT_READ);
        TLSLifecycleTestAccess::doSSLHandshake(*f.connection, [] {}, [] {}, [](int) {});
        TLSLifecycleTestAccess::triggerWriteEvent(*f.connection);
        result.expectEqual(0, TLSLifecycleTestAccess::writerCounters().operationCalls, "stale write during handshake performs no TLS write");
        result.expectEqual(static_cast<int>(payload.size()), static_cast<int>(TLSLifecycleTestAccess::queuedWriteBytes(*f.connection)), "blocked handshake write leaves queue intact");
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
        result.expectEqual(0, TLSLifecycleTestAccess::writerCounters().operationCalls, "stale write during shutdown performs no TLS write");
        TLSShutdown* helper = TLSLifecycleTestAccess::lastShutdown();
        TLSLifecycleTestAccess::enqueueShutdownResult(1, SSL_ERROR_NONE);
        TLSLifecycleTestAccess::readEvent(helper);
    }
}

void handoffBuffer(TestResult& result) {
    resetTlsTestState();
    TestFixture f(false);
    const std::string handoff = "abcdef";
    TLSLifecycleTestAccess::appendHandoffBytes(*f.connection, handoff.data(), handoff.size());
    result.expectEqual(6, static_cast<int>(TLSLifecycleTestAccess::handoffBufferSize(*f.connection)), "handoff buffer records bytes");
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
        result.expectTrue(TLSLifecycleTestAccess::doSSLHandshake(*f.connection, [] {}, [] {}, [](int) {}), "handshake starts before stopSSL");
        TLSHandshake* helper = TLSLifecycleTestAccess::lastHandshake();
        TLSLifecycleTestAccess::stopSSL(*f.connection);
        result.expectEqual(6, TLSLifecycleTestAccess::transportState(*f.connection), "stopSSL during handshake enters Closing");
        result.expectTrue(TLSLifecycleTestAccess::writerActivationBlocked(*f.connection), "writer remains blocked during handshake cleanup");
        result.expectTrue(TLSLifecycleTestAccess::lifecycleHasSSL(*f.connection), "helper keeps SSL alive until handshake release");
        TLSLifecycleTestAccess::enqueueHandshakeResult(1, SSL_ERROR_NONE);
        TLSLifecycleTestAccess::writeEvent(helper);
        result.expectTrue(TLSLifecycleTestAccess::pendingStopSSL(*f.connection), "handshake stopSSL keeps release requested until deferred cleanup");
        result.expectTrue(TLSLifecycleTestAccess::handshakeGuardActive(*f.connection), "handshake guard remains active until deferred release");
        releaseDisabledEvents();
        result.expectEqual(1, TLSLifecycleTestAccess::handshakeCounters().releases, "handshake helper release observed");
        result.expectEqual(1, TLSLifecycleTestAccess::handshakeCounters().destroyed, "handshake helper destroyed after release");
        result.expectTrue(!TLSLifecycleTestAccess::handshakeGuardActive(*f.connection), "handshake guard clears after release");
        result.expectTrue(!TLSLifecycleTestAccess::lifecycleHasSSL(*f.connection), "stopSSL releases lifecycle SSL after handshake helper release");
        result.expectEqual(6, TLSLifecycleTestAccess::transportState(*f.connection), "stopSSL during handshake remains Closing after release");
        result.expectTrue(!TLSLifecycleTestAccess::sslAttached(*f.connection), "stopSSL during handshake keeps connection SSL detached");
        result.expectTrue(TLSLifecycleTestAccess::writerActivationBlocked(*f.connection), "raw I/O remains blocked after handshake stopSSL release");
    }

    resetTlsTestState();
    {
        TestFixture f(true);
        makeTlsActive(f);
        TLSLifecycleTestAccess::enqueueShutdownResult(-1, SSL_ERROR_WANT_READ);
        TLSLifecycleTestAccess::doSSLShutdown(*f.connection);
        TLSShutdown* helper = TLSLifecycleTestAccess::lastShutdown();
        TLSLifecycleTestAccess::stopSSL(*f.connection);
        result.expectEqual(1, TLSLifecycleTestAccess::shutdownIntent(*f.connection), "stopSSL during shutdown escalates to CloseTransport");
        result.expectEqual(4, TLSLifecycleTestAccess::transportState(*f.connection), "stopSSL during shutdown keeps ShutdownInProgress until helper completes");
        result.expectTrue(!TLSLifecycleTestAccess::sslAttached(*f.connection), "stopSSL during shutdown detaches connection SSL pointers");
        result.expectTrue(TLSLifecycleTestAccess::writerActivationBlocked(*f.connection), "writer remains blocked during shutdown cleanup");
        result.expectTrue(TLSLifecycleTestAccess::lifecycleHasSSL(*f.connection), "shutdown helper keeps SSL alive until release");
        result.expectTrue(!TLSLifecycleTestAccess::closeEofPending(*f.connection), "stopSSL during shutdown clears clean EOF notification");
        TLSLifecycleTestAccess::enqueueShutdownResult(1, SSL_ERROR_NONE);
        TLSLifecycleTestAccess::readEvent(helper);
        result.expectTrue(TLSLifecycleTestAccess::shutdownGuardActive(*f.connection), "shutdown guard remains active until deferred release");
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
        result.expectEqual(0, static_cast<int>(TLSLifecycleTestAccess::handoffBufferSize(*f.connection)), "zero buffered handoff remains empty");
        result.expectTrue(!TLSLifecycleTestAccess::sslAttached(*f.connection), "zero buffered handoff releases SSL");
    }

    resetTlsTestState();
    {
        TestFixture f(false);
        makeTlsActive(f);
        const std::string bioBytes = "bio";
        BIO* bio = BIO_new_mem_buf(bioBytes.data(), static_cast<int>(bioBytes.size()));
        result.expectEqual(0, SSL_pending(f.connection->getSSL()), "BIO-only handoff starts with no decrypted TLS data");
        TLSLifecycleTestAccess::setReadBio(*f.connection, bio);
        TLSLifecycleTestAccess::enqueueShutdownResult(1, SSL_ERROR_NONE);
        TLSLifecycleTestAccess::onReadShutdown(*f.connection);
        char out[8] = {};
        const ssize_t n = TLSLifecycleTestAccess::readFromConnection(*f.connection, out, sizeof(out));
        result.expectTrue(n == 3 && std::string("bio") == std::string(out, out + n), "BIO handoff bytes preserve exact order");
    }

    resetTlsTestState();
    {
        TestFixture f(false);
        TlsPair pair;
        result.expectTrue(pair.handshake(), "memory BIO TLS handshake completes");
        result.expectTrue(pair.writeFromServer("tls"), "server writes real TLS payload");
        result.expectTrue(pair.makeClientPending(), "SSL_pending(receiverSsl) is greater than zero before handoff");
        const int pendingBefore = SSL_pending(pair.client);
        TLSLifecycleTestAccess::replaceSSL(*f.connection, pair.client);
        pair.client = nullptr;
        result.expectTrue(TLSLifecycleTestAccess::preserveTlsHandoffBytes(*f.connection), "real SSL_pending handoff succeeds");
        result.expectTrue(pendingBefore > 0, "real decrypted data was pending");
        SSL* attached = f.connection->getSSL();
        result.expectTrue(attached != nullptr, "SSL remains attached for pending-drain assertion");
        if (attached != nullptr) {
            result.expectEqual(0, SSL_pending(attached), "SSL_pending drained after handoff");
        }
        char out[8] = {};
        const ssize_t n = TLSLifecycleTestAccess::readFromConnection(*f.connection, out, sizeof(out));
        result.expectTrue(n == 3 && std::string("tls") == std::string(out, out + n), "SocketReader returns decrypted TLS payload");
        TLSLifecycleTestAccess::stopSSL(*f.connection);
    }

    resetTlsTestState();
    {
        TestFixture f(false);
        TlsPair pair;
        result.expectTrue(pair.handshake(), "ordering TLS handshake completes");
        result.expectTrue(pair.writeFromServer("tls"), "ordering server writes TLS payload");
        result.expectTrue(pair.makeClientPending(), "ordering SSL_pending is greater than zero");
        BIO_write(pair.serverToClient, "-bio", 4);
        TLSLifecycleTestAccess::replaceSSL(*f.connection, pair.client);
        pair.client = nullptr;
        result.expectTrue(TLSLifecycleTestAccess::preserveTlsHandoffBytes(*f.connection), "combined TLS/BIO handoff succeeds");
        TLSLifecycleTestAccess::stopSSL(*f.connection);
        TLSLifecycleTestAccess::transitionTo(*f.connection, 0);
        const std::string rawBytes = "-raw";
        ::write(f.pipeFd.writeFd(), rawBytes.data(), rawBytes.size());
        std::string accumulated;
        char out[8] = {};
        ssize_t n = TLSLifecycleTestAccess::readFromConnection(*f.connection, out, 2);
        accumulated.append(out, out + n);
        result.expectTrue(n == 2 && std::string("tl") == std::string(out, out + n), "first partial handoff read returns tl");
        n = TLSLifecycleTestAccess::readFromConnection(*f.connection, out, 1);
        accumulated.append(out, out + n);
        result.expectTrue(n == 1 && std::string("s") == std::string(out, out + n), "second partial handoff read returns s before BIO/raw");
        n = TLSLifecycleTestAccess::readFromConnection(*f.connection, out, 4);
        accumulated.append(out, out + n);
        result.expectTrue(n == 4 && std::string("-bio") == std::string(out, out + n), "BIO bytes follow decrypted TLS bytes");
        result.expectEqual(0, static_cast<int>(TLSLifecycleTestAccess::handoffBufferSize(*f.connection)), "handoff buffer is empty before raw read");
        n = TLSLifecycleTestAccess::readFromConnection(*f.connection, out, sizeof(out));
        accumulated.append(out, out + n);
        result.expectTrue(n == 4 && std::string("-raw") == std::string(out, out + n), "raw socket bytes follow handoff bytes");
        result.expectTrue(accumulated == "tls-bio-raw", "complete stream is tls-bio-raw without loss or duplication");
        TLSLifecycleTestAccess::stopSSL(*f.connection);
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
    staleWriteGate(result);
    handoffBuffer(result);
    transitionMatrix(result);
    realHandoff(result);
    return result.processResult();
}
