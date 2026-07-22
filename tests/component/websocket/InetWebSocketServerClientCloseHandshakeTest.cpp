#include "InetWebSocketBehaviorRunner.h"
#include "support/Phase3SemanticLogCapture.h"

#include <string>
#include <utility>

int main(int argc, char* argv[]) {
    int result = tests::support::cTestSkipReturnCode;
    if (tests::support::shouldSkipRootWithoutSNodeCGroup()) {
        tests::support::printRootWithoutSNodeCGroupSkipMessage("InetWebSocketServerClientCloseHandshakeTest");
    } else {
        tests::support::Phase3SemanticLogCapture capture("snodec-phase3-websocket-close-handshake");
        char testName[] = "InetWebSocketServerClientCloseHandshakeTest";
        char logLevel[] = "--log-level=6";
        char logFormat[] = "--log-format=json";
        char quiet[] = "--quiet";
        char* logArgs[] = {testName, logLevel, logFormat, quiet, nullptr};
        tests::component::websocket::behavior::State state{tests::component::websocket::behavior::Scenario::CloseHandshake};
        result = tests::component::websocket::behavior::runInetBehaviorTest(
            4,
            logArgs,
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
            },
            [&capture](tests::support::TestResult& testResult) {
                const auto records = capture.finish();
                for (const auto& [instance, httpComponent] : {std::pair{"ipv4-websocket-close-handshake-server", "web.http.server"},
                                                              std::pair{"ipv4-websocket-close-handshake-client", "web.http.client"}}) {
                    testResult.expectEqual(1,
                                           capture.count(records, "web.websocket.subprotocol", instance, "websocket established:"),
                                           "WebSocket establishes once for " + std::string(instance));
                    testResult.expectEqual(1,
                                           capture.count(records, "web.websocket.subprotocol", instance, "websocket ended:"),
                                           "WebSocket ends once for " + std::string(instance));
                    testResult.expectEqual(1,
                                           capture.count(records, "web.websocket.subprotocol", instance, "subprotocol started:"),
                                           "subprotocol starts once for " + std::string(instance));
                    testResult.expectEqual(1,
                                           capture.count(records, "web.websocket.subprotocol", instance, "subprotocol stopped:"),
                                           "subprotocol stops once for " + std::string(instance));
                    testResult.expectTrue(
                        capture.matchingIdentityAndLevel(records, "web.websocket.subprotocol", instance, "websocket ", "info"),
                        "WebSocket lifecycle has matching connection identity and Info level for " + std::string(instance));
                    testResult.expectEqual(1,
                                           capture.count(records, httpComponent, instance, "request completed:"),
                                           "HTTP upgrade has one completion for " + std::string(instance));
                }
            });
    }
    static_cast<void>(argc);
    static_cast<void>(argv);
    return result;
}
