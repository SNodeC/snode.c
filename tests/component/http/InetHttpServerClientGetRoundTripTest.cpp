#include "HttpServerClientRoundTripTest.h"
#include "net/in/SocketAddress.h"
#include "support/Phase3SemanticLogCapture.h"
#include "web/http/legacy/in/Client.h"
#include "web/http/legacy/in/Server.h"

#include <string_view>
#include <utility>

int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[]) {
    tests::support::TestResult testResult;
    int result = tests::support::cTestSkipReturnCode;

    if (tests::support::shouldSkipRootWithoutSNodeCGroup()) {
        tests::support::printRootWithoutSNodeCGroupSkipMessage("InetHttpServerClientGetRoundTripTest");
    } else {
        tests::component::http::RoundTripState state;
        tests::support::Phase3SemanticLogCapture capture("snodec-phase3-http-success");

        capture.initCore("InetHttpServerClientGetRoundTripTest");

        using Server = web::http::legacy::in::Server;
        using Client = web::http::legacy::in::Client;

        const Server server("ipv4-http-round-trip-server", [&state](const auto& request, const auto& response) {
            tests::component::http::handleRequest(request, response, state);
        });
        Client client(
            "ipv4-http-round-trip-client",
            [&state](const auto& request) {
                tests::component::http::sendRequest(request, state);
            },
            [&state](const auto&) {
                ++state.httpDisconnectedCount;
            });

        tests::component::http::configureHttpRoundTrip(server, client, state);

        server.listen(net::in::SocketAddress("127.0.0.1", 0),
                      [&client, &state](const net::in::SocketAddress& socketAddress, core::socket::State listenState) {
                          if (listenState == core::socket::State::OK) {
                              ++state.listenOkCount;
                              const std::uint16_t effectivePort = socketAddress.getPort();
                              if (effectivePort != 0) {
                                  ++state.effectiveListenEndpointOkCount;
                                  client.connect(net::in::SocketAddress("127.0.0.1", effectivePort),
                                                 [&state](const net::in::SocketAddress&, core::socket::State connectState) {
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
        tests::component::http::expectRoundTrip(testResult, state, "IPv4 legacy", startResult);
        core::SNodeC::free();

        const std::vector<nlohmann::json> records = capture.finish();
        for (const auto& [component, instance] :
             {std::pair{"web.http.server", "ipv4-http-round-trip-server"}, std::pair{"web.http.client", "ipv4-http-round-trip-client"}}) {
            testResult.expectEqual(
                1, capture.count(records, component, instance, "request started:"), "one request start for " + std::string(instance));
            testResult.expectEqual(1,
                                   capture.count(records, component, instance, "request completed:"),
                                   "one request completion for " + std::string(instance));
            testResult.expectEqual(
                0, capture.count(records, component, instance, "request failed:"), "no request failure for " + std::string(instance));
            testResult.expectEqual(
                0, capture.count(records, component, instance, "request aborted:"), "no request abort for " + std::string(instance));
            testResult.expectTrue(capture.matchingIdentityAndLevel(records,
                                                                   component,
                                                                   instance,
                                                                   "request ",
                                                                   "debug",
                                                                   true,
                                                                   std::string_view(instance).ends_with("-server") ? "server" : "client"),
                                  "request lifecycle has matching connection identity and Debug level for " + std::string(instance));
        }
        result = testResult.processResult();
    }

    return result;
}
