#include "HttpServerClientRoundTripTest.h"

#include "net/in6/SocketAddress.h"
#include "web/http/legacy/in6/Client.h"
#include "web/http/legacy/in6/Server.h"

int main(int argc, char* argv[]) {
    tests::support::TestResult testResult;
    int result = tests::support::cTestSkipReturnCode;

    if (tests::support::shouldSkipRootWithoutSNodeCGroup()) {
        tests::support::printRootWithoutSNodeCGroupSkipMessage("Inet6HttpServerClientGetRoundTripTest");
    } else {
        tests::component::http::RoundTripState state;

        core::SNodeC::init(argc, argv);

        using Server = web::http::legacy::in6::Server;
        using Client = web::http::legacy::in6::Client;

        const Server server("ipv6-http-round-trip-server", [&state](const auto& request, const auto& response) {
            tests::component::http::handleRequest(request, response, state);
        });
        Client client(
            "ipv6-http-round-trip-client",
            [&state](const auto& request) {
                tests::component::http::sendRequest(request, state);
            },
            [&state](const auto&) {
                ++state.httpDisconnectedCount;
            });

        tests::component::http::configureHttpRoundTrip(server, client, state);

        server.listen(net::in6::SocketAddress("::1", 0),
                      [&client, &state](const net::in6::SocketAddress& socketAddress, core::socket::State listenState) {
                          if (listenState == core::socket::State::OK) {
                              ++state.listenOkCount;
                              const std::uint16_t effectivePort = socketAddress.getPort();
                              if (effectivePort != 0) {
                                  ++state.effectiveListenEndpointOkCount;
                                  client.connect(net::in6::SocketAddress("::1", effectivePort),
                                                 [&state](const net::in6::SocketAddress&, core::socket::State connectState) {
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
        tests::component::http::expectRoundTrip(testResult, state, "IPv6 legacy", startResult);
        result = testResult.processResult();
        core::SNodeC::free();
    }

    return result;
}
