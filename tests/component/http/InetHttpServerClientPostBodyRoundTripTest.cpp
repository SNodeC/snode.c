#include "HttpServerClientBodyStatusTest.h"

#include "net/in/SocketAddress.h"
#include "web/http/legacy/in/Client.h"
#include "web/http/legacy/in/Server.h"

int main(int argc, char* argv[]) {
    tests::support::TestResult testResult;
    int result = tests::support::cTestSkipReturnCode;

    if (tests::support::shouldSkipRootWithoutSNodeCGroup()) {
        tests::support::printRootWithoutSNodeCGroupSkipMessage("InetHttpServerClientPostBodyRoundTripTest");
    } else {
        tests::component::http::BodyStatusState state;

        core::SNodeC::init(argc, argv);

        using Server = web::http::legacy::in::Server;
        using Client = web::http::legacy::in::Client;

        const Server server("ipv4-http-postbody-server", [&state](const auto& request, const auto& response) {
            tests::component::http::handlePostBodyRequest(request, response, state);
        });
        Client client(
            "ipv4-http-postbody-client",
            [&state](const auto& request) {
                tests::component::http::sendPostBodyRequest(request, state);
            },
            [&state](const auto&) {
                ++state.httpDisconnectedCount;
            });

        tests::component::http::configureHttpBodyStatusTest(server, client, state);

        server.listen(net::in::SocketAddress("127.0.0.1", 0),
                      [&client, &state](const auto& socketAddress, core::socket::State listenState) {
                          if (listenState == core::socket::State::OK) {
                              ++state.listenOkCount;
                              const std::uint16_t effectivePort = socketAddress.getPort();
                              if (effectivePort != 0) {
                                  ++state.effectiveListenEndpointOkCount;
                                  client.connect(net::in::SocketAddress("127.0.0.1", effectivePort),
                                                 [&state](const auto&, core::socket::State connectState) {
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
        tests::component::http::expectPostBodyRoundTrip(testResult, state, "IPv4 legacy", startResult);
        result = testResult.processResult();
        core::SNodeC::free();
    }

    return result;
}
