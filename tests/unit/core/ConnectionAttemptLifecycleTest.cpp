#include "net/in/stream/legacy/SocketClient.h"
#include "net/in/stream/legacy/SocketServer.h"
#include "tests/support/NetworkTest.h"
int main(int argc, char** argv) {
    using namespace tests::support;
    const bool cancel = argc > 1 && std::string(argv[1]) == "cancel";
    TestResult result;
    Runtime runtime;
    Session clientState, serverState;
    int reservation = socket(AF_INET, SOCK_STREAM, 0);
    require(reservation >= 0, "reserve port");
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    require(bind(reservation, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == 0, "bind reserved port");
    socklen_t size = sizeof(addr);
    getsockname(reservation, reinterpret_cast<sockaddr*>(&addr), &size);
    const unsigned short port = ntohs(addr.sin_port);
    using Client = net::in::stream::legacy::SocketClient<Factory, Session&>;
    Client client("attempt-client", clientState);
    net::in::stream::legacy::SocketServer<Factory, Session&> server("attempt-server", serverState);
    client.getConfig()->Instance::forceUnrequired();
    server.getConfig()->Instance::forceUnrequired();
    client.getConfig()->setRetry(true);
    client.getConfig()->setRetryTimeout(0.01);
    client.getConfig()->setRetryTries(3);
    int failures = 0;
    bool observedCancellation = false;
    Client::FlowHandle flow;
    flow = client.connect(net::in::SocketAddress("127.0.0.1", port), [&](const auto&, auto status) {
        if (status != core::socket::State::OK) {
            ++failures;
            if (failures == 1) {
                if (cancel) {
                    flow->terminateFlow();
                    core::timer::Timer::singleshotTimer(
                        [&] {
                            observedCancellation = true;
                        },
                        utils::Timeval(0.1));
                } else {
                    ::close(reservation);
                    reservation = -1;
                    server.listen(net::in::SocketAddress("127.0.0.1", port), [](const auto&, auto status) {
                        require(status == core::socket::State::OK, "start peer after first refusal");
                    });
                }
            }
        }
    });
    result.expectTrue(runUntil([&] {
                          return cancel ? observedCancellation : clientState.connected == 1;
                      }),
                      "connection flow completes");
    result.expectEqual(1, failures, "one real connection refusal");
    result.expectEqual(cancel ? 0 : 1, clientState.connected, "cancel suppresses retry; retry reaches real server");
    if (cancel)
        result.expectTrue(flow->isTerminated() && flow->getRetryCount() == 0, "cancelled flow starts no further attempts");
    else
        result.expectEqual(1, static_cast<int>(flow->getRetryCount()), "one retry succeeds");
    if (reservation >= 0)
        ::close(reservation);
    return result.processResult();
}
