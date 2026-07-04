/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
 */

#ifndef TESTS_COMPONENT_HTTP_HTTPSERVERCLIENTBODYSTATUSTEST_H
#define TESTS_COMPONENT_HTTP_HTTPSERVERCLIENTBODYSTATUSTEST_H

#include "core/SNodeC.h"
#include "support/TestResult.h"
#include "utils/Timeval.h"
#include "web/http/client/Response.h"
#include "web/http/server/Response.h"

#include <memory>
#include <string>
#include <vector>

namespace tests::component::http {

    struct BodyStatusState {
        int listenOkCount = 0;
        int effectiveListenEndpointOkCount = 0;
        int clientConnectOkCount = 0;
        int httpConnectedCount = 0;
        int serverRequestCount = 0;
        int clientResponseCount = 0;
        int httpDisconnectedCount = 0;
        int unexpectedStateCount = 0;
        int parseErrorCount = 0;

        bool serverSawMethod = false;
        bool serverSawUrl = false;
        bool serverSawQuery = false;
        bool serverSawHeader = false;
        bool serverSawBody = false;
        bool clientSawStatus = false;
        bool clientSawReason = false;
        bool clientSawHeader = false;
        bool clientSawBody = false;
    };

    inline std::string toString(const std::vector<char>& bytes) {
        return std::string(bytes.begin(), bytes.end());
    }

    template <typename Server, typename Client>
    void configureHttpBodyStatusTest(Server& server, Client& client, BodyStatusState&) {
        server.getConfig()->Instance::forceUnrequired();
        client.getConfig()->Instance::forceUnrequired();
    }

    template <typename Request, typename Response>
    void handlePostBodyRequest(const std::shared_ptr<Request>& request, const std::shared_ptr<Response>& response, BodyStatusState& state) {
        ++state.serverRequestCount;
        state.serverSawMethod = request->method == "POST";
        state.serverSawUrl = request->url == "/component/body?transport=legacy";
        state.serverSawQuery = request->query("transport") == "legacy";
        state.serverSawHeader = request->get("Content-Type") == "text/plain; charset=utf-8";
        state.serverSawBody = toString(request->body) == "deterministic snode.c component request body";

        response->status(200).type("text/plain").set("X-SNodeC-Body", "received").send("received deterministic request body");
    }

    template <typename MasterRequest>
    void sendPostBodyRequest(const std::shared_ptr<MasterRequest>& request, BodyStatusState& state) {
        ++state.httpConnectedCount;
        request->method = "POST";
        request->url = "/component/body";
        request->query("transport", "legacy");
        request->set("Content-Type", "text/plain; charset=utf-8");
        request->set("Connection", "close");
        request->send(
            "deterministic snode.c component request body",
            [&state](const auto&, const auto& response) {
                ++state.clientResponseCount;
                state.clientSawStatus = response->statusCode == "200";
                state.clientSawHeader = response->get("X-SNodeC-Body") == "received";
                state.clientSawBody = toString(response->body) == "received deterministic request body";
                core::SNodeC::stop();
            },
            [&state](const auto&, const std::string&) {
                ++state.parseErrorCount;
                core::SNodeC::stop();
            });
    }

    template <typename Request, typename Response>
    void handleStatusVariantRequest(const std::shared_ptr<Request>& request, const std::shared_ptr<Response>& response, BodyStatusState& state) {
        ++state.serverRequestCount;
        state.serverSawMethod = request->method == "GET";
        state.serverSawUrl = request->url == "/component/status?transport=legacy";
        state.serverSawQuery = request->query("transport") == "legacy";
        state.serverSawHeader = request->get("X-SNodeC-Status") == "want-404";

        response->status(404).type("text/plain").set("X-SNodeC-Status", "not-found").send("deterministic not found body");
    }

    template <typename MasterRequest>
    void sendStatusVariantRequest(const std::shared_ptr<MasterRequest>& request, BodyStatusState& state) {
        ++state.httpConnectedCount;
        request->method = "GET";
        request->url = "/component/status";
        request->query("transport", "legacy");
        request->set("X-SNodeC-Status", "want-404");
        request->set("Connection", "close");
        request->end(
            [&state](const auto&, const auto& response) {
                ++state.clientResponseCount;
                state.clientSawStatus = response->statusCode == "404";
                state.clientSawReason = response->reason == "Not Found";
                state.clientSawHeader = response->get("X-SNodeC-Status") == "not-found";
                state.clientSawBody = toString(response->body) == "deterministic not found body";
                core::SNodeC::stop();
            },
            [&state](const auto&, const std::string&) {
                ++state.parseErrorCount;
                core::SNodeC::stop();
            });
    }

    inline void expectCommon(tests::support::TestResult& testResult, const BodyStatusState& state, const std::string& transportName) {
        testResult.expectEqual(1, state.listenOkCount, transportName + " HTTP server listen callback reports OK exactly once");
        testResult.expectEqual(1, state.effectiveListenEndpointOkCount, transportName + " HTTP server reports one effective endpoint");
        testResult.expectEqual(1, state.clientConnectOkCount, transportName + " HTTP client connect callback reports OK exactly once");
        testResult.expectEqual(1, state.httpConnectedCount, transportName + " HTTP client reaches HTTP-connected callback exactly once");
        testResult.expectEqual(1, state.serverRequestCount, transportName + " HTTP server observes exactly one request");
        testResult.expectEqual(1, state.clientResponseCount, transportName + " HTTP client observes exactly one response");
        testResult.expectEqual(0, state.parseErrorCount, transportName + " HTTP client reports no response parse errors");
        testResult.expectEqual(0, state.unexpectedStateCount, transportName + " HTTP round trip reports no unexpected socket states");
        testResult.expectTrue(state.serverSawMethod, transportName + " HTTP server observes deterministic method");
        testResult.expectTrue(state.serverSawUrl, transportName + " HTTP server observes deterministic request target");
        testResult.expectTrue(state.serverSawQuery, transportName + " HTTP server observes deterministic query value");
        testResult.expectTrue(state.serverSawHeader, transportName + " HTTP server observes deterministic request header");
        testResult.expectTrue(state.clientSawStatus, transportName + " HTTP client observes deterministic status");
        testResult.expectTrue(state.clientSawHeader, transportName + " HTTP client observes deterministic response header");
        testResult.expectTrue(state.clientSawBody, transportName + " HTTP client observes deterministic response body");
    }

    inline void expectPostBodyRoundTrip(tests::support::TestResult& testResult,
                                        const BodyStatusState& state,
                                        const std::string& transportName,
                                        int startResult) {
        testResult.expectEqual(0, startResult, "event loop stops successfully after " + transportName + " HTTP POST body round trip");
        expectCommon(testResult, state, transportName);
        testResult.expectTrue(state.serverSawBody, transportName + " HTTP server observes deterministic request body");
    }

    inline void expectStatusVariantRoundTrip(tests::support::TestResult& testResult,
                                             const BodyStatusState& state,
                                             const std::string& transportName,
                                             int startResult) {
        testResult.expectEqual(0, startResult, "event loop stops successfully after " + transportName + " HTTP status variant round trip");
        expectCommon(testResult, state, transportName);
        testResult.expectTrue(state.clientSawReason, transportName + " HTTP client observes deterministic reason phrase");
    }

} // namespace tests::component::http

#endif // TESTS_COMPONENT_HTTP_HTTPSERVERCLIENTBODYSTATUSTEST_H
