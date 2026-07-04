#ifndef TESTS_COMPONENT_WEBSOCKET_INETWEBSOCKETBEHAVIORRUNNER_H
#define TESTS_COMPONENT_WEBSOCKET_INETWEBSOCKETBEHAVIORRUNNER_H

#include "WebSocketServerClientBehaviorTest.h"

#include "core/SNodeC.h"
#include "net/in/SocketAddress.h"
#include "utils/Timeval.h"
#include "web/http/legacy/in/Client.h"
#include "web/http/legacy/in/Server.h"

namespace tests::component::websocket::behavior {

    template <typename Expect>
    int runInetBehaviorTest(int argc, char* argv[], const char* serverName, const char* clientName, State& state, Expect expect) {
        tests::support::TestResult testResult;
        core::SNodeC::init(argc, argv);

        using Server = web::http::legacy::in::Server;
        using Client = web::http::legacy::in::Client;

        const Server server(serverName, [&state](const auto& request, const auto& response) { handleUpgradeRequest(request, response, state); });
        Client client(clientName, [&state](const auto& request) { startUpgrade(request, state); }, [&state](const auto&) { ++state.httpDisconnectedCount; });

        configure(server, client, state);
        server.listen(net::in::SocketAddress("127.0.0.1", 0), [&client, &state](const net::in::SocketAddress& socketAddress, core::socket::State listenState) {
            if (listenState == core::socket::State::OK) {
                ++state.listenOkCount;
                const std::uint16_t effectivePort = socketAddress.getPort();
                if (effectivePort != 0) {
                    ++state.effectiveListenEndpointOkCount;
                    client.connect(net::in::SocketAddress("127.0.0.1", effectivePort), [&state](const net::in::SocketAddress&, core::socket::State connectState) {
                        if (connectState == core::socket::State::OK) ++state.clientConnectOkCount;
                        else { ++state.unexpectedStateCount; core::SNodeC::stop(); }
                    });
                } else { ++state.unexpectedStateCount; core::SNodeC::stop(); }
            } else { ++state.unexpectedStateCount; core::SNodeC::stop(); }
        });

        const int startResult = core::SNodeC::start(utils::Timeval({1, 0}));
        expect(testResult, state, startResult);
        const int result = testResult.processResult();
        core::SNodeC::free();
        return result;
    }

} // namespace tests::component::websocket::behavior

#endif
