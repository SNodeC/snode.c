#include "net/in/stream/tls/SocketServer.h"
#include "tests/support/NetworkTest.h"

#include <atomic>
#include <thread>

int main(int argc, char** argv) {
    using namespace tests::support;
    const bool stall = argc > 1 && std::string(argv[1]) == "stall";
    TestResult result;
    Runtime runtime;
    Session state;
    net::in::stream::tls::SocketServer<Factory, Session&> server("shutdown-server", state);
    server.getConfig()->Instance::forceUnrequired();
    server.getConfig()->setTerminateTimeout(utils::Timeval(0.2));
    server.getConfig()->setShutdownTimeout(utils::Timeval(0.2));
    require(installCertificate(server.getConfig()->getSslCtx()), "certificate");
    std::atomic<bool> finish{false}, peerReady{false}, peerSawClose{false};
    std::thread peerThread;
    state.onData = [&](auto*) {
        core::SNodeC::stop();
    };
    server.listen(net::in::SocketAddress("127.0.0.1", 0), [&](const auto& addr, auto status) {
        require(status == core::socket::State::OK, "listen");
        peerThread = std::thread([&, port = addr.getPort()] {
            Peer peer;
            peer.connect(port, true);
            bool sent = false;
            while (!finish.load()) {
                peer.handshake();
                if (peer.ready) {
                    peerReady = true;
                    if (!sent)
                        sent = peer.write("stop") == 4;
                    if (sent && !stall) {
                        char data[32];
                        peer.read(data, sizeof(data));
                        if (SSL_get_shutdown(peer.ssl) & SSL_RECEIVED_SHUTDOWN) {
                            peerSawClose = true;
                            SSL_shutdown(peer.ssl);
                        }
                    }
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        });
    });
    auto deadline = core::timer::Timer::singleshotTimer(
        [] {
            core::SNodeC::stop();
        },
        utils::Timeval(8));
    const auto start = std::chrono::steady_clock::now();
    core::SNodeC::start();
    finish = true;
    if (peerThread.joinable())
        peerThread.join();
    result.expectTrue(peerReady && state.received == "stop", "shutdown requested during an established TLS session");
    result.expectEqual(1, state.disconnected, "framework shutdown disconnects once");
    result.expectTrue(std::chrono::steady_clock::now() - start < std::chrono::seconds(5), "framework drain is bounded");
    if (!stall)
        result.expectTrue(peerSawClose, "cooperative peer receives TLS close_notify");
    return result.processResult();
}
