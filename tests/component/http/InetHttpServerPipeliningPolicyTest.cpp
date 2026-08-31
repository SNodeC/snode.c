/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include "core/SNodeC.h"
#include "net/in/SocketAddress.h"
#include "support/TestResult.h"
#include "utils/Timeval.h"
#include "web/http/client/ConfigHTTP.h"
#include "web/http/client/Response.h"
#include "web/http/legacy/in/Client.h"
#include "web/http/legacy/in/Server.h"
#include "web/http/server/ConfigHttpServer.h"
#include "web/http/server/Request.h"
#include "web/http/server/Response.h"

#include <string>
#include <vector>

namespace {

    struct State {
        int listenOkCount = 0;
        int clientConnectOkCount = 0;
        int httpConnectedCount = 0;
        int queuedRequestCount = 0;
        int serverRequestCount = 0;
        int successfulResponseCount = 0;
        int abortedResponseCount = 0;
        int clientDisconnectCount = 0;
        int unexpectedStateCount = 0;
        std::vector<std::string> serverUrls;
    };

} // namespace

int main(int argc, char* argv[]) {
    tests::support::TestResult testResult;
    int result = tests::support::cTestSkipReturnCode;

    if (tests::support::shouldSkipRootWithoutSNodeCGroup()) {
        tests::support::printRootWithoutSNodeCGroupSkipMessage("InetHttpServerPipeliningPolicyTest");
    } else {
        State state;
        core::SNodeC::init(argc, argv);

        using Server = web::http::legacy::in::Server;
        using Client = web::http::legacy::in::Client;

        const Server server("ipv4-http-server-pipelining-policy", [&state](const auto& request, const auto& response) {
            ++state.serverRequestCount;
            state.serverUrls.push_back(request->url);
            response->status(200).send("ok");
        });
        Client client(
            "ipv4-http-server-pipelining-policy-client",
            [&state](const auto& request) {
                ++state.httpConnectedCount;
                request->method = "GET";
                request->url = "/first";
                request->set("Connection", "keep-alive");
                if (request->end(
                        [&state](const auto&, const auto& response) {
                            if (response->statusCode == "200") {
                                ++state.successfulResponseCount;
                            } else {
                                ++state.abortedResponseCount;
                            }
                        },
                        [](const auto&, const std::string&) {
                        })) {
                    ++state.queuedRequestCount;
                }

                request->method = "GET";
                request->url = "/second";
                request->set("Connection", "close");
                if (request->end(
                        [&state](const auto&, const auto& response) {
                            if (response->statusCode == "200") {
                                ++state.successfulResponseCount;
                            } else {
                                ++state.abortedResponseCount;
                            }
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

        server.getConfig()
            ->Instance::getSubCommand<web::http::server::ConfigHttpServer>()
            ->setMaximumPendingRequests(0)
            ->setAllowPipelining(false);
        client.getConfig()->Instance::getSubCommand<web::http::client::ConfigHTTP>()->setPipelinedRequests(true);
        server.getConfig()->Instance::forceUnrequired();
        client.getConfig()->Instance::forceUnrequired();

        server.listen(net::in::SocketAddress("127.0.0.1", 0),
                      [&client, &state](const auto& socketAddress, core::socket::State listenState) {
                          if (listenState == core::socket::State::OK && socketAddress.getPort() != 0) {
                              ++state.listenOkCount;
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
                      });

        const int startResult = core::SNodeC::start(utils::Timeval({1, 0}));
        testResult.expectEqual(0, startResult, "event loop stops after rejecting buffered pipelining");
        testResult.expectEqual(1, state.listenOkCount, "server listens on one effective endpoint");
        testResult.expectEqual(1, state.clientConnectOkCount, "client connects once");
        testResult.expectEqual(1, state.httpConnectedCount, "HTTP client callback runs once");
        testResult.expectEqual(2, state.queuedRequestCount, "client queues both requests before receiving a response");
        testResult.expectEqual(1, state.serverRequestCount, "synchronous response does not allow a buffered second request");
        testResult.expectTrue(state.serverUrls == std::vector<std::string>{"/first"},
                              "only the first buffered request reaches the synchronous handler");
        testResult.expectEqual(1, state.successfulResponseCount, "first request receives its synchronous response before policy close");
        testResult.expectEqual(1, state.abortedResponseCount, "buffered second request is reported as aborted on policy close");
        testResult.expectEqual(1, state.clientDisconnectCount, "pipelining policy closes the connection once");
        testResult.expectEqual(0, state.unexpectedStateCount, "pipelining policy test reports no unexpected states");

        result = testResult.processResult();
        core::SNodeC::free();
    }

    return result;
}
