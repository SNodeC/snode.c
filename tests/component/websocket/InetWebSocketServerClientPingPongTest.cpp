#include "InetWebSocketBehaviorRunner.h"

int main(int argc, char* argv[]) {
    int result = tests::support::cTestSkipReturnCode;
    if (tests::support::shouldSkipRootWithoutSNodeCGroup()) {
        tests::support::printRootWithoutSNodeCGroupSkipMessage("InetWebSocketServerClientPingPongTest");
    } else {
        tests::component::websocket::behavior::State state{tests::component::websocket::behavior::Scenario::PingPong};
        result = tests::component::websocket::behavior::runInetBehaviorTest(
            argc,
            argv,
            "ipv4-websocket-ping-pong-server",
            "ipv4-websocket-ping-pong-client",
            state,
            [](tests::support::TestResult& testResult, const tests::component::websocket::behavior::State& state, int startResult) {
                using namespace tests::component::websocket::behavior;
                expectCommon(testResult, state, "IPv4 legacy WebSocket ping/pong", startResult);
                testResult.expectEqual(0, state.serverMessageStartCount, "server observes no application message starts during ping/pong");
                testResult.expectEqual(0, state.serverMessageEndCount, "server observes no application message ends during ping/pong");
                testResult.expectEqual(0, state.clientMessageStartCount, "client observes no application message starts during ping/pong");
                testResult.expectEqual(0, state.clientMessageEndCount, "client observes no application message ends during ping/pong");
                testResult.expectEqual(1, state.clientPongReceivedCount, "client observes one pong response");
            });
    }
    return result;
}
