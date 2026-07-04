#include "InetWebSocketBehaviorRunner.h"

int main(int argc, char* argv[]) {
    int result = tests::support::cTestSkipReturnCode;
    if (tests::support::shouldSkipRootWithoutSNodeCGroup()) {
        tests::support::printRootWithoutSNodeCGroupSkipMessage("InetWebSocketServerClientBinaryEchoTest");
    } else {
        tests::component::websocket::behavior::State state{tests::component::websocket::behavior::Scenario::BinaryEcho};
        result = tests::component::websocket::behavior::runInetBehaviorTest(
            argc,
            argv,
            "ipv4-websocket-binary-echo-server",
            "ipv4-websocket-binary-echo-client",
            state,
            [](tests::support::TestResult& testResult, const tests::component::websocket::behavior::State& state, int startResult) {
                using namespace tests::component::websocket::behavior;
                expectCommon(testResult, state, "IPv4 legacy WebSocket binary echo", startResult);
                testResult.expectEqual(1, state.serverMessageStartCount, "server observes one binary message start");
                testResult.expectEqual(1, state.serverMessageEndCount, "server observes one binary message end");
                testResult.expectEqual(1, state.clientMessageStartCount, "client observes one binary message start");
                testResult.expectEqual(1, state.clientMessageEndCount, "client observes one binary message end");
                testResult.expectEqual(1, state.serverBinaryOpcodeCount, "server observes binary opcode");
                testResult.expectEqual(1, state.clientBinaryOpcodeCount, "client observes binary opcode");
                testResult.expectTrue(state.serverBinary == binaryRequest, "server receives exact binary request bytes");
                testResult.expectTrue(state.clientBinary == binaryReply, "client receives exact binary reply bytes");
            });
    }
    return result;
}
