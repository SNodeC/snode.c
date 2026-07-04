#include "core/SNodeC.h"
#include "core/socket/State.h"
#include "core/socket/stream/SocketConnection.h"
#include "core/socket/stream/SocketContext.h"
#include "core/socket/stream/SocketContextFactory.h"
#include "net/in/SocketAddress.h"
#include "net/in/stream/legacy/SocketClient.h"
#include "support/TestResult.h"
#include "utils/Timeval.h"
#include "web/http/legacy/in/Server.h"
#include "web/http/server/Request.h"
#include "web/http/server/Response.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace {
    std::string toString(const std::vector<char>& bytes) { return {bytes.begin(), bytes.end()}; }

    constexpr std::string_view chunkedRequest =
        "POST /chunked-request HTTP/1.1\r\n"
        "Host: 127.0.0.1\r\n"
        "Transfer-Encoding: chunked\r\n"
        "Connection: close\r\n"
        "\r\n"
        "6\r\n"
        "hello \r\n"
        "8\r\n"
        "chunked \r\n"
        "7\r\n"
        "request\r\n"
        "0\r\n"
        "\r\n";

    struct State {
        int listenOkCount = 0, effectiveListenEndpointOkCount = 0, clientConnectOkCount = 0;
        int clientFactoryCreateCount = 0, rawClientConnectedCount = 0, rawClientDisconnectedCount = 0;
        int rawClientResponseCount = 0, serverRequestCount = 0, unexpectedStateCount = 0;
        bool serverSawMethod = false, serverSawTarget = false, serverSawBody = false, serverSawTransferEncoding = false;
        bool rawClientSawStatus = false, rawClientSawBody = false;
    };

    class RawClientContext : public core::socket::stream::SocketContext {
    public:
        RawClientContext(core::socket::stream::SocketConnection* socketConnection, State& state)
            : core::socket::stream::SocketContext(socketConnection), state(state) {}

    private:
        void onConnected() override {
            ++state.rawClientConnectedCount;
            sendToPeer(chunkedRequest.data(), chunkedRequest.size());
        }
        void onDisconnected() override { ++state.rawClientDisconnectedCount; }
        std::size_t onReceivedFromPeer() override {
            char chunk[4096];
            const std::size_t chunkLen = readFromPeer(chunk, sizeof(chunk));
            if (chunkLen > 0) {
                responseBuffer.append(chunk, chunkLen);
                if (responseBuffer.find("chunked-request-ok") != std::string::npos) {
                    ++state.rawClientResponseCount;
                    state.rawClientSawStatus = responseBuffer.find("HTTP/1.1 200") != std::string::npos || responseBuffer.find("HTTP/1.0 200") != std::string::npos;
                    state.rawClientSawBody = true;
                    core::SNodeC::stop();
                }
            }
            return chunkLen;
        }
        bool onSignal([[maybe_unused]] int signum) override { return true; }

        State& state;
        std::string responseBuffer;
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

int main(int argc, char* argv[]) {
    tests::support::TestResult testResult;
    int result = tests::support::cTestSkipReturnCode;
    if (tests::support::shouldSkipRootWithoutSNodeCGroup()) {
        tests::support::printRootWithoutSNodeCGroupSkipMessage("InetHttpServerClientChunkedRequestTest");
    } else {
        State state;
        core::SNodeC::init(argc, argv);
        using Server = web::http::legacy::in::Server;
        const Server server("ipv4-http-chunked-request-server", [&state](const auto& request, const auto& response) {
            ++state.serverRequestCount;
            state.serverSawMethod = request->method == "POST";
            state.serverSawTarget = request->url == "/chunked-request";
            state.serverSawBody = toString(request->body) == "hello chunked request";
            state.serverSawTransferEncoding = request->get("Transfer-Encoding") == "chunked";
            response->status(200).type("text/plain").set("Connection", "close").send("chunked-request-ok");
        });
        net::in::stream::legacy::SocketClient<RawClientFactory, State&> client("ipv4-http-raw-chunked-request-client", state);
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
        testResult.expectEqual(0, startResult, "event loop stops after raw chunked request response");
        testResult.expectEqual(1, state.listenOkCount, "listen OK once");
        testResult.expectEqual(1, state.effectiveListenEndpointOkCount, "effective endpoint once");
        testResult.expectEqual(1, state.clientConnectOkCount, "raw client connect OK once");
        testResult.expectEqual(1, state.clientFactoryCreateCount, "raw client factory creates one context");
        testResult.expectEqual(1, state.rawClientConnectedCount, "raw client connected once");
        testResult.expectEqual(1, state.serverRequestCount, "server handles one request");
        testResult.expectEqual(1, state.rawClientResponseCount, "raw client receives one matching response");
        testResult.expectEqual(0, state.unexpectedStateCount, "no unexpected states");
        testResult.expectTrue(state.serverSawMethod, "server sees POST method");
        testResult.expectTrue(state.serverSawTarget, "server sees chunked request target");
        testResult.expectTrue(state.serverSawBody, "server sees reassembled chunked body");
        testResult.expectTrue(state.serverSawTransferEncoding, "server exposes transfer-encoding header");
        testResult.expectTrue(state.rawClientSawStatus, "raw client sees 200 response");
        testResult.expectTrue(state.rawClientSawBody, "raw client sees response body");
        result = testResult.processResult();
        core::SNodeC::free();
    }
    return result;
}
