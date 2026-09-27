#include "core/socket/stream/tls/TLSHandshake.h"
#include "core/socket/stream/tls/TLSShutdown.h"
#include "tests/support/NetworkTest.h"

int main(int argc, char** argv) {
    using namespace tests::support;
    using core::socket::stream::tls::TLSHandshake;
    using core::socket::stream::tls::TLSShutdown;
    const std::string mode = argc > 1 ? argv[1] : "success";
    TestResult result;
    Runtime runtime;
    int sockets[2];
    require(socketpair(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0, sockets) == 0, "socketpair");
    SSL_CTX* serverCtx = SSL_CTX_new(TLS_server_method());
    require(serverCtx && installCertificate(serverCtx), "certificate");
    SSL* server = SSL_new(serverCtx);
    require(server != nullptr, "server SSL");
    SSL_set_fd(server, sockets[0]);
    SSL_set_accept_state(server);
    Peer peer;
    peer.fd = sockets[1];
    peer.ctx = SSL_CTX_new(TLS_client_method());
    require(peer.ctx != nullptr, "client context");
    peer.ssl = SSL_new(peer.ctx);
    require(peer.ssl != nullptr, "client SSL");
    SSL_set_fd(peer.ssl, peer.fd);
    SSL_set_connect_state(peer.ssl);
    int successes = 0, timeouts = 0, errors = 0, shutdowns = 0;
    bool shutdownStarted = false;
    TLSHandshake::doHandshake(
        "public-handshake",
        server,
        [&] {
            ++successes;
        },
        [&] {
            ++timeouts;
        },
        [&](int) {
            ++errors;
        },
        utils::Timeval(0.1));
    if (mode == "invalid")
        require(::send(peer.fd, "not TLS at all\r\n", 16, MSG_NOSIGNAL) == 16, "invalid peer input");
    result.expectTrue(runUntil(
                          [&] {
                              return mode == "shutdown" ? shutdowns == 1 : successes + timeouts + errors == 1;
                          },
                          [&] {
                              if (mode == "success" || mode == "shutdown")
                                  peer.handshake();
                              if (mode == "shutdown" && successes && peer.ready) {
                                  if (!shutdownStarted) {
                                      shutdownStarted = true;
                                      TLSShutdown::doShutdown(
                                          "public-shutdown",
                                          server,
                                          [&] {
                                              ++shutdowns;
                                          },
                                          [&] {
                                              ++timeouts;
                                          },
                                          [&](int) {
                                              ++errors;
                                          },
                                          utils::Timeval(1));
                                  }
                                  SSL_shutdown(peer.ssl);
                              }
                          }),
                      "public TLS helper reaches one terminal callback");
    result.expectEqual(mode == "timeout" ? 1 : 0, timeouts, "timeout callback count");
    result.expectEqual(mode == "invalid" ? 1 : 0, errors, "error callback count");
    result.expectEqual(mode == "success" || mode == "shutdown" ? 1 : 0, successes, "success callback count");
    if (mode == "shutdown")
        result.expectEqual(1, shutdowns, "shutdown callback occurs once");
    SSL_free(server);
    SSL_CTX_free(serverCtx);
    ::close(sockets[0]);
    return result.processResult();
}
