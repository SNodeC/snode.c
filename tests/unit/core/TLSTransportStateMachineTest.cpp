#include "net/in/stream/tls/SocketServer.h"
#include "tests/support/NetworkTest.h"
int main(int argc, char** argv) {
    using namespace tests::support;
    const bool plaintext = argc > 1 && std::string(argv[1]) == "plaintext";
    TestResult result;
    Runtime runtime;
    Session state;
    Peer peer;
    net::in::stream::tls::SocketServer<Factory, Session&> server("transport-server", state);
    server.getConfig()->Instance::forceUnrequired();
    server.getConfig()->setNoCloseNotifyIsEOF(plaintext);
    require(installCertificate(server.getConfig()->getSslCtx()), "certificate");
    server.listen(net::in::SocketAddress("127.0.0.1", 0), [&](const auto& addr, auto status) {
        require(status == core::socket::State::OK, "listen");
        peer.connect(addr.getPort(), true);
    });
    bool sent = false, shutdown = false, rawSent = false;
    result.expectTrue(runUntil(
                          [&] {
                              return plaintext ? state.received == "tls-raw" : state.disconnected == 1;
                          },
                          [&] {
                              peer.handshake();
                              if (peer.ready && !sent)
                                  sent = peer.write("tls") == 3;
                              if (sent && peer.ssl && !shutdown)
                                  shutdown = SSL_shutdown(peer.ssl) == 1;
                              if (plaintext && shutdown && !rawSent) {
                                  SSL_free(peer.ssl);
                                  peer.ssl = nullptr;
                                  rawSent = peer.write("-raw") == 4;
                              }
                          }),
                      "TLS shutdown reaches the configured public outcome");
    result.expectEqual(1, state.connected, "one TLS session");
    result.expectTrue(state.received == (plaintext ? "tls-raw" : "tls"), "no lost, duplicate or reordered bytes");
    result.expectEqual(0, state.errors, "orderly close_notify is not an error");
    return result.processResult();
}
