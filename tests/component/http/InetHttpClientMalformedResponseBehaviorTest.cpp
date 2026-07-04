#include "core/SNodeC.h"
#include "core/socket/State.h"
#include "core/socket/stream/SocketConnection.h"
#include "core/socket/stream/SocketContext.h"
#include "core/socket/stream/SocketContextFactory.h"
#include "net/in/SocketAddress.h"
#include "net/in/stream/legacy/SocketServer.h"
#include "support/TestResult.h"
#include "utils/Timeval.h"
#include "web/http/legacy/in/Client.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

namespace {
    constexpr std::string_view malformedResponse =
        "HTTP/1.1 200 OK\r\n"
        "Content-Length: not-a-number\r\n"
        "Connection: close\r\n"
        "\r\n";

    struct State {
        int listenOkCount = 0, effectiveListenEndpointOkCount = 0, clientConnectOkCount = 0;
        int serverFactoryCreateCount = 0, rawServerConnectedCount = 0, rawServerDisconnectedCount = 0;
        int rawServerRequestReadCount = 0, httpConnectedCount = 0, httpDisconnectedCount = 0;
        int clientResponseCount = 0, parseErrorCount = 0, unexpectedStateCount = 0;
        bool rawServerSawGet = false;
        std::string parseErrorMessage;
    };

    class RawServerContext : public core::socket::stream::SocketContext {
    public:
        RawServerContext(core::socket::stream::SocketConnection* socketConnection, State& state)
            : core::socket::stream::SocketContext(socketConnection), state(state) {}
    private:
        void onConnected() override { ++state.rawServerConnectedCount; }
        void onDisconnected() override { ++state.rawServerDisconnectedCount; }
        std::size_t onReceivedFromPeer() override {
            char chunk[4096];
            const std::size_t chunkLen = readFromPeer(chunk, sizeof(chunk));
            if (chunkLen > 0) {
                requestBuffer.append(chunk, chunkLen);
                if (requestBuffer.find("\r\n\r\n") != std::string::npos) {
                    ++state.rawServerRequestReadCount;
                    state.rawServerSawGet = requestBuffer.find("GET /malformed-response HTTP/1.1") != std::string::npos;
                    sendToPeer(malformedResponse.data(), malformedResponse.size());
                    shutdownWrite();
                }
            }
            return chunkLen;
        }
        bool onSignal([[maybe_unused]] int signum) override { return true; }
        State& state;
        std::string requestBuffer;
    };

    class RawServerFactory : public core::socket::stream::SocketContextFactory {
    public:
        explicit RawServerFactory(State& state) : state(state) {}
        core::socket::stream::SocketContext* create(core::socket::stream::SocketConnection* socketConnection) override {
            ++state.serverFactoryCreateCount;
            return new RawServerContext(socketConnection, state);
        }
    private:
        State& state;
    };
}

int main(int argc, char* argv[]) {
    tests::support::TestResult testResult;
    int result = tests::support::cTestSkipReturnCode;
    if (tests::support::shouldSkipRootWithoutSNodeCGroup()) {
        tests::support::printRootWithoutSNodeCGroupSkipMessage("InetHttpClientMalformedResponseBehaviorTest");
    } else {
        State state;
        core::SNodeC::init(argc, argv);
        net::in::stream::legacy::SocketServer<RawServerFactory, State&> server("ipv4-http-raw-malformed-response-server", state);
        web::http::legacy::in::Client client(
            "ipv4-http-malformed-response-client",
            [&state](const auto& request) {
                ++state.httpConnectedCount;
                request->method = "GET";
                request->url = "/malformed-response";
                request->set("Connection", "close");
                if (!request->end(
                        [&state](const auto&, const auto&) {
                            ++state.clientResponseCount;
                            core::SNodeC::stop();
                        },
                        [&state](const auto&, const std::string& message) {
                            ++state.parseErrorCount;
                            state.parseErrorMessage = message;
                            core::SNodeC::stop();
                        })) {
                    ++state.unexpectedStateCount;
                    core::SNodeC::stop();
                }
            },
            [&state](const auto&) { ++state.httpDisconnectedCount; });
        server.getConfig()->Instance::forceUnrequired();
        client.getConfig()->Instance::forceUnrequired();
        server.listen(net::in::SocketAddress("127.0.0.1", 0), [&client, &state](const auto& socketAddress, core::socket::State listenState) {
            if (listenState == core::socket::State::OK) {
                ++state.listenOkCount;
                const std::uint16_t effectivePort = socketAddress.getPort();
                if (effectivePort != 0) {
                    ++state.effectiveListenEndpointOkCount;
                    client.connect(net::in::SocketAddress("127.0.0.1", effectivePort), [&state](const auto&, core::socket::State connectState) {
                        if (connectState == core::socket::State::OK) ++state.clientConnectOkCount; else { ++state.unexpectedStateCount; core::SNodeC::stop(); }
                    });
                } else { ++state.unexpectedStateCount; core::SNodeC::stop(); }
            } else { ++state.unexpectedStateCount; core::SNodeC::stop(); }
        });
        const int startResult = core::SNodeC::start(utils::Timeval({1, 0}));
        testResult.expectEqual(0, startResult, "event loop stops after malformed response parse error");
        testResult.expectEqual(1, state.listenOkCount, "listen OK once");
        testResult.expectEqual(1, state.effectiveListenEndpointOkCount, "effective endpoint once");
        testResult.expectEqual(1, state.clientConnectOkCount, "HTTP client connect OK once");
        testResult.expectEqual(1, state.serverFactoryCreateCount, "raw server factory creates one context");
        testResult.expectEqual(1, state.rawServerConnectedCount, "raw server connected once");
        testResult.expectEqual(1, state.rawServerRequestReadCount, "raw server reads one HTTP request");
        testResult.expectEqual(1, state.httpConnectedCount, "HTTP connected callback once");
        testResult.expectEqual(1, state.parseErrorCount, "client reports one response parse error");
        testResult.expectEqual(0, state.clientResponseCount, "client does not report successful response");
        testResult.expectEqual(0, state.unexpectedStateCount, "no unexpected states");
        testResult.expectTrue(state.rawServerSawGet, "raw server sees client GET request");
        testResult.expectTrue(!state.parseErrorMessage.empty(), "parse error provides a message");
        result = testResult.processResult();
        core::SNodeC::free();
    }
    return result;
}
