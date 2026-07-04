#include "WebSocketServerClientTextEchoTest.h"

#include "web/http/legacy/un/Client.h"
#include "web/http/legacy/un/Server.h"

#include <filesystem>
#include <string>
#include <unistd.h>

namespace {

    std::string makeSocketPath() {
        return "/tmp/snodec-websocket-component-" + std::to_string(::getpid()) + ".sock";
    }

} // namespace

int main(int argc, char* argv[]) {
    tests::support::TestResult testResult;
    int result = tests::support::cTestSkipReturnCode;

    if (tests::support::shouldSkipRootWithoutSNodeCGroup()) {
        tests::support::printRootWithoutSNodeCGroupSkipMessage("UnixWebSocketServerClientTextEchoTest");
    } else {
        tests::component::websocket::EchoState state;
        const std::string socketPath = makeSocketPath();
        std::filesystem::remove(socketPath);

        core::SNodeC::init(argc, argv);

        using Server = web::http::legacy::un::Server;
        using Client = web::http::legacy::un::Client;

        const Server server("unix-websocket-echo-server", [&state](const auto& request, const auto& response) {
            tests::component::websocket::handleUpgradeRequest(request, response, state);
        });
        Client client(
            "unix-websocket-echo-client",
            [&state](const auto& request) {
                tests::component::websocket::startUpgrade(request, state);
            },
            [&state](const auto&) {
                ++state.httpDisconnectedCount;
            });

        tests::component::websocket::configureWebSocketEcho(server, client, state);

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
        tests::component::websocket::expectTextEcho(testResult, state, "Unix-domain legacy", startResult);
        result = testResult.processResult();
        core::SNodeC::free();
        std::filesystem::remove(socketPath);
    }

    return result;
}
