#include "core/SNodeC.h"
#include "core/socket/State.h"
#include "core/socket/stream/SocketConnection.h"
#include "core/socket/stream/SocketContext.h"
#include "core/socket/stream/SocketContextFactory.h"
#include "net/in/SocketAddress.h"
#include "net/in/stream/legacy/SocketClient.h"
#include "support/Phase3SemanticLogCapture.h"
#include "support/TestResult.h"
#include "utils/Timeval.h"
#include "web/http/legacy/in/Server.h"
#include "web/http/server/Response.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

namespace {
    constexpr std::string_view malformedRequest =
        "GET /malformed HTTP/1.1\r\n"
        "Host: 127.0.0.1\r\n"
        "BadHeaderWithoutColon\r\n"
        "Connection: close\r\n"
        "\r\n";

    struct State {
        int listenOkCount = 0, effectiveListenEndpointOkCount = 0, clientConnectOkCount = 0;
        int clientFactoryCreateCount = 0, rawClientConnectedCount = 0, rawClientDisconnectedCount = 0;
        int serverRequestCount = 0, rawClientReadCount = 0, unexpectedStateCount = 0;
        std::string responseBuffer;
    };

    class RawClientContext : public core::socket::stream::SocketContext {
    public:
        RawClientContext(core::socket::stream::SocketConnection* socketConnection, State& state)
            : core::socket::stream::SocketContext(socketConnection), state(state) {}
    private:
        void onConnected() override {
            ++state.rawClientConnectedCount;
            sendToPeer(malformedRequest.data(), malformedRequest.size());
        }
        void onDisconnected() override {
            ++state.rawClientDisconnectedCount;
            core::SNodeC::stop();
        }
        std::size_t onReceivedFromPeer() override {
            char chunk[4096];
            const std::size_t chunkLen = readFromPeer(chunk, sizeof(chunk));
            if (chunkLen > 0) {
                ++state.rawClientReadCount;
                state.responseBuffer.append(chunk, chunkLen);
                if (state.responseBuffer.find("\r\n\r\n") != std::string::npos) {
                    core::SNodeC::stop();
                }
            }
            return chunkLen;
        }
        bool onSignal([[maybe_unused]] int signum) override { return true; }
        State& state;
    };

    class RawClientFactory : public core::socket::stream::SocketContextFactory {
    public:
        explicit RawClientFactory(State& state) : state(state) {}
        core::socket::stream::SocketContext* create(core::socket::stream::SocketConnection* socketConnection) override {
            ++state.clientFactoryCreateCount;
            return new RawClientContext(socketConnection, state);
        }
    private:
        State& state;
    };
}

int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[]) {
    tests::support::TestResult testResult;
    int result = tests::support::cTestSkipReturnCode;
    if (tests::support::shouldSkipRootWithoutSNodeCGroup()) {
        tests::support::printRootWithoutSNodeCGroupSkipMessage("InetHttpServerMalformedRequestBehaviorTest");
    } else {
        State state;
        tests::support::Phase3SemanticLogCapture capture("snodec-phase3-http-malformed-request");
        capture.initCore("InetHttpServerMalformedRequestBehaviorTest");
        using Server = web::http::legacy::in::Server;
        const Server server("ipv4-http-malformed-request-server", [&state](const auto&, const auto& response) {
            ++state.serverRequestCount;
            response->status(200).set("Connection", "close").send("unexpected-normal-handler");
        });
        net::in::stream::legacy::SocketClient<RawClientFactory, State&> client("ipv4-http-raw-malformed-request-client", state);
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
        const bool sawBadRequest = state.responseBuffer.find("HTTP/1.1 400") != std::string::npos || state.responseBuffer.find("HTTP/1.0 400") != std::string::npos;
        const bool didNotReturnNormalOk = state.responseBuffer.find("unexpected-normal-handler") == std::string::npos;
        testResult.expectEqual(0, startResult, "event loop stops after malformed request is rejected");
        testResult.expectEqual(1, state.listenOkCount, "listen OK once");
        testResult.expectEqual(1, state.effectiveListenEndpointOkCount, "effective endpoint once");
        testResult.expectEqual(1, state.clientConnectOkCount, "raw client connect OK once");
        testResult.expectEqual(1, state.clientFactoryCreateCount, "raw client factory creates one context");
        testResult.expectEqual(1, state.rawClientConnectedCount, "raw client connected once");
        testResult.expectEqual(0, state.serverRequestCount, "malformed request does not reach normal handler");
        testResult.expectEqual(0, state.unexpectedStateCount, "no unexpected states");
        testResult.expectTrue(sawBadRequest || (state.rawClientDisconnectedCount == 1 && didNotReturnNormalOk), "server either returns 400 or closes without normal 200 handler response");
        core::SNodeC::free();
        const auto records = capture.finish();
        constexpr std::string_view component = "web.http.server";
        constexpr std::string_view instance = "ipv4-http-malformed-request-server";
        testResult.expectEqual(1, capture.count(records, component, instance, "request started:"), "malformed request starts once");
        testResult.expectEqual(1, capture.count(records, component, instance, "request failed:"), "malformed request fails once");
        testResult.expectEqual(0, capture.count(records, component, instance, "request completed:"), "malformed request never completes");
        testResult.expectEqual(
            0, capture.count(records, component, instance, "request aborted:"), "malformed request failure is not an abort");
        testResult.expectTrue(capture.matchingIdentityAndLevel(records, component, instance, "request ", "debug", true, "server"),
                              "malformed request lifecycle uses matching connection identity and Debug level");
        result = testResult.processResult();
    }
    return result;
}
