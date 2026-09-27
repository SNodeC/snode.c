#include "net/in/stream/legacy/SocketServer.h"
#include "tests/support/NetworkTest.h"
int main() {
    using namespace tests::support;
    TestResult result;
    Runtime runtime;
    Session first, second;
    Peer peer;
    net::in::stream::legacy::SocketServer<Factory, Session&> server("context-server", first);
    server.getConfig()->Instance::forceUnrequired();
    first.onData = [&](auto* context) {
        context->getSocketConnection()->setSocketContext(new Context(context->getSocketConnection(), second));
    };
    server.listen(net::in::SocketAddress("127.0.0.1", 0), [&](const auto& address, auto status) {
        require(status == core::socket::State::OK, "listen");
        peer.connect(address.getPort());
        require(peer.write("switch") == 6, "request context replacement");
    });
    bool sent = false;
    result.expectTrue(runUntil(
                          [&] {
                              return second.disconnected == 1;
                          },
                          [&] {
                              if (second.connected && !sent)
                                  sent = peer.write("replacement") == 11;
                              if (second.received == "replacement")
                                  peer.close();
                          }),
                      "context replacement and close complete through real connection");
    result.expectEqual(1, first.connected, "initial context attached once");
    result.expectEqual(1, first.disconnected, "initial context detached once");
    result.expectEqual(1, second.connected, "replacement context attached once");
    result.expectEqual(1, second.disconnected, "replacement context detached once");
    result.expectTrue(first.received == "switch" && second.received == "replacement", "data reaches the current context only");
    return result.processResult();
}
