/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
 */

#include "core/SNodeC.h"
#include "net/in/SocketAddress.h"
#include "support/TestResult.h"
#include "utils/Timeval.h"
#include "web/http/client/ConfigHTTP.h"
#include "web/http/legacy/in/Client.h"
#include "web/http/legacy/in/Server.h"
#include "web/http/server/ConfigHttpServer.h"

#include <cstdint>
#include <string>
#include <vector>

namespace {

    struct State {
        int listenOkCount = 0;
        int effectiveListenEndpointOkCount = 0;
        int clientConnectOkCount = 0;
        int httpConnectedCount = 0;
        int queuedRequestCount = 0;
        int serverRequestCount = 0;
        int clientDisconnectCount = 0;
        int unexpectedStateCount = 0;
        std::vector<std::string> serverUrls;
    };

} // namespace

int main(int argc, char* argv[]) {
    tests::support::TestResult testResult;
    int result = tests::support::cTestSkipReturnCode;

    if (tests::support::shouldSkipRootWithoutSNodeCGroup()) {
        tests::support::printRootWithoutSNodeCGroupSkipMessage("InetHttpServerRequestPolicyTest");
    } else {
        State state;
        core::SNodeC::init(argc, argv);

        using Server = web::http::legacy::in::Server;
        using Client = web::http::legacy::in::Client;

        const Server server("ipv4-http-server-request-policy", [&state](const auto& request, const auto&) {
            ++state.serverRequestCount;
            state.serverUrls.push_back(request->url);
            // Intentionally leave the first response pending so the second parsed
            // request exercises the per-connection pending/pipelining policy.
        });
        Client client(
            "ipv4-http-server-request-policy-client",
            [&state](const auto& request) {
                ++state.httpConnectedCount;
                request->method = "GET";
                request->url = "/first";
                request->set("Connection", "keep-alive");
                if (request->end(
                        [](const auto&, const auto&) {
                        },
                        [](const auto&, const std::string&) {
                        })) {
                    ++state.queuedRequestCount;
                }

                request->method = "GET";
                request->url = "/second";
                request->set("Connection", "close");
                if (request->end(
                        [](const auto&, const auto&) {
                        },
                        [](const auto&, const std::string&) {
                        })) {
                    ++state.queuedRequestCount;
                }
            },
            [&state](const auto&) {
                ++state.clientDisconnectCount;
                core::SNodeC::stop();
            });

        auto* serverHttpConfig = server.getConfig()->Instance::getSubCommand<web::http::server::ConfigHttpServer>();
        serverHttpConfig->setMaximumPendingRequests(1)->setAllowPipelining(true);
        client.getConfig()->Instance::getSubCommand<web::http::client::ConfigHTTP>()->setPipelinedRequests(true);
        server.getConfig()->Instance::forceUnrequired();
        client.getConfig()->Instance::forceUnrequired();

        server.listen(net::in::SocketAddress("127.0.0.1", 0),
                      [&client, &state](const auto& socketAddress, core::socket::State listenState) {
                          if (listenState == core::socket::State::OK) {
                              ++state.listenOkCount;
                              if (socketAddress.getPort() != 0) {
                                  ++state.effectiveListenEndpointOkCount;
                                  client.connect(net::in::SocketAddress("127.0.0.1", socketAddress.getPort()),
                                                 [&state](const auto&, core::socket::State connectState) {
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
        testResult.expectEqual(0, startResult, "event loop stops after server rejects excess pipelined request");
        testResult.expectEqual(1, state.listenOkCount, "server listen succeeds once");
        testResult.expectEqual(1, state.effectiveListenEndpointOkCount, "server reports an effective endpoint");
        testResult.expectEqual(1, state.clientConnectOkCount, "client connects once");
        testResult.expectEqual(1, state.httpConnectedCount, "HTTP client callback runs once");
        testResult.expectEqual(2, state.queuedRequestCount, "client queues both requests before a response");
        testResult.expectEqual(1, state.serverRequestCount, "server retains and dispatches only the first request");
        testResult.expectTrue(state.serverUrls == std::vector<std::string>{"/first"}, "excess pipelined request never reaches the handler");
        testResult.expectEqual(1, state.clientDisconnectCount, "policy rejection closes the connection safely");
        testResult.expectEqual(0, state.unexpectedStateCount, "server request policy test reports no unexpected states");

        result = testResult.processResult();
        core::SNodeC::free();
    }

    return result;
}
