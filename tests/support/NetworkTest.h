// SPDX-License-Identifier: MIT OR LGPL-3.0-or-later
// Application-side fixtures only: ordinary sockets, OpenSSL peers and public APIs.
#pragma once
#include "core/SNodeC.h"
#include "core/socket/stream/SocketConnection.h"
#include "core/socket/stream/SocketContext.h"
#include "core/socket/stream/SocketContextFactory.h"
#include "core/timer/Timer.h"
#include "tests/support/TestResult.h"

#include <arpa/inet.h>
#include <chrono>
#include <fcntl.h>
#include <functional>
#include <openssl/ec.h>
#include <openssl/ssl.h>
#include <openssl/x509.h>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <unistd.h>

namespace tests::support {
    inline void require(bool ok, const char* message) {
        if (!ok)
            throw std::runtime_error(message);
    }
    struct Runtime {
        char name[32] = "public-api-test";
        char quiet[8] = "--quiet";
        char* argv[3] = {name, quiet, nullptr};
        Runtime() {
            core::SNodeC::init(2, argv);
        }
        ~Runtime() {
            core::SNodeC::free();
        }
    };
    inline bool runUntil(const std::function<bool()>& done, const std::function<void()>& step = {}) {
        bool completed = false;
        auto poll = core::timer::Timer::intervalTimer(
            [&] {
                if (step)
                    step();
                if (done()) {
                    completed = true;
                    core::SNodeC::stop();
                }
            },
            utils::Timeval(0.005));
        auto deadline = core::timer::Timer::singleshotTimer(
            [] {
                core::SNodeC::stop();
            },
            utils::Timeval(8));
        core::SNodeC::start();
        return completed;
    }
    inline bool installCertificate(SSL_CTX* ctx) {
        EVP_PKEY_CTX* generator = EVP_PKEY_CTX_new_id(EVP_PKEY_EC, nullptr);
        EVP_PKEY* key = nullptr;
        bool ok = generator && EVP_PKEY_keygen_init(generator) == 1 &&
                  EVP_PKEY_CTX_set_ec_paramgen_curve_nid(generator, NID_X9_62_prime256v1) == 1 && EVP_PKEY_keygen(generator, &key) == 1;
        EVP_PKEY_CTX_free(generator);
        X509* cert = X509_new();
        if (ok && cert) {
            X509_set_version(cert, 2);
            ASN1_INTEGER_set(X509_get_serialNumber(cert), 1);
            X509_gmtime_adj(X509_get_notBefore(cert), -60);
            X509_gmtime_adj(X509_get_notAfter(cert), 3600);
            X509_set_pubkey(cert, key);
            X509_NAME* name = X509_get_subject_name(cert);
            X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC, reinterpret_cast<const unsigned char*>("localhost"), -1, -1, 0);
            X509_set_issuer_name(cert, name);
            ok = X509_sign(cert, key, EVP_sha256()) > 0 && SSL_CTX_use_certificate(ctx, cert) == 1 && SSL_CTX_use_PrivateKey(ctx, key) == 1;
        } else
            ok = false;
        X509_free(cert);
        EVP_PKEY_free(key);
        return ok;
    }
    struct Peer {
        int fd = -1;
        SSL_CTX* ctx = nullptr;
        SSL* ssl = nullptr;
        bool ready = false;
        ~Peer() {
            SSL_free(ssl);
            SSL_CTX_free(ctx);
            if (fd >= 0)
                ::close(fd);
        }
        void connect(unsigned short port, bool tls = false) {
            fd = ::socket(AF_INET, SOCK_STREAM, 0);
            require(fd >= 0, "peer socket");
            sockaddr_in addr{};
            addr.sin_family = AF_INET;
            addr.sin_port = htons(port);
            inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
            require(::connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == 0, "peer connect");
            require(fcntl(fd, F_SETFL, O_NONBLOCK) == 0, "peer nonblocking");
            if (tls) {
                ctx = SSL_CTX_new(TLS_client_method());
                require(ctx != nullptr, "peer context");
                SSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, nullptr);
                ssl = SSL_new(ctx);
                require(ssl != nullptr, "peer SSL");
                require(SSL_set_fd(ssl, fd) == 1, "peer SSL fd");
                SSL_set_connect_state(ssl);
            } else
                ready = true;
        }
        void handshake() {
            if (ssl && !ready)
                ready = SSL_connect(ssl) == 1;
        }
        void close(bool reset = false) {
            if (fd < 0)
                return;
            if (reset) {
                linger value{1, 0};
                setsockopt(fd, SOL_SOCKET, SO_LINGER, &value, sizeof(value));
            }
            ::close(fd);
            fd = -1;
        }
        int write(const std::string& bytes) {
            return ssl ? SSL_write(ssl, bytes.data(), static_cast<int>(bytes.size()))
                       : static_cast<int>(::send(fd, bytes.data(), bytes.size(), MSG_NOSIGNAL));
        }
        int read(char* bytes, int size) {
            return ssl ? SSL_read(ssl, bytes, size) : static_cast<int>(::recv(fd, bytes, static_cast<std::size_t>(size), 0));
        }
    };
    struct Session {
        int connected = 0, disconnected = 0, errors = 0;
        std::string received;
        core::socket::stream::SocketContext* context = nullptr;
        std::function<void(core::socket::stream::SocketContext*)> onConnect;
        std::function<void(core::socket::stream::SocketContext*)> onData;
    };
    class Context : public core::socket::stream::SocketContext {
    public:
        Context(core::socket::stream::SocketConnection* connection, Session& session)
            : SocketContext(connection)
            , session(session) {
        }

    private:
        void onConnected() override {
            ++session.connected;
            session.context = this;
            if (session.onConnect)
                session.onConnect(this);
        }
        void onDisconnected() override {
            ++session.disconnected;
            session.context = nullptr;
        }
        std::size_t onReceivedFromPeer() override {
            char data[4096];
            auto count = readFromPeer(data, sizeof(data));
            session.received.append(data, count);
            if (session.onData)
                session.onData(this);
            return count;
        }
        void onReadError(int error) override {
            if (error)
                ++session.errors;
            SocketContext::onReadError(error);
        }
        void onWriteError(int error) override {
            if (error)
                ++session.errors;
            SocketContext::onWriteError(error);
        }
        bool onSignal(int) override {
            return true;
        }
        Session& session;
    };
    class Factory : public core::socket::stream::SocketContextFactory {
    public:
        explicit Factory(Session& session)
            : session(session) {
        }
        core::socket::stream::SocketContext* create(core::socket::stream::SocketConnection* connection) override {
            return new Context(connection, session);
        }

    private:
        Session& session;
    };
} // namespace tests::support
