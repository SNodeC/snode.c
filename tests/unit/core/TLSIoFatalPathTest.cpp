#include "net/in/stream/tls/SocketServer.h"
#include "tests/support/NetworkTest.h"

int main(int argc, char** argv) {
    using namespace tests::support;
    const bool reset = argc > 1 && std::string(argv[1]) == "reset";
    TestResult result;
    Runtime runtime;
    Session state;
    Peer peer;
    net::in::stream::tls::SocketServer<Factory, Session&> server("fatal-server", state);
    server.getConfig()->Instance::forceUnrequired();
    server.getConfig()->setNoCloseNotifyIsEOF(false);
    require(installCertificate(server.getConfig()->getSslCtx()), "server certificate");
    server.listen(net::in::SocketAddress("127.0.0.1", 0), [&](const auto& address, auto status) {
        require(status == core::socket::State::OK, "listen");
        peer.connect(address.getPort(), true);
    });
    bool sent = false, closed = false;
    result.expectTrue(runUntil(
                          [&] {
                              return closed && state.disconnected == 1;
                          },
                          [&] {
                              peer.handshake();
                              if (peer.ready && !sent)
                                  sent = peer.write("before-failure") == 14;
                              if (state.received == "before-failure" && !closed) {
                                  peer.close(reset);
                                  closed = true;
                              }
                          }),
                      "real TLS peer failure disconnects promptly");
    result.expectEqual(1, state.connected, "one successful handshake before failure");
    result.expectEqual(1, state.disconnected, "one disconnection after failure");
    result.expectTrue(state.errors > 0, "missing close_notify is an application-visible TLS error");
    return result.processResult();
}
