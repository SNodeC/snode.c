#include "HttpServerClientBehaviorTest.h"

#include "net/in/SocketAddress.h"
#include "web/http/legacy/in/Client.h"
#include "web/http/legacy/in/Server.h"

int main(int argc, char* argv[]) {
    tests::support::TestResult testResult;
    int result = tests::support::cTestSkipReturnCode;

    if (tests::support::shouldSkipRootWithoutSNodeCGroup()) {
        tests::support::printRootWithoutSNodeCGroupSkipMessage("InetHttpServerClientRepeatedRequestTest");
    } else {
        tests::component::http::BehaviorState state;

        core::SNodeC::init(argc, argv);

        using Server = web::http::legacy::in::Server;
        using Client = web::http::legacy::in::Client;

        const Server server("ipv4-http-behavior-server", [&state](const auto& request, const auto& response) {
            tests::component::http::handleRepeatedRequest(request, response, state);
        });
        Client client(
            "ipv4-http-behavior-client",
            [&state](const auto& request) {
                tests::component::http::sendRepeatedRequests(request, state);
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
        tests::component::http::expectBehaviorCommon(testResult, state, "repeated request", startResult);
        testResult.expectEqual(2, state.queuedRequestCount, "IPv4 legacy HTTP queues both keep-alive requests");
        testResult.expectEqual(2, state.serverRequestCount, "IPv4 legacy HTTP server observes both repeated requests");
        testResult.expectEqual(2, state.clientResponseCount, "IPv4 legacy HTTP client observes both repeated responses");
        testResult.expectTrue(state.serverUrls == std::vector<std::string>({"/first", "/second"}), "IPv4 legacy HTTP server observes repeated targets in order");
        testResult.expectTrue(state.clientStatuses == std::vector<std::string>({"200", "200"}), "IPv4 legacy HTTP client observes repeated status codes in order");
        testResult.expectTrue(state.clientBodies == std::vector<std::string>({"first-response", "second-response"}), "IPv4 legacy HTTP client observes repeated bodies in order");
        result = testResult.processResult();
        core::SNodeC::free();
    }

    return result;
}
