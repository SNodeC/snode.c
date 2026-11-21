// SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
#include "mcp/server/Server.h"
#include "tests/support/TestResult.h"

#include <string>

using mcp::server::Json;
namespace {
    Json request(const std::string& method, Json params = Json::object()) {
        params["_meta"] = {{mcp::server::versionKey, mcp::server::modernVersion}, {mcp::server::capabilitiesKey, Json::object()}};
        return {{"jsonrpc", "2.0"}, {"id", 7}, {"method", method}, {"params", params}};
    }
} // namespace
int main() {
    tests::support::TestResult test;
    using mcp::server::Server;
    std::string error;
    const Json schema{{"$schema", "https://json-schema.org/draft/2020-12/schema"},
                      {"type", "object"},
                      {"properties",
                       {{"word", {{"type", "string"}, {"minLength", 1}, {"maxLength", 2}}},
                        {"number", {{"type", "integer"}, {"minimum", 0}, {"maximum", 3}}},
                        {"list", {{"type", "array"}, {"items", {{"enum", {1, 2}}}}, {"minItems", 1}, {"maxItems", 2}}}}},
                      {"required", {"word"}},
                      {"additionalProperties", false}};
    test.expectTrue(Server::checkSchema(schema, error), "supported dialect/subset accepted");
    test.expectTrue(Server::validate(schema, {{"word", "é日"}, {"number", 2.0}, {"list", {1, 2}}}, error),
                    "Unicode code points, integral numbers, array items");
    for (const auto& value : Json::array({Json::object(),
                                          {{"word", "abc"}},
                                          {{"word", ""}},
                                          {{"word", "a"}, {"extra", true}},
                                          {{"word", "a"}, {"number", -1}},
                                          {{"word", "a"}, {"number", 4}},
                                          {{"word", "a"}, {"number", 1.5}},
                                          {{"word", "a"}, {"list", Json::array()}},
                                          {{"word", "a"}, {"list", {3}}},
                                          {{"word", "a"}, {"list", {1, 2, 1}}}})) {
        test.expectTrue(!Server::validate(schema, value, error), "invalid value rejected: " + value.dump());
    }
    for (const std::string keyword :
         {"$ref", "$defs", "anyOf", "oneOf", "allOf", "not", "if", "then", "else", "patternProperties", "format"}) {
        Json unsupported = schema;
        unsupported["properties"]["word"][keyword] = Json::object();
        test.expectTrue(!Server::checkSchema(unsupported, error), "unsupported nested keyword rejected: " + keyword);
    }
    for (const Json& malformed : Json::array({{{"$schema", "http://json-schema.org/draft-07/schema#"}},
                                              {{"type", "typo"}},
                                              {{"required", {1}}},
                                              {{"required", {"a", "a"}}},
                                              {{"additionalProperties", Json::object()}},
                                              {{"minLength", -1}},
                                              {{"maxItems", 0.5}},
                                              {{"items", 1}},
                                              {{"properties", Json::array()}},
                                              {{"enum", {1, 1}}},
                                              {{"minimum", "a"}}})) {
        test.expectTrue(!Server::checkSchema(malformed, error), "malformed schema rejected: " + malformed.dump());
    }
    test.expectTrue(Server::validate({{"const", nullptr}}, nullptr, error), "const null");
    test.expectTrue(!Server::validate(false, true, error), "boolean false schema");
    test.expectTrue(Server::validate({{"type", {"null", "boolean"}}}, true, error), "type union");
    Server server("test");
    int calls = 0;
    std::shared_ptr<mcp::server::Reply> pending;
    server.tool({{"name", "echo"}, {"inputSchema", schema}}, [&calls](const Json& args, const auto& reply) {
        ++calls;
        reply->result({{"content", {{{"type", "text"}, {"text", args["word"]}}}}});
        reply->result(Json::object());
    });
    server.tool({{"name", "pending"}, {"inputSchema", {{"type", "object"}}}}, [&pending](const Json&, const auto& reply) {
        pending = reply;
    });
    server.resource({{"name", "style"}, {"uri", "snodec://style"}}, [](const Json&, const auto& reply) {
        reply->result({{"contents", {{{"uri", "snodec://style"}, {"text", "style"}}}}});
    });
    server.prompt({{"name", "review"}}, [](const Json&, const auto& reply) {
        reply->result({{"messages", Json::array()}});
    });
    server.resource({{"name", "slow"}, {"uri", "snodec://slow"}}, [&pending](const Json&, const auto& reply) {
        pending = reply;
    });
    Json response;
    int messages = 0;
    auto output = [&response, &messages](const Json& value, bool) {
        response = value;
        if (!value.is_null()) {
            ++messages;
        }
    };
    server.dispatch(request("server/discover"), false, output);
    test.expectTrue(response["result"]["supportedVersions"] == Json::array({mcp::server::modernVersion, mcp::server::legacyVersion}),
                    "versions");
    test.expectTrue(response["result"]["capabilities"] ==
                        Json({{"tools", Json::object()}, {"resources", Json::object()}, {"prompts", Json::object()}}),
                    "capabilities derived without subscribe/listChanged");
    test.expectTrue(response["result"]["resultType"] == "complete" && response["result"]["ttlMs"] == 0 &&
                        response["result"]["cacheScope"] == "private",
                    "modern result/cache contract");
    server.dispatch(request("tools/list"), false, output);
    test.expectEqual(2, static_cast<int>(response["result"]["tools"].size()), "populated tool registry");
    const int before = messages;
    server.dispatch(request("tools/call", {{"name", "echo"}, {"arguments", {{"word", "é"}}}}), false, output);
    test.expectEqual(before + 1, messages, "only one terminal response");
    test.expectEqual(1, calls, "valid handler invoked");
    server.dispatch(request("tools/call", {{"name", "echo"}, {"arguments", {{"word", "too long"}}}}), false, output);
    test.expectTrue(response["result"]["isError"] == true && !response.contains("error"), "input-value failure is a tool error");
    test.expectEqual(1, calls, "invalid arguments never reach handler");
    for (const auto& [method, code] : std::map<std::string, int>{{"ping", -32601}, {"unknown", -32601}, {"tools/call", -32602}}) {
        server.dispatch(request(method), false, output);
        test.expectEqual(code, response["error"]["code"].get<int>(), "protocol error mapping");
    }
    Json noMeta = request("tools/list");
    noMeta["params"].erase("_meta");
    server.dispatch(noMeta, false, output);
    test.expectEqual(-32602, response["error"]["code"].get<int>(), "no inherited modern capabilities");
    Json unsupported = request("tools/list");
    unsupported["params"]["_meta"][mcp::server::versionKey] = "other";
    server.dispatch(unsupported, false, output);
    test.expectEqual(-32022, response["error"]["code"].get<int>(), "unsupported version code");
    test.expectTrue(response["error"]["data"].contains("supported") && response["error"]["data"]["requested"] == "other",
                    "version error data");
    for (const Json& malformed : Json::array({nullptr,
                                              Json::array(),
                                              {{"jsonrpc", "2.0"}, {"method", "x"}, {"id", nullptr}},
                                              {{"jsonrpc", "1"}, {"method", "x"}, {"id", 1}},
                                              {{"jsonrpc", "2.0"}, {"method", "x"}, {"params", Json::array()}}})) {
        server.dispatch(malformed, false, output);
        test.expectEqual(-32600, response["error"]["code"].get<int>(), "invalid envelopes");
    }
    const Json initialize{
        {"jsonrpc", "2.0"},
        {"method", "initialize"},
        {"id", 1},
        {"params",
         {{"protocolVersion", "2025-06-18"}, {"capabilities", Json::object()}, {"clientInfo", {{"name", "test"}, {"version", "1"}}}}}};
    server.dispatch(initialize, true, output);
    test.expectTrue(response["result"]["protocolVersion"] == mcp::server::legacyVersion && !response["result"].contains("resultType"),
                    "legacy normalization");
    Json malformedInitialize = initialize;
    malformedInitialize["params"].erase("clientInfo");
    server.dispatch(malformedInitialize, true, output);
    test.expectEqual(-32602, response["error"]["code"].get<int>(), "legacy initialize requires client information");
    server.dispatch(request("server/discover"), true, output);
    test.expectEqual(-32601, response["error"]["code"].get<int>(), "discover belongs only to the modern era");
    server.dispatch(request("resources/read", {{"uri", "missing"}}), true, output);
    test.expectEqual(-32002, response["error"]["code"].get<int>(), "legacy resource error");
    server.dispatch(request("resources/read", {{"uri", "missing"}}), false, output);
    test.expectEqual(-32602, response["error"]["code"].get<int>(), "modern resource error remains strict");
    server.dispatch(request("tools/list"), false, output);
    test.expectTrue(response["result"].contains("resultType"), "legacy dispatch never changes modern contract");
    const int notified = messages;
    server.dispatch({{"jsonrpc", "2.0"}, {"method", "notifications/initialized"}}, true, output);
    test.expectEqual(notified, messages, "no notification response");
    server.dispatch(request("resources/read", {{"uri", "snodec://slow"}}), false, output, std::chrono::milliseconds(0));
    pending->expire();
    test.expectEqual(-32603, response["error"]["code"].get<int>(), "resource deadline is a JSON-RPC error");
    server.dispatch(request("tools/call", {{"name", "pending"}}), false, output, std::chrono::milliseconds(0));
    pending->expire();
    test.expectTrue(response["result"]["isError"] == true, "tool deadline is a tool error");
    server.dispatch(request("tools/call", {{"name", "pending"}}), false, output);
    pending->cancel();
    const int cancelled = messages;
    pending->result(Json::object());
    test.expectEqual(cancelled, messages, "cancel prevents late result");
    auto subscription = server.dispatch(request("subscriptions/listen", {{"notifications", {{"toolsListChanged", true}}}}), false, output);
    test.expectTrue(response["method"] == "notifications/subscriptions/acknowledged" && response["params"]["notifications"].empty() &&
                        response["params"]["_meta"][mcp::server::subscriptionKey] == 7,
                    "empty honored filter and correlated ack");
    std::vector<Json> shutdown;
    auto ending = server.dispatch(request("subscriptions/listen"), false, [&shutdown](const Json& value, bool) {
        shutdown.push_back(value);
    });
    ending->cancel(true);
    test.expectTrue(shutdown.size() == 3 && shutdown[1]["method"] == "notifications/cancelled" && shutdown.back().is_null(),
                    "subscription teardown cancellation");
    subscription->cancel();
    Server invalid("invalid");
    invalid.tool({{"name", "bad"}, {"inputSchema", {{"type", "object"}, {"$ref", "unsafe"}}}}, [](const Json&, const auto&) {
    });
    test.expectTrue(!invalid.valid(), "unsupported advertised schema poisons startup");
    server.tool({{"name", "echo"}, {"inputSchema", schema}}, [](const Json&, const auto&) {
    });
    test.expectTrue(!server.valid(), "duplicate registration rejected");
    return test.processResult();
}
