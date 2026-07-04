#include "core/SNodeC.h"
#include "net/in/SocketAddress.h"
#include "support/TestResult.h"
#include "utils/Timeval.h"
#include "web/http/client/Response.h"
#include "web/http/legacy/in/Client.h"
#include "web/http/legacy/in/Server.h"
#include "web/http/server/Request.h"
#include "web/http/server/Response.h"

#include <string>
#include <vector>

namespace {
    std::string toString(const std::vector<char>& bytes) {
        return {bytes.begin(), bytes.end()};
    }

    struct State {
        int listenOkCount = 0;
        int effectiveListenEndpointOkCount = 0;
        int clientConnectOkCount = 0;
        int httpConnectedCount = 0;
        int serverRequestCount = 0;
        int clientResponseCount = 0;
        int httpDisconnectedCount = 0;
        int unexpectedStateCount = 0;
        int parseErrorCount = 0;
        bool clientSawStatus = false;
        bool clientSawBody = false;
        bool clientSawChunkedHeader = false;
    };
} // namespace

int main(int argc, char* argv[]) {
    tests::support::TestResult testResult;
    int result = tests::support::cTestSkipReturnCode;

    if (tests::support::shouldSkipRootWithoutSNodeCGroup()) {
        tests::support::printRootWithoutSNodeCGroupSkipMessage("InetHttpServerClientChunkedResponseTest");
    } else {
        State state;
        core::SNodeC::init(argc, argv);

        using Server = web::http::legacy::in::Server;
        using Client = web::http::legacy::in::Client;

        const Server server("ipv4-http-chunked-response-server", [&state](const auto& request, const auto& response) {
            ++state.serverRequestCount;
            if (request->url != "/chunked-response") {
                ++state.unexpectedStateCount;
                response->status(404).set("Connection", "close").send("unexpected target");
                return;
            }

            response->status(200)
                .type("text/plain")
                .set("Connection", "close")
                .set("Transfer-Encoding", "chunked")
                .sendHeader()
                .sendFragment("hello ")
                .sendFragment("chunked ")
                .sendFragment("response")
                .sendFragment("");
        });
        Client client(
            "ipv4-http-chunked-response-client",
            [&state](const auto& request) {
                ++state.httpConnectedCount;
                request->method = "GET";
                request->url = "/chunked-response";
                request->set("Connection", "close");
                if (!request->end(
                        [&state](const auto&, const auto& response) {
                            ++state.clientResponseCount;
                            state.clientSawStatus = response->statusCode == "200";
                            state.clientSawBody = toString(response->body) == "hello chunked response";
                            state.clientSawChunkedHeader = response->get("Transfer-Encoding") == "chunked";
                            core::SNodeC::stop();
                        },
                        [&state](const auto&, const std::string&) {
                            ++state.parseErrorCount;
                            core::SNodeC::stop();
                        })) {
                    ++state.unexpectedStateCount;
                    core::SNodeC::stop();
                }
            },
            [&state](const auto&) {
                ++state.httpDisconnectedCount;
            });
        server.getConfig()->Instance::forceUnrequired();
        client.getConfig()->Instance::forceUnrequired();

        server.listen(net::in::SocketAddress("127.0.0.1", 0), [&client, &state](const auto& socketAddress, core::socket::State listenState) {
            if (listenState == core::socket::State::OK) {
                ++state.listenOkCount;
                const std::uint16_t effectivePort = socketAddress.getPort();
                if (effectivePort != 0) {
                    ++state.effectiveListenEndpointOkCount;
                    client.connect(net::in::SocketAddress("127.0.0.1", effectivePort), [&state](const auto&, core::socket::State connectState) {
                        if (connectState == core::socket::State::OK) {
                            ++state.clientConnectOkCount;
                        } else {
                            ++state.unexpectedStateCount;
                            core::SNodeC::stop();
                        }
                    });
                } else {
                    ++state.unexpectedStateCount;
                    core::SNodeC::stop();
                }
            } else {
                ++state.unexpectedStateCount;
                core::SNodeC::stop();
            }
        });

        const int startResult = core::SNodeC::start(utils::Timeval({1, 0}));
        testResult.expectEqual(0, startResult, "event loop stops successfully after IPv4 legacy HTTP chunked response");
        testResult.expectEqual(1, state.listenOkCount, "listen OK once");
        testResult.expectEqual(1, state.effectiveListenEndpointOkCount, "effective endpoint once");
        testResult.expectEqual(1, state.clientConnectOkCount, "connect OK once");
        testResult.expectEqual(1, state.httpConnectedCount, "HTTP connected once");
        testResult.expectEqual(1, state.serverRequestCount, "server handler once");
        testResult.expectEqual(1, state.clientResponseCount, "client response once");
        testResult.expectEqual(0, state.parseErrorCount, "no response parse errors");
        testResult.expectEqual(0, state.unexpectedStateCount, "no unexpected states");
        testResult.expectTrue(state.clientSawStatus, "client sees 200");
        testResult.expectTrue(state.clientSawBody, "client sees reassembled chunked response body");
        testResult.expectTrue(state.clientSawChunkedHeader, "client sees chunked transfer-encoding header");
        result = testResult.processResult();
        core::SNodeC::free();
    }

    return result;
}
