#include "core/SNodeC.h"
#include "net/in/SocketAddress.h"
#include "support/TestResult.h"
#include "utils/Timeval.h"
#include "web/http/client/Response.h"
#include "web/http/legacy/in/Client.h"
#include "web/http/legacy/in/Server.h"
#include "web/http/server/Request.h"
#include "web/http/server/Response.h"

#include <iomanip>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

namespace {
    constexpr int headerCount = 32;
    std::string headerName(int i) {
        std::ostringstream os;
        os << "X-Test-Header-" << std::setw(3) << std::setfill('0') << i;
        return os.str();
    }
    std::string headerValue(int i) {
        std::ostringstream os;
        os << "value-" << std::setw(3) << std::setfill('0') << i;
        return os.str();
    }
    std::string toString(const std::vector<char>& bytes) {
        return {bytes.begin(), bytes.end()};
    }
    struct State {
        int listenOkCount = 0, effectiveListenEndpointOkCount = 0, clientConnectOkCount = 0, httpConnectedCount = 0, serverRequestCount = 0,
            clientResponseCount = 0, httpDisconnectedCount = 0, unexpectedStateCount = 0, parseErrorCount = 0;
        bool representativeHeadersVisible = false, clientSawStatus = false, clientSawBody = false;
    };
} // namespace

int main(int argc, char* argv[]) {
    tests::support::TestResult testResult;
    int result = tests::support::cTestSkipReturnCode;
    if (tests::support::shouldSkipRootWithoutSNodeCGroup()) {
        tests::support::printRootWithoutSNodeCGroupSkipMessage("InetHttpServerClientHeaderHeavyResponseTest");
    } else {
        State state;
        core::SNodeC::init(argc, argv);
        using Server = web::http::legacy::in::Server;
        using Client = web::http::legacy::in::Client;
        const Server server("ipv4-http-header-heavy-response-server", [&state](const auto& request, const auto& response) {
            ++state.serverRequestCount;
            for (int i = 0; i < headerCount; ++i)
                response->set(headerName(i), headerValue(i));
            response->status(200).type("text/plain").set("Connection", "close").send("header-heavy-response-ok");
        });
        Client client(
            "ipv4-http-header-heavy-response-client",
            [&state](const auto& request) {
                ++state.httpConnectedCount;
                request->method = "GET";
                request->url = "/header-heavy-response";
                request->set("Connection", "close");
                if (!request->end(
                        [&state](const auto&, const auto& response) {
                            ++state.clientResponseCount;
                            state.clientSawStatus = response->statusCode == "200";
                            state.clientSawBody = toString(response->body) == "header-heavy-response-ok";
                            state.representativeHeadersVisible = response->get(headerName(0)) == headerValue(0) &&
                                                                 response->get(headerName(15)) == headerValue(15) &&
                                                                 response->get(headerName(31)) == headerValue(31);
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
        server.listen(net::in::SocketAddress("127.0.0.1", 0), [&client, &state](const auto& addr, core::socket::State s) {
            if (s == core::socket::State::OK) {
                ++state.listenOkCount;
                const auto port = addr.getPort();
                if (port != 0) {
                    ++state.effectiveListenEndpointOkCount;
                    client.connect(net::in::SocketAddress("127.0.0.1", port), [&state](const auto&, core::socket::State cs) {
                        if (cs == core::socket::State::OK)
                            ++state.clientConnectOkCount;
                        else {
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
        testResult.expectEqual(0, startResult, "event loop stops successfully after IPv4 legacy HTTP header-heavy response");
        testResult.expectEqual(1, state.listenOkCount, "listen OK once");
        testResult.expectEqual(1, state.effectiveListenEndpointOkCount, "effective endpoint once");
        testResult.expectEqual(1, state.clientConnectOkCount, "connect OK once");
        testResult.expectEqual(1, state.httpConnectedCount, "HTTP connected once");
        testResult.expectEqual(1, state.serverRequestCount, "server handler once");
        testResult.expectEqual(1, state.clientResponseCount, "client response once");
        testResult.expectEqual(0, state.parseErrorCount, "no response parse errors");
        testResult.expectEqual(0, state.unexpectedStateCount, "no unexpected states");
        testResult.expectTrue(state.representativeHeadersVisible, "client sees representative bounded response headers");
        testResult.expectTrue(state.clientSawStatus, "client sees 200");
        testResult.expectTrue(state.clientSawBody, "client sees response body");
        result = testResult.processResult();
        core::SNodeC::free();
    }
    return result;
}
