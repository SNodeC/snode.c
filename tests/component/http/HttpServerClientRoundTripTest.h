/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
 */

#ifndef TESTS_COMPONENT_HTTP_HTTPSERVERCLIENTROUNDTRIPTEST_H
#define TESTS_COMPONENT_HTTP_HTTPSERVERCLIENTROUNDTRIPTEST_H

#include "core/SNodeC.h"
#include "support/TestResult.h"
#include "utils/Timeval.h"
#include "web/http/client/Response.h"
#include "web/http/server/Response.h"

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

namespace tests::component::http {

    struct RoundTripState {
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
        bool clientSawStatus = false;
        bool clientSawHeader = false;
        bool clientSawBody = false;
    };

    inline std::string toString(const std::vector<char>& bytes) {
        return std::string(bytes.begin(), bytes.end());
    }

    template <typename Server, typename Client>
    void configureHttpRoundTrip(Server& server, Client& client, RoundTripState& state) {
        server.getConfig()->Instance::forceUnrequired();
        client.getConfig()->Instance::forceUnrequired();
    }

    template <typename Request, typename Response>
    void handleRequest(const std::shared_ptr<Request>& request, const std::shared_ptr<Response>& response, RoundTripState& state) {
        ++state.serverRequestCount;
        state.serverSawMethod = request->method == "GET";
        state.serverSawUrl = request->url == "/component/round-trip?transport=legacy";
        state.serverSawQuery = request->query("transport") == "legacy";
        state.serverSawHeader = request->get("X-SNodeC-Component") == "http-round-trip";

        response->status(200).type("text/plain").set("X-SNodeC-Response", "component-ok").send("hello from snode.c http component test");
    }

    template <typename MasterRequest>
    void sendRequest(const std::shared_ptr<MasterRequest>& request, RoundTripState& state) {
        ++state.httpConnectedCount;
        request->method = "GET";
        request->url = "/component/round-trip";
        request->query("transport", "legacy");
        request->set("X-SNodeC-Component", "http-round-trip");
        request->set("Connection", "close");
        request->end(
            [&state](const auto&, const auto& response) {
                ++state.clientResponseCount;
                state.clientSawStatus = response->statusCode == "200";
                state.clientSawHeader = response->get("X-SNodeC-Response") == "component-ok";
                state.clientSawBody = toString(response->body) == "hello from snode.c http component test";
                core::SNodeC::stop();
            },
            [&state](const auto&, const std::string&) {
                ++state.parseErrorCount;
                core::SNodeC::stop();
            });
    }

    inline void expectRoundTrip(tests::support::TestResult& testResult,
                                const RoundTripState& state,
                                const std::string& transportName,
                                int startResult) {
        testResult.expectEqual(0, startResult, "event loop stops successfully after " + transportName + " HTTP round trip");
        testResult.expectEqual(1, state.listenOkCount, transportName + " HTTP server listen callback reports OK exactly once");
        testResult.expectEqual(1, state.effectiveListenEndpointOkCount, transportName + " HTTP server reports one effective endpoint");
        testResult.expectEqual(1, state.clientConnectOkCount, transportName + " HTTP client connect callback reports OK exactly once");
        testResult.expectEqual(1, state.httpConnectedCount, transportName + " HTTP client reaches HTTP-connected callback exactly once");
        testResult.expectEqual(1, state.serverRequestCount, transportName + " HTTP server observes exactly one request");
        testResult.expectEqual(1, state.clientResponseCount, transportName + " HTTP client observes exactly one response");
        testResult.expectEqual(0, state.parseErrorCount, transportName + " HTTP client reports no response parse errors");
        testResult.expectEqual(0, state.unexpectedStateCount, transportName + " HTTP round trip reports no unexpected socket states");
        testResult.expectTrue(state.serverSawMethod, transportName + " HTTP server observes GET method");
        testResult.expectTrue(state.serverSawUrl, transportName + " HTTP server observes deterministic request target");
        testResult.expectTrue(state.serverSawQuery, transportName + " HTTP server observes deterministic query value");
        testResult.expectTrue(state.serverSawHeader, transportName + " HTTP server observes deterministic request header");
        testResult.expectTrue(state.clientSawStatus, transportName + " HTTP client observes status 200");
        testResult.expectTrue(state.clientSawHeader, transportName + " HTTP client observes deterministic response header");
        testResult.expectTrue(state.clientSawBody, transportName + " HTTP client observes deterministic response body");
    }

} // namespace tests::component::http

#endif // TESTS_COMPONENT_HTTP_HTTPSERVERCLIENTROUNDTRIPTEST_H
