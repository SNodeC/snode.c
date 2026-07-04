#include "core/SNodeC.h"
#include "net/in/SocketAddress.h"
#include "support/TestResult.h"
#include "utils/Timeval.h"
#include "web/http/legacy/in/Client.h"

#include <arpa/inet.h>
#include <cstdint>
#include <string>
#include <sys/socket.h>
#include <unistd.h>

namespace {
    struct State {
        int clientConnectFailureCount = 0, unexpectedConnectOkCount = 0, httpConnectedCount = 0, httpDisconnectedCount = 0,
            clientResponseCount = 0, parseErrorCount = 0;
    };
    std::uint16_t reserveAndReleaseLocalPort() {
        const int fd = ::socket(AF_INET, SOCK_STREAM, 0);
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        addr.sin_port = 0;
        if (fd < 0 || ::bind(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
            if (fd >= 0)
                ::close(fd);
            return 0;
        }
        socklen_t len = sizeof(addr);
        if (::getsockname(fd, reinterpret_cast<sockaddr*>(&addr), &len) != 0) {
            ::close(fd);
            return 0;
        }
        const auto port = ntohs(addr.sin_port);
        ::close(fd);
        return port;
    }
} // namespace

int main(int argc, char* argv[]) {
    tests::support::TestResult testResult;
    int result = tests::support::cTestSkipReturnCode;
    if (tests::support::shouldSkipRootWithoutSNodeCGroup()) {
        tests::support::printRootWithoutSNodeCGroupSkipMessage("InetHttpClientConnectFailureTest");
    } else {
        State state;
        const std::uint16_t port = reserveAndReleaseLocalPort();
        core::SNodeC::init(argc, argv);
        web::http::legacy::in::Client client(
            "ipv4-http-connect-failure-client",
            [&state](const auto& request) {
                ++state.httpConnectedCount;
                request->method = "GET";
                request->url = "/must-not-send";
                request->set("Connection", "close");
                request->end(
                    [&state](const auto&, const auto&) {
                        ++state.clientResponseCount;
                        core::SNodeC::stop();
                    },
                    [&state](const auto&, const std::string&) {
                        ++state.parseErrorCount;
                        core::SNodeC::stop();
                    });
            },
            [&state](const auto&) {
                ++state.httpDisconnectedCount;
            });
        client.getConfig()->Instance::forceUnrequired();
        if (port == 0) {
            ++state.parseErrorCount;
        } else {
            client.connect(net::in::SocketAddress("127.0.0.1", port), [&state](const auto&, core::socket::State connectState) {
                if (connectState != core::socket::State::OK)
                    ++state.clientConnectFailureCount;
                else
                    ++state.unexpectedConnectOkCount;
                core::SNodeC::stop();
            });
        }
        const int startResult = core::SNodeC::start(utils::Timeval({1, 0}));
        testResult.expectTrue(port != 0, "reserved a deterministic loopback port before releasing it");
        testResult.expectEqual(0, startResult, "event loop stops successfully after IPv4 legacy HTTP connect failure");
        testResult.expectEqual(1, state.clientConnectFailureCount, "HTTP client connect callback reports one failure");
        testResult.expectEqual(0, state.unexpectedConnectOkCount, "HTTP client connect callback never reports OK");
        testResult.expectEqual(0, state.httpConnectedCount, "HTTP connected/request callback does not fire");
        testResult.expectEqual(0, state.clientResponseCount, "HTTP response callback does not fire");
        testResult.expectEqual(0, state.parseErrorCount, "HTTP parse-error callback does not fire");
        result = testResult.processResult();
        core::SNodeC::free();
    }
    return result;
}
