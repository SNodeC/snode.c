#include "HttpServerClientBehaviorTest.h"

#include "net/in/SocketAddress.h"
#include "web/http/legacy/in/Client.h"
#include "web/http/legacy/in/Server.h"

int main(int argc, char* argv[]) {
    tests::support::TestResult testResult;
    int result = tests::support::cTestSkipReturnCode;

    if (tests::support::shouldSkipRootWithoutSNodeCGroup()) {
        tests::support::printRootWithoutSNodeCGroupSkipMessage("InetHttpServerClientLargeResponseBodyTest");
    } else {
        tests::component::http::BehaviorState state;

        core::SNodeC::init(argc, argv);

        using Server = web::http::legacy::in::Server;
        using Client = web::http::legacy::in::Client;

        const Server server("ipv4-http-behavior-server", [&state](const auto& request, const auto& response) {
            tests::component::http::handleLargeResponseRequest(request, response, state);
        });
        Client client(
            "ipv4-http-behavior-client",
            [&state](const auto& request) {
                tests::component::http::sendLargeResponseRequest(request, state);
            },
            [&state](const auto&) {
                ++state.httpDisconnectedCount;
            });

        tests::component::http::configureHttpBehaviorTest(server, client, state);

        server.listen(net::in::SocketAddress("127.0.0.1", 0), [&client, &state](const auto& socketAddress, core::socket::State listenState) {
            if (listenState == core::socket::State::OK) {
                ++state.listenOkCount;
                const std::uint16_t effectivePort = socketAddress.getPort();
                if (effectivePort != 0) {
                    ++state.effectiveListenEndpointOkCount;
                    client.connect(net::in::SocketAddress("127.0.0.1", effectivePort), [&state](const auto&, core::socket::State connectState) {
                        if (connectState == core::socket::State::OK) {
                            ++state.clientConnectOkCount;
                        } else {
                            ++state.unexpectedStateCount;
                            core::SNodeC::stop();
                        }
                    });
                } else {
                    ++state.unexpectedStateCount;
                    core::SNodeC::stop();
                }
            } else {
                ++state.unexpectedStateCount;
                core::SNodeC::stop();
            }
        });

        const int startResult = core::SNodeC::start(utils::Timeval({1, 0}));
        tests::component::http::expectBehaviorCommon(testResult, state, "large response body", startResult);
        testResult.expectEqual(1, state.serverRequestCount, "IPv4 legacy HTTP server observes one large-response request");
        testResult.expectEqual(1, state.clientResponseCount, "IPv4 legacy HTTP client observes one large-response response");
        testResult.expectTrue(state.serverUrls == std::vector<std::string>({"/large-response"}), "IPv4 legacy HTTP server observes large-response target");
        testResult.expectTrue(state.clientStatuses == std::vector<std::string>({"200"}), "IPv4 legacy HTTP client observes large-response status 200");
        testResult.expectEqual(static_cast<int>(tests::component::http::boundedBodySize), static_cast<int>(state.clientBodies.front().size()), "IPv4 legacy HTTP client observes large-response body size");
        testResult.expectTrue(state.clientBodies.front() == tests::component::http::makePatternBody(tests::component::http::boundedBodySize), "IPv4 legacy HTTP client observes exact large-response body");
        testResult.expectTrue(state.clientSawContentLength, "IPv4 legacy HTTP client observes expected large-response Content-Length");
        result = testResult.processResult();
        core::SNodeC::free();
    }

    return result;
}
