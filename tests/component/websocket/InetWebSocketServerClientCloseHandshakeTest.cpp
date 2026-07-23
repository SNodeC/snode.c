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
                testResult.expectEqual(
                    0, state.serverMessageStartCount, "server observes no application message starts during explicit close");
                testResult.expectEqual(0, state.serverMessageEndCount, "server observes no application message ends during explicit close");
                testResult.expectEqual(
                    0, state.clientMessageStartCount, "client observes no application message starts during explicit close");
                testResult.expectEqual(0, state.clientMessageEndCount, "client observes no application message ends during explicit close");
                testResult.expectEqual(1, state.serverWsDisconnectedCount, "server observes disconnect once after close handshake");
            },
            [&capture](tests::support::TestResult& testResult) {
                const auto records = capture.finish();
                bool serverInitiatingUpgrade = false;
                bool serverFactorySelection = false;
                bool serverUpgradeBootstrap = false;
                for (const auto& record : records) {
                    const std::string component = record.value("component", "");
                    const std::string instance = record.value("instance", "");
                    const std::string message = record.value("message", "");
                    const bool serverUpgradeIdentity =
                        record.value("origin", "") == "framework" && record.value("boundary", "") == "connection" &&
                        component == "web.http.server" && record.value("role", "") == "server" &&
                        instance == "ipv4-websocket-close-handshake-server" && !record.value("connection", "").empty();

                    if (message.starts_with("Initiating upgrade:")) {
                        serverInitiatingUpgrade =
                            serverInitiatingUpgrade ||
                            (serverUpgradeIdentity && message.find("GET /component/websocket/behavior HTTP/1.1") != std::string::npos &&
                             message.find("websocket") != std::string::npos &&
                             message.find("ipv4-websocket-close-handshake-server") == std::string::npos &&
                             message.find("HTTP:") == std::string::npos && message.find("HTTP upgrade:") == std::string::npos);
                    } else if (message.starts_with("SocketContextUpgradeFactory create success for:")) {
                        serverFactorySelection =
                            serverFactorySelection || (serverUpgradeIdentity && message.find("websocket") != std::string::npos);
                    } else if (message.starts_with("Upgrade bootstrap")) {
                        serverUpgradeBootstrap =
                            serverUpgradeBootstrap || (serverUpgradeIdentity && message.find("success") != std::string::npos);
                    }
                }
                testResult.expectTrue(serverInitiatingUpgrade,
                                      "HTTP server upgrade presentation uses structured connection identity and retains request data");
                testResult.expectTrue(serverFactorySelection,
                                      "HTTP server upgrade factory diagnostic retains selected protocol on the bound scope");
                testResult.expectTrue(serverUpgradeBootstrap, "HTTP server upgrade bootstrap diagnostic uses the bound server scope");

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
                    testResult.expectEqual(1,
                                           capture.count(records, "web.websocket.subprotocol", instance, "       Total Payload sent:"),
                                           "subprotocol sent-payload summary is connection-bound for " + std::string(instance));
                    testResult.expectEqual(
                        1,
                        capture.count(records, "web.websocket.subprotocol", instance, "  Total Payload processed:"),
                        "subprotocol processed-payload summary is connection-bound for " + std::string(instance));
                    testResult.expectTrue(
                        capture.matchingIdentityAndLevel(records, "web.websocket.subprotocol", instance, "websocket ", "info"),
                        "WebSocket lifecycle has matching connection identity and Info level for " + std::string(instance));
                    testResult.expectTrue(
                        capture.matchingIdentityAndLevel(
                            records, "web.websocket.subprotocol", instance, "       Total Payload sent:", "debug"),
                        "subprotocol sent-payload summary uses connection identity and Debug level for " + std::string(instance));
                    testResult.expectTrue(
                        capture.matchingIdentityAndLevel(
                            records, "web.websocket.subprotocol", instance, "  Total Payload processed:", "debug"),
                        "subprotocol processed-payload summary uses connection identity and Debug level for " +
                            std::string(instance));
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
