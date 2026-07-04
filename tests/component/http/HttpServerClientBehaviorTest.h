/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
 */

#ifndef TESTS_COMPONENT_HTTP_HTTPSERVERCLIENTBEHAVIORTEST_H
#define TESTS_COMPONENT_HTTP_HTTPSERVERCLIENTBEHAVIORTEST_H

#include "core/SNodeC.h"
#include "support/TestResult.h"
#include "utils/Timeval.h"
#include "web/http/server/Request.h"
#include "web/http/server/Response.h"

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

namespace tests::component::http {

    inline constexpr std::size_t boundedBodySize = 64U * 1024U;

    struct BehaviorState {
        int listenOkCount = 0;
        int effectiveListenEndpointOkCount = 0;
        int clientConnectOkCount = 0;
        int httpConnectedCount = 0;
        int httpDisconnectedCount = 0;
        int serverRequestCount = 0;
        int clientResponseCount = 0;
        int parseErrorCount = 0;
        int unexpectedStateCount = 0;
        int queuedRequestCount = 0;

        std::vector<std::string> serverUrls;
        std::vector<std::string> clientBodies;
        std::vector<std::string> clientStatuses;
        std::string serverBody;
        bool clientSawContentLength = false;
    };

    inline std::string toString(const std::vector<char>& bytes) {
        return std::string(bytes.begin(), bytes.end());
    }

    inline std::string makePatternBody(std::size_t size) {
        std::string body;
        body.reserve(size);
        for (std::size_t i = 0; i < size; ++i) {
            body.push_back(static_cast<char>('A' + (i % 26)));
        }
        return body;
    }

    template <typename Server, typename Client>
    void configureHttpBehaviorTest(Server& server, Client& client, BehaviorState&) {
        server.getConfig()->Instance::forceUnrequired();
        client.getConfig()->Instance::forceUnrequired();
    }

    template <typename Request, typename Response>
    void handleRepeatedRequest(const std::shared_ptr<Request>& request, const std::shared_ptr<Response>& response, BehaviorState& state) {
        ++state.serverRequestCount;
        state.serverUrls.push_back(request->url);

        if (request->url == "/first") {
            response->status(200).type("text/plain").set("Connection", "keep-alive").send("first-response");
        } else if (request->url == "/second") {
            response->status(200).type("text/plain").set("Connection", "close").send("second-response");
        } else {
            ++state.unexpectedStateCount;
            response->status(404).set("Connection", "close").send("unexpected repeated request target");
        }
    }

