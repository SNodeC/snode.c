#include "HttpServerClientBehaviorTest.h"
#include "net/in/SocketAddress.h"
#include "support/Phase3SemanticLogCapture.h"
#include "web/http/legacy/in/Client.h"
#include "web/http/legacy/in/Server.h"

#include <string_view>

int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[]) {
    tests::support::TestResult testResult;
    int result = tests::support::cTestSkipReturnCode;

    if (tests::support::shouldSkipRootWithoutSNodeCGroup()) {
        tests::support::printRootWithoutSNodeCGroupSkipMessage("InetHttpClientPrematureServerCloseTest");
    } else {
        tests::component::http::BehaviorState state;
        tests::support::Phase3SemanticLogCapture capture("snodec-phase3-http-premature-close");

        capture.initCore("InetHttpClientPrematureServerCloseTest");

        using Server = web::http::legacy::in::Server;
        using Client = web::http::legacy::in::Client;

        const Server server("ipv4-http-behavior-server", [&state](const auto& request, const auto& response) {
            tests::component::http::handlePrematureCloseRequest(request, response, state);
        });
        Client client(
            "ipv4-http-behavior-client",
            [&state](const auto& request) {
                tests::component::http::sendPrematureCloseRequest(request, state);
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
        tests::component::http::expectBehaviorCommon(testResult, state, "premature server close", startResult);
        testResult.expectEqual(1, state.serverRequestCount, "IPv4 legacy HTTP server observes one premature-close request");
        testResult.expectEqual(1, state.clientResponseCount, "IPv4 legacy HTTP client observes connection-loss response");
        testResult.expectTrue(state.serverUrls == std::vector<std::string>({"/premature-close"}), "IPv4 legacy HTTP server observes premature-close target");
        testResult.expectTrue(state.clientStatuses == std::vector<std::string>({"0"}), "IPv4 legacy HTTP client maps premature close to status 0");
        testResult.expectTrue(state.clientBodies == std::vector<std::string>({"Connection loss"}), "IPv4 legacy HTTP client reports connection-loss reason");
        core::SNodeC::free();
        const auto records = capture.finish();
        constexpr std::string_view component = "web.http.client";
        constexpr std::string_view instance = "ipv4-http-behavior-client";
        testResult.expectEqual(
            1, capture.count(records, component, instance, "request started:"), "interrupted client request starts once");
        testResult.expectEqual(
            1, capture.count(records, component, instance, "request aborted:"), "interrupted client request aborts once");
        testResult.expectEqual(
            0, capture.count(records, component, instance, "request completed:"), "interrupted client request never completes");
        testResult.expectEqual(
            0, capture.count(records, component, instance, "request failed:"), "transport loss is not a request parse failure");
        testResult.expectTrue(capture.matchingIdentityAndLevel(records, component, instance, "request ", "debug", true, "client"),
                              "interrupted client request lifecycle uses matching connection identity and Debug level");
        result = testResult.processResult();
    }

    return result;
}
