/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
 */

#include "core/socket/stream/tls/ssl_utils.h"
#include "support/TestResult.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <openssl/ssl.h>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace {
    class SslCtxGuard {
    public:
        SslCtxGuard()
            : ctx(SSL_CTX_new(TLS_client_method())) {
        }

        ~SslCtxGuard() {
            if (ctx != nullptr) {
                SSL_CTX_free(ctx);
            }
        }

        SSL_CTX* get() const {
            return ctx;
        }

    private:
        SSL_CTX* ctx = nullptr;
    };

    class SslGuard {
    public:
        explicit SslGuard(SSL_CTX* ctx)
            : ssl(SSL_new(ctx)) {
        }

        ~SslGuard() {
            if (ssl != nullptr) {
                SSL_free(ssl);
            }
        }

        SSL* get() const {
            return ssl;
        }

    private:
        SSL* ssl = nullptr;
    };
} // namespace

int main() {
    tests::support::TestResult testResult;

    SslCtxGuard ctx;
    testResult.expectTrue(ctx.get() != nullptr, "OpenSSL client context is created");
    if (ctx.get() == nullptr) {
        return testResult.processResult();
    }

    SslGuard ssl(ctx.get());
    testResult.expectTrue(ssl.get() != nullptr, "OpenSSL SSL object is created");
    if (ssl.get() == nullptr) {
        return testResult.processResult();
    }

    SSL_set_verify(ssl.get(), SSL_VERIFY_PEER, nullptr);
    testResult.expectTrue(!core::socket::stream::tls::ssl_set_peer_identity(ssl.get(), ""),
                          "empty peer identity fails closed when verification is enabled");
    testResult.expectTrue(core::socket::stream::tls::ssl_set_peer_identity(ssl.get(), "example.test"),
                          "DNS peer identity is accepted when verification is enabled");
    testResult.expectTrue(core::socket::stream::tls::ssl_set_peer_identity(ssl.get(), "127.0.0.1"),
                          "IP peer identity is accepted when verification is enabled");

    SSL_set_verify(ssl.get(), SSL_VERIFY_NONE, nullptr);
    testResult.expectTrue(core::socket::stream::tls::ssl_set_peer_identity(ssl.get(), ""),
                          "empty peer identity is accepted only when verification is disabled");
    testResult.expectTrue(core::socket::stream::tls::ssl_set_sni(ssl.get(), "example.test"), "DNS SNI setup succeeds");
    testResult.expectTrue(core::socket::stream::tls::ssl_is_ip_address("127.0.0.1"), "IPv4 literal is recognized");
    testResult.expectTrue(core::socket::stream::tls::ssl_is_ip_address("::1"), "IPv6 literal is recognized");
    testResult.expectTrue(!core::socket::stream::tls::ssl_is_ip_address("example.test"), "DNS name is not treated as an IP literal");

    return testResult.processResult();
}
