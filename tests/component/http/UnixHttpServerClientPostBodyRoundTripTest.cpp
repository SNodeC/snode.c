#include "HttpServerClientBodyStatusTest.h"

#include "web/http/legacy/un/Client.h"
#include "web/http/legacy/un/Server.h"

#include <filesystem>
#include <string>
#include <unistd.h>

namespace {

    std::string makeSocketPath() {
        return "/tmp/snodec-http-component-postbody-" + std::to_string(::getpid()) + ".sock";
    }

} // namespace

int main(int argc, char* argv[]) {
    tests::support::TestResult testResult;
    int result = tests::support::cTestSkipReturnCode;

    if (tests::support::shouldSkipRootWithoutSNodeCGroup()) {
        tests::support::printRootWithoutSNodeCGroupSkipMessage("UnixHttpServerClientPostBodyRoundTripTest");
    } else {
        tests::component::http::BodyStatusState state;
        const std::string socketPath = makeSocketPath();
        std::filesystem::remove(socketPath);

        core::SNodeC::init(argc, argv);

        using Server = web::http::legacy::un::Server;
        using Client = web::http::legacy::un::Client;

        const Server server("unix-http-postbody-server", [&state](const auto& request, const auto& response) {
            tests::component::http::handlePostBodyRequest(request, response, state);
        });
        Client client(
            "unix-http-postbody-client",
            [&state](const auto& request) {
                tests::component::http::sendPostBodyRequest(request, state);
            },
            [&state](const auto&) {
                ++state.httpDisconnectedCount;
            });

        tests::component::http::configureHttpBodyStatusTest(server, client, state);

        server.listen(socketPath, [&client, &socketPath, &state](const auto&, core::socket::State listenState) {
            if (listenState == core::socket::State::OK) {
                ++state.listenOkCount;
                ++state.effectiveListenEndpointOkCount;
                client.connect(socketPath, [&state](const auto&, core::socket::State connectState) {
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
        });

        const int startResult = core::SNodeC::start(utils::Timeval({1, 0}));
        tests::component::http::expectPostBodyRoundTrip(testResult, state, "Unix-domain legacy", startResult);
        result = testResult.processResult();
        core::SNodeC::free();
        std::filesystem::remove(socketPath);
    }

    return result;
}
