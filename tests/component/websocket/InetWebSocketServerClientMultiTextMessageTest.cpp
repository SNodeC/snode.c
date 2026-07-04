#include "InetWebSocketBehaviorRunner.h"

int main(int argc, char* argv[]) {
    int result = tests::support::cTestSkipReturnCode;
    if (tests::support::shouldSkipRootWithoutSNodeCGroup()) {
        tests::support::printRootWithoutSNodeCGroupSkipMessage("InetWebSocketServerClientMultiTextMessageTest");
    } else {
        tests::component::websocket::behavior::State state{tests::component::websocket::behavior::Scenario::MultiText};
        result = tests::component::websocket::behavior::runInetBehaviorTest(
            argc,
            argv,
            "ipv4-websocket-multi-text-server",
            "ipv4-websocket-multi-text-client",
            state,
            [](tests::support::TestResult& testResult, const tests::component::websocket::behavior::State& state, int startResult) {
                using namespace tests::component::websocket::behavior;
                expectCommon(testResult, state, "IPv4 legacy WebSocket multi-text", startResult);
                testResult.expectEqual(3, state.serverMessageStartCount, "server observes three text message starts");
                testResult.expectEqual(3, state.serverMessageEndCount, "server observes three text message ends");
                testResult.expectEqual(3, state.clientMessageStartCount, "client observes three text message starts");
                testResult.expectEqual(3, state.clientMessageEndCount, "client observes three text message ends");
                testResult.expectEqual(3, state.serverTextOpcodeCount, "server observes text opcode for each message");
                testResult.expectEqual(3, state.clientTextOpcodeCount, "client observes text opcode for each reply");
                testResult.expectTrue(state.serverMessages == textMessages, "server receives exact text message sequence");
                testResult.expectTrue(state.clientMessages == textReplies, "client receives exact text reply sequence");
            });
    }
    return result;
}
