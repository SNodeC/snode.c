// SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
#include "core/SNodeC.h"
#include "mcp/server/detail/Server.h"
#include "tests/support/TestResult.h"

#include <string>

using mcp::server::Json;
using mcp::server::ToolResult;
namespace {
    Json request(const std::string& method, Json params = Json::object()) {
        params["_meta"] = {{mcp::server::detail::versionKey, mcp::server::detail::protocolVersion2026},
                           {mcp::server::detail::capabilitiesKey, Json::object()}};
        return {{"jsonrpc", "2.0"}, {"id", 7}, {"method", method}, {"params", params}};
    }
} // namespace
int main(int argc, char* argv[]) {
    const auto objectSchema = mcp::server::Json::parse(R"json(
{
    "type": "object"
}
)json");
    core::SNodeC::init(argc, argv);
    tests::support::TestResult test;
    using mcp::server::Server;
    using mcp::server::detail::Access;
    std::string error;
    const Json schema = Json::parse(R"json(
{
    "$schema": "https://json-schema.org/draft/2020-12/schema",
    "additionalProperties": false,
    "properties": {
        "list": {
            "items": {
                "enum": [
                    1,
                    2
                ]
            },
            "maxItems": 2,
            "minItems": 1,
            "type": "array"
        },
        "number": {
            "maximum": 3,
            "minimum": 0,
            "type": "integer"
        },
        "word": {
            "maxLength": 2,
            "minLength": 1,
            "type": "string"
        }
    },
    "required": [
        "word"
    ],
    "type": "object"
}
)json");
    test.expectTrue(Access::Registry::checkSchema(schema, error), "supported dialect/subset accepted");
    test.expectTrue(Access::Registry::validate(schema, {{"word", "é日"}, {"number", 2.0}, {"list", {1, 2}}}, error),
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
        test.expectTrue(!Access::Registry::validate(schema, value, error), "invalid value rejected: " + value.dump());
    }
    for (const std::string keyword :
         {"$ref", "$defs", "anyOf", "oneOf", "allOf", "not", "if", "then", "else", "patternProperties", "format"}) {
        Json unsupported = schema;
        unsupported["properties"]["word"][keyword] = Json::object();
        test.expectTrue(!Access::Registry::checkSchema(unsupported, error), "unsupported nested keyword rejected: " + keyword);
    }
    for (const Json& malformed : Json::array({Json::parse(R"json(
{
    "$schema": "http://json-schema.org/draft-07/schema#"
}
)json"),
                                              Json::parse(R"json(
{
    "type": "typo"
}
)json"),
                                              Json::parse(R"json(
{
    "required": [
        1
    ]
}
)json"),
                                              Json::parse(R"json(
{
    "required": [
        "a",
        "a"
    ]
}
)json"),
                                              Json::parse(R"json(
{
    "additionalProperties": {}
}
)json"),
                                              Json::parse(R"json(
{
    "minLength": -1
}
)json"),
                                              Json::parse(R"json(
{
    "maxItems": 0.5
}
)json"),
                                              Json::parse(R"json(
{
    "items": 1
}
)json"),
                                              Json::parse(R"json(
{
    "properties": []
}
)json"),
                                              Json::parse(R"json(
{
    "enum": [
        1,
        1
    ]
}
)json"),
                                              Json::parse(R"json(
{
    "minimum": "a"
}
)json")})) {
        test.expectTrue(!Access::Registry::checkSchema(malformed, error), "malformed schema rejected: " + malformed.dump());
    }
    test.expectTrue(Access::Registry::validate(Json::parse(R"json(
{
    "const": null
}
)json"),
                                               nullptr,
                                               error),
                    "const null");
    test.expectTrue(!Access::Registry::validate(Json::parse(R"json(
false
)json"),
                                                true,
                                                error),
                    "boolean false schema");
    test.expectTrue(Access::Registry::validate(Json::parse(R"json(
{
    "type": [
        "null",
        "boolean"
    ]
}
)json"),
                                               true,
                                               error),
                    "type union");
    Server server("test", "test-version");
    int calls = 0;
    std::shared_ptr<mcp::server::ToolReply> pending;
    std::shared_ptr<mcp::server::ResourceReply> pendingResource;
    server.tool({"echo", {}, schema}, [&calls](const ::mcp::server::ToolCall& call, const auto& reply) {
        const auto& args = call.arguments;
        ++calls;
        reply->result(ToolResult::text(args["word"]));
        reply->result(ToolResult::content({}));
    });
    server.tool({"pending", {}, objectSchema}, [&pending](const auto&, const auto& reply) {
        pending = reply;
    });
    server.resource({"snodec://style", "style"}, [](const auto&, const auto& reply) {
        reply->result(mcp::server::ResourceResult{{mcp::server::TextResource{"snodec://style", "text/plain", "style"}}});
    });
    server.prompt({"review"}, [](const auto&, const auto& reply) {
        reply->result(mcp::server::PromptResult{});
    });
    server.resource({"snodec://slow", "slow"}, [&pendingResource](const auto&, const auto& reply) {
        pendingResource = reply;
    });
    Json response;
    int messages = 0;
    auto output = [&response, &messages](const Json& value, bool) {
        response = value;
        if (!value.is_null()) {
            ++messages;
        }
    };
    Access::registry(server).dispatch(request("server/discover"), mcp::server::detail::ProtocolEra::PerRequest20260728, output);
    test.expectTrue(response["result"]["supportedVersions"] ==
                        Json::array({mcp::server::detail::protocolVersion2026, mcp::server::detail::protocolVersion2025}),
                    "versions");
    test.expectTrue(response["result"]["capabilities"] ==
                        Json({{"tools", Json::object()}, {"resources", Json::object()}, {"prompts", Json::object()}}),
                    "capabilities derived without subscribe/listChanged");
    test.expectTrue(response["result"]["resultType"] == "complete" && response["result"]["ttlMs"] == 0 &&
                        response["result"]["cacheScope"] == "private",
                    "modern result/cache contract");
    Access::registry(server).dispatch(request("tools/list"), mcp::server::detail::ProtocolEra::PerRequest20260728, output);
    test.expectEqual(2, static_cast<int>(response["result"]["tools"].size()), "populated tool registry");
    const int before = messages;
    Access::registry(server).dispatch(request("tools/call", {{"name", "echo"}, {"arguments", {{"word", "é"}}}}),
                                      mcp::server::detail::ProtocolEra::PerRequest20260728,
                                      output);
    test.expectEqual(before + 1, messages, "only one terminal response");
    test.expectEqual(1, calls, "valid handler invoked");
    Access::registry(server).dispatch(request("tools/call", {{"name", "echo"}, {"arguments", {{"word", "too long"}}}}),
                                      mcp::server::detail::ProtocolEra::PerRequest20260728,
                                      output);
    test.expectTrue(response["result"]["isError"] == true && !response.contains("error"), "input-value failure is a tool error");
    test.expectEqual(1, calls, "invalid arguments never reach handler");
    for (const auto& [method, code] : std::map<std::string, int>{{"ping", -32601}, {"unknown", -32601}, {"tools/call", -32602}}) {
        Access::registry(server).dispatch(request(method), mcp::server::detail::ProtocolEra::PerRequest20260728, output);
        test.expectEqual(code, response["error"]["code"].get<int>(), "protocol error mapping");
    }
    Json noMeta = request("tools/list");
    noMeta["params"].erase("_meta");
    Access::registry(server).dispatch(noMeta, mcp::server::detail::ProtocolEra::PerRequest20260728, output);
    test.expectEqual(-32602, response["error"]["code"].get<int>(), "no inherited modern capabilities");
    Json unsupported = request("tools/list");
    unsupported["params"]["_meta"][mcp::server::detail::versionKey] = "other";
    Access::registry(server).dispatch(unsupported, mcp::server::detail::ProtocolEra::PerRequest20260728, output);
    test.expectEqual(-32022, response["error"]["code"].get<int>(), "unsupported version code");
    test.expectTrue(response["error"]["data"].contains("supported") && response["error"]["data"]["requested"] == "other",
                    "version error data");
    for (const Json& malformed : Json::array({nullptr,
                                              Json::array(),
                                              {{"jsonrpc", "2.0"}, {"method", "x"}, {"id", nullptr}},
                                              {{"jsonrpc", "1"}, {"method", "x"}, {"id", 1}},
                                              {{"jsonrpc", "2.0"}, {"method", "x"}, {"params", Json::array()}}})) {
        Access::registry(server).dispatch(malformed, mcp::server::detail::ProtocolEra::PerRequest20260728, output);
        test.expectEqual(-32600, response["error"]["code"].get<int>(), "invalid envelopes");
    }
    const Json initialize{
        {"jsonrpc", "2.0"},
        {"method", "initialize"},
        {"id", 1},
        {"params",
         {{"protocolVersion", "2025-06-18"}, {"capabilities", Json::object()}, {"clientInfo", {{"name", "test"}, {"version", "1"}}}}}};
    Access::registry(server).dispatch(initialize, mcp::server::detail::ProtocolEra::Initialization20251125, output);
    test.expectTrue(response["result"]["protocolVersion"] == mcp::server::detail::protocolVersion2025 &&
                        !response["result"].contains("resultType"),
                    "legacy normalization");
    Json malformedInitialize = initialize;
    malformedInitialize["params"].erase("clientInfo");
    Access::registry(server).dispatch(malformedInitialize, mcp::server::detail::ProtocolEra::Initialization20251125, output);
    test.expectEqual(-32602, response["error"]["code"].get<int>(), "legacy initialize requires client information");
    Access::registry(server).dispatch(request("server/discover"), mcp::server::detail::ProtocolEra::Initialization20251125, output);
    test.expectEqual(-32601, response["error"]["code"].get<int>(), "discover belongs only to the modern era");
    Access::registry(server).dispatch(
        request("resources/read", {{"uri", "missing"}}), mcp::server::detail::ProtocolEra::Initialization20251125, output);
    test.expectEqual(-32002, response["error"]["code"].get<int>(), "legacy resource error");
    Access::registry(server).dispatch(
        request("resources/read", {{"uri", "missing"}}), mcp::server::detail::ProtocolEra::PerRequest20260728, output);
    test.expectEqual(-32602, response["error"]["code"].get<int>(), "modern resource error remains strict");
    Access::registry(server).dispatch(request("tools/list"), mcp::server::detail::ProtocolEra::PerRequest20260728, output);
    test.expectTrue(response["result"].contains("resultType"), "legacy dispatch never changes modern contract");
    const int notified = messages;
    Access::registry(server).dispatch(
        {{"jsonrpc", "2.0"}, {"method", "notifications/initialized"}}, mcp::server::detail::ProtocolEra::Initialization20251125, output);
    test.expectEqual(notified, messages, "no notification response");
    Access::registry(server).dispatch(request("resources/read", {{"uri", "snodec://slow"}}),
                                      mcp::server::detail::ProtocolEra::PerRequest20260728,
                                      output,
                                      utils::Timeval(0));
    Access::response(*pendingResource).expire();
    test.expectEqual(-32603, response["error"]["code"].get<int>(), "resource deadline is a JSON-RPC error");
    Access::registry(server).dispatch(
        request("tools/call", {{"name", "pending"}}), mcp::server::detail::ProtocolEra::PerRequest20260728, output, utils::Timeval(0));
    Access::response(*pending).expire();
    test.expectTrue(response["result"]["isError"] == true, "tool deadline is a tool error");
    Access::registry(server).dispatch(
        request("tools/call", {{"name", "pending"}}), mcp::server::detail::ProtocolEra::PerRequest20260728, output);
    Access::response(*pending).cancel();
    const int cancelled = messages;
    pending->result(ToolResult::content({}));
    test.expectEqual(cancelled, messages, "cancel prevents late result");
    auto subscription =
        Access::registry(server).dispatch(request("subscriptions/listen", {{"notifications", {{"toolsListChanged", true}}}}),
                                          mcp::server::detail::ProtocolEra::PerRequest20260728,
                                          output);
    test.expectTrue(response["method"] == "notifications/subscriptions/acknowledged" && response["params"]["notifications"].empty() &&
                        response["params"]["_meta"][mcp::server::detail::subscriptionKey] == 7,
                    "empty honored filter and correlated ack");
    std::vector<Json> shutdown;
    auto ending = Access::registry(server).dispatch(
        request("subscriptions/listen"), mcp::server::detail::ProtocolEra::PerRequest20260728, [&shutdown](const Json& value, bool) {
            shutdown.push_back(value);
        });
    ending->cancel(mcp::server::CancelReason::Shutdown);
    test.expectTrue(shutdown.size() == 3 && shutdown[1]["method"] == "notifications/cancelled" && shutdown.back().is_null(),
                    "subscription teardown cancellation");
    subscription->cancel();
    Server invalid("invalid", "test-version");
    bool rejected = false;
    try {
        invalid.tool({"bad", {}, Json::parse(R"json(
{
    "$ref": "unsafe",
    "type": "object"
}
)json")},
                     [](const auto&, const auto&) {
                     });
    } catch (const mcp::server::RegistrationError& error) {
        rejected = std::string(error.what()).find("tool bad: inputSchema:") != std::string::npos;
    }
    test.expectTrue(rejected, "unsupported schema is rejected immediately with its registration name");
    rejected = false;
    try {
        server.tool({"echo", {}, schema}, [](const auto&, const auto&) {
        });
    } catch (const mcp::server::RegistrationError& error) {
        rejected = std::string(error.what()).find("duplicate registration") != std::string::npos;
    }
    test.expectTrue(rejected, "duplicate registration rejected immediately");
    core::SNodeC::free();
    return test.processResult();
}
