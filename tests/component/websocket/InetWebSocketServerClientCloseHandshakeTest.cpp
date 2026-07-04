#include "InetWebSocketBehaviorRunner.h"

int main(int argc, char* argv[]) {
    int result = tests::support::cTestSkipReturnCode;
    if (tests::support::shouldSkipRootWithoutSNodeCGroup()) {
        tests::support::printRootWithoutSNodeCGroupSkipMessage("InetWebSocketServerClientCloseHandshakeTest");
    } else {
        tests::component::websocket::behavior::State state{tests::component::websocket::behavior::Scenario::CloseHandshake};
        result = tests::component::websocket::behavior::runInetBehaviorTest(
            argc,
            argv,
            "ipv4-websocket-close-handshake-server",
            "ipv4-websocket-close-handshake-client",
            state,
            [](tests::support::TestResult& testResult, const tests::component::websocket::behavior::State& state, int startResult) {
                using namespace tests::component::websocket::behavior;
                expectCommon(testResult, state, "IPv4 legacy WebSocket close handshake", startResult);
                testResult.expectEqual(0, state.serverMessageStartCount, "server observes no application message starts during explicit close");
                testResult.expectEqual(0, state.serverMessageEndCount, "server observes no application message ends during explicit close");
                testResult.expectEqual(0, state.clientMessageStartCount, "client observes no application message starts during explicit close");
                testResult.expectEqual(0, state.clientMessageEndCount, "client observes no application message ends during explicit close");
                testResult.expectEqual(1, state.serverWsDisconnectedCount, "server observes disconnect once after close handshake");
            });
    }
    return result;
}
