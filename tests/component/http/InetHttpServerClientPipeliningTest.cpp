#include "core/SNodeC.h"
#include "net/in/SocketAddress.h"
#include "support/TestResult.h"
#include "utils/Timeval.h"
#include "web/http/client/ConfigHTTP.h"
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
        int queuedRequestCount = 0;
        std::vector<std::string> serverUrls;
        std::vector<std::string> clientStatuses;
        std::vector<std::string> clientBodies;
    };
} // namespace

int main(int argc, char* argv[]) {
    tests::support::TestResult testResult;
    int result = tests::support::cTestSkipReturnCode;

    if (tests::support::shouldSkipRootWithoutSNodeCGroup()) {
        tests::support::printRootWithoutSNodeCGroupSkipMessage("InetHttpServerClientPipeliningTest");
    } else {
        State state;
        core::SNodeC::init(argc, argv);

        using Server = web::http::legacy::in::Server;
        using Client = web::http::legacy::in::Client;

        const Server server("ipv4-http-pipelining-server", [&state](const auto& request, const auto& response) {
            ++state.serverRequestCount;
            state.serverUrls.push_back(request->url);
            if (request->url == "/pipeline-first") {
                response->status(200).type("text/plain").set("Connection", "keep-alive").send("pipeline-first-response");
            } else if (request->url == "/pipeline-second") {
                response->status(200).type("text/plain").set("Connection", "close").send("pipeline-second-response");
            } else {
                ++state.unexpectedStateCount;
                response->status(404).set("Connection", "close").send("unexpected pipeline target");
            }
        });
        Client client(
            "ipv4-http-pipelining-client",
            [&state](const auto& request) {
                ++state.httpConnectedCount;
                request->method = "GET";
                request->url = "/pipeline-first";
                request->set("Connection", "keep-alive");
                if (request->end(
                        [&state](const auto&, const auto& response) {
                            ++state.clientResponseCount;
                            state.clientStatuses.push_back(response->statusCode);
                            state.clientBodies.push_back(toString(response->body));
                        },
                        [&state](const auto&, const std::string&) {
                            ++state.parseErrorCount;
                            core::SNodeC::stop();
                        })) {
                    ++state.queuedRequestCount;
                } else {
                    ++state.unexpectedStateCount;
                    core::SNodeC::stop();
                    return;
                }

                request->method = "GET";
                request->url = "/pipeline-second";
                request->set("Connection", "close");
                if (request->end(
                        [&state](const auto&, const auto& response) {
                            ++state.clientResponseCount;
                            state.clientStatuses.push_back(response->statusCode);
                            state.clientBodies.push_back(toString(response->body));
                            core::SNodeC::stop();
                        },
                        [&state](const auto&, const std::string&) {
                            ++state.parseErrorCount;
                            core::SNodeC::stop();
                        })) {
                    ++state.queuedRequestCount;
                } else {
                    ++state.unexpectedStateCount;
                    core::SNodeC::stop();
                }
            },
            [&state](const auto&) {
                ++state.httpDisconnectedCount;
            });
        server.getConfig()->Instance::forceUnrequired();
        client.getConfig()->Instance::forceUnrequired();
        client.getConfig()->Instance::getSubCommand<web::http::client::ConfigHTTP>()->setPipelinedRequests(true);

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
        testResult.expectEqual(0, startResult, "event loop stops successfully after IPv4 legacy HTTP pipelining");
        testResult.expectEqual(1, state.listenOkCount, "listen OK once");
        testResult.expectEqual(1, state.effectiveListenEndpointOkCount, "effective endpoint once");
        testResult.expectEqual(1, state.clientConnectOkCount, "connect OK once");
        testResult.expectEqual(1, state.httpConnectedCount, "HTTP connected once");
        testResult.expectEqual(2, state.queuedRequestCount, "client queues both pipelined requests before first response callback");
        testResult.expectEqual(2, state.serverRequestCount, "server observes two pipelined requests");
        testResult.expectEqual(2, state.clientResponseCount, "client observes two pipelined responses");
        testResult.expectEqual(0, state.parseErrorCount, "no response parse errors");
        testResult.expectEqual(0, state.unexpectedStateCount, "no unexpected states");
        testResult.expectTrue(state.serverUrls == std::vector<std::string>({"/pipeline-first", "/pipeline-second"}), "server observes pipelined targets in order");
        testResult.expectTrue(state.clientStatuses == std::vector<std::string>({"200", "200"}), "client observes pipelined statuses in order");
        testResult.expectTrue(state.clientBodies == std::vector<std::string>({"pipeline-first-response", "pipeline-second-response"}), "client observes pipelined bodies in order");
        result = testResult.processResult();
        core::SNodeC::free();
    }

    return result;
}