    template <typename MasterRequest>
    void sendRepeatedRequests(const std::shared_ptr<MasterRequest>& request, BehaviorState& state) {
        ++state.httpConnectedCount;
        request->method = "GET";
        request->url = "/first";
        request->set("Connection", "keep-alive");
        if (request->end(
                [&state, request](const auto&, const auto& response) {
                    ++state.clientResponseCount;
                    state.clientStatuses.push_back(response->statusCode);
                    state.clientBodies.push_back(toString(response->body));

                    request->method = "GET";
                    request->url = "/second";
                    request->set("Connection", "close");
                    if (request->end(
                            [&state](const auto&, const auto& secondResponse) {
                                ++state.clientResponseCount;
                                state.clientStatuses.push_back(secondResponse->statusCode);
                                state.clientBodies.push_back(toString(secondResponse->body));
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
                [&state](const auto&, const std::string&) {
                    ++state.parseErrorCount;
                    core::SNodeC::stop();
                })) {
            ++state.queuedRequestCount;
        } else {
            ++state.unexpectedStateCount;
            core::SNodeC::stop();
        }
    }

    template <typename Request, typename Response>
    void handleDisconnectRequest(const std::shared_ptr<Request>& request, const std::shared_ptr<Response>& response, BehaviorState& state) {
        ++state.serverRequestCount;
        state.serverUrls.push_back(request->url);
        response->status(200).type("text/plain").set("Connection", "close").send("disconnect-ok");
    }

    template <typename MasterRequest>
    void sendDisconnectRequest(const std::shared_ptr<MasterRequest>& request, BehaviorState& state) {
        ++state.httpConnectedCount;
        request->method = "GET";
        request->url = "/disconnect";
        request->set("Connection", "close");
        if (!request->end(
                [&state](const auto&, const auto& response) {
                    ++state.clientResponseCount;
                    state.clientStatuses.push_back(response->statusCode);
                    state.clientBodies.push_back(toString(response->body));
                },
                [&state](const auto&, const std::string&) {
                    ++state.parseErrorCount;
                    core::SNodeC::stop();
                })) {
            ++state.unexpectedStateCount;
            core::SNodeC::stop();
        }
    }

    template <typename Request, typename Response>
    void handlePrematureCloseRequest(const std::shared_ptr<Request>& request, const std::shared_ptr<Response>& response, BehaviorState& state) {
        ++state.serverRequestCount;
        state.serverUrls.push_back(request->url);
        response->getSocketContext()->close();
    }

    template <typename MasterRequest>
    void sendPrematureCloseRequest(const std::shared_ptr<MasterRequest>& request, BehaviorState& state) {
        ++state.httpConnectedCount;
        request->method = "GET";
        request->url = "/premature-close";
        request->set("Connection", "close");
        if (!request->end(
                [&state](const auto&, const auto& response) {
                    ++state.clientResponseCount;
                    state.clientStatuses.push_back(response->statusCode);
                    state.clientBodies.push_back(response->reason);
                    core::SNodeC::stop();
                },
                [&state](const auto&, const std::string&) {
                    ++state.parseErrorCount;
                    core::SNodeC::stop();
                })) {
            ++state.unexpectedStateCount;
            core::SNodeC::stop();
        }
    }

    template <typename Request, typename Response>
    void handleLargeResponseRequest(const std::shared_ptr<Request>& request, const std::shared_ptr<Response>& response, BehaviorState& state) {
        ++state.serverRequestCount;
        state.serverUrls.push_back(request->url);
        const std::string body = makePatternBody(boundedBodySize);
        response->status(200).type("application/octet-stream").set("Connection", "close").send(body);
    }

    template <typename MasterRequest>
    void sendLargeResponseRequest(const std::shared_ptr<MasterRequest>& request, BehaviorState& state) {
        ++state.httpConnectedCount;
        request->method = "GET";
        request->url = "/large-response";
        request->set("Connection", "close");
        if (!request->end(
                [&state](const auto&, const auto& response) {
                    ++state.clientResponseCount;
                    state.clientStatuses.push_back(response->statusCode);
                    state.clientBodies.push_back(toString(response->body));
                    state.clientSawContentLength = response->get("Content-Length") == std::to_string(boundedBodySize);
                    core::SNodeC::stop();
                },
                [&state](const auto&, const std::string&) {
                    ++state.parseErrorCount;
                    core::SNodeC::stop();
                })) {
            ++state.unexpectedStateCount;
            core::SNodeC::stop();
        }
    }

    template <typename Request, typename Response>
    void handleLargeRequestRequest(const std::shared_ptr<Request>& request, const std::shared_ptr<Response>& response, BehaviorState& state) {
        ++state.serverRequestCount;
        state.serverUrls.push_back(request->url);
        state.serverBody = toString(request->body);
        response->status(200).type("text/plain").set("Connection", "close").send("large-request-ok");
    }

    template <typename MasterRequest>
    void sendLargeRequestRequest(const std::shared_ptr<MasterRequest>& request, BehaviorState& state) {
        ++state.httpConnectedCount;
        request->method = "POST";
        request->url = "/large-request";
        request->set("Connection", "close");
        if (!request->send(
                makePatternBody(boundedBodySize),
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
            ++state.unexpectedStateCount;
            core::SNodeC::stop();
        }
    }



    template <typename Request, typename Response>
    void handleFragmentedResponseRequest(const std::shared_ptr<Request>& request, const std::shared_ptr<Response>& response, BehaviorState& state) {
        ++state.serverRequestCount;
        state.serverUrls.push_back(request->url);
        response->status(200).type("text/plain").set("Connection", "close").set("Transfer-Encoding", "chunked").sendHeader();
        response->sendFragment("fragment-");
        response->sendFragment("response-");
        response->sendFragment("ok");
        response->end();
    }

    template <typename MasterRequest>
    void sendFragmentedResponseRequest(const std::shared_ptr<MasterRequest>& request, BehaviorState& state) {
        ++state.httpConnectedCount;
        request->method = "GET";
        request->url = "/fragmented-response";
        request->set("Connection", "close");
        if (!request->end(
                [&state](const auto&, const auto& response) {
                    ++state.clientResponseCount;
                    state.clientStatuses.push_back(response->statusCode);
                    state.clientBodies.push_back(toString(response->body));
                    state.clientSawContentLength = response->get("Transfer-Encoding") == "chunked";
                    core::SNodeC::stop();
                },
                [&state](const auto&, const std::string&) {
                    ++state.parseErrorCount;
                    core::SNodeC::stop();
                })) {
            ++state.unexpectedStateCount;
            core::SNodeC::stop();
        }
    }

    template <typename Request, typename Response>
    void handleFragmentedRequestRequest(const std::shared_ptr<Request>& request, const std::shared_ptr<Response>& response, BehaviorState& state) {
        ++state.serverRequestCount;
        state.serverUrls.push_back(request->url);
        state.serverBody = toString(request->body);
        state.clientSawContentLength = request->get("Transfer-Encoding") == "chunked";
        response->status(200).type("text/plain").set("Connection", "close").send("fragmented-request-ok");
    }

    template <typename MasterRequest>
    void sendFragmentedRequestRequest(const std::shared_ptr<MasterRequest>& request, BehaviorState& state) {
        ++state.httpConnectedCount;
        request->method = "POST";
        request->url = "/fragmented-request";
        request->set("Connection", "close");
        request->set("Transfer-Encoding", "chunked");
        request->sendHeader().sendFragment("fragment-").sendFragment("request-").sendFragment("ok");
        if (!request->end(
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
            ++state.unexpectedStateCount;
            core::SNodeC::stop();
        }
    }

    inline void expectBehaviorCommon(tests::support::TestResult& testResult,
                                     const BehaviorState& state,
                                     const std::string& behaviorName,
                                     int startResult) {
        testResult.expectEqual(0, startResult, "event loop stops successfully after IPv4 legacy HTTP " + behaviorName);
        testResult.expectEqual(1, state.listenOkCount, "IPv4 legacy HTTP server listen callback reports OK exactly once for " + behaviorName);
        testResult.expectEqual(1, state.effectiveListenEndpointOkCount, "IPv4 legacy HTTP server reports one effective endpoint for " + behaviorName);
        testResult.expectEqual(1, state.clientConnectOkCount, "IPv4 legacy HTTP client connect callback reports OK exactly once for " + behaviorName);
        testResult.expectEqual(1, state.httpConnectedCount, "IPv4 legacy HTTP client reaches HTTP-connected callback exactly once for " + behaviorName);
        testResult.expectEqual(0, state.parseErrorCount, "IPv4 legacy HTTP client reports no response parse errors for " + behaviorName);
        testResult.expectEqual(0, state.unexpectedStateCount, "IPv4 legacy HTTP reports no unexpected socket states for " + behaviorName);
    }

} // namespace tests::component::http

#endif // TESTS_COMPONENT_HTTP_HTTPSERVERCLIENTBEHAVIORTEST_H
