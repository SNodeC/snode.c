// SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
#include "core/SNodeC.h"
#include "mcp/server/detail/Server.h"
#include "tests/support/TestResult.h"

#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>

using namespace mcp::server;
int main(int argc, char* argv[]) {
    const auto objectSchema = nlohmann::ordered_json::parse(R"json(
{
    "type": "object"
}
)json");
    core::SNodeC::init(argc, argv);
    tests::support::TestResult test;
    Server server("results", "test-version");
    ToolResult next = ToolResult::text("header bytes\n");
    server.tool({"value", {}, objectSchema}, [&next](const auto&, const auto& reply) {
        reply->result(next);
    });
    Json response;
    const auto call = [&](bool legacy) {
        Json params{{"name", "value"}};
        if (!legacy) {
            params["_meta"] = {{detail::versionKey, detail::protocolVersion2026}, {detail::capabilitiesKey, Json::object()}};
        }
        detail::Access::registry(server).dispatch({{"jsonrpc", "2.0"}, {"id", 1}, {"method", "tools/call"}, {"params", params}},
                                                  legacy ? detail::ProtocolEra::Initialization20251125
                                                         : detail::ProtocolEra::PerRequest20260728,
                                                  [&response](const Json& value, bool) {
                                                      response = value;
                                                  });
    };
    for (bool legacy : {false, true}) {
        call(legacy);
        test.expectTrue(response["result"]["content"][0]["text"] == "header bytes\n", "exact text bytes");
        test.expectTrue(!response["result"].contains("structuredContent"), "text has no structured content");
        for (const Json& value : Json::array({Json{{"text", "same bytes"}}, Json::array({1, "two"}), 42, "text", nullptr})) {
            next = ToolResult::json(value);
            call(legacy);
            test.expectTrue(response["result"]["content"][0]["text"] == value.dump(), "JSON text comes from same value");
            test.expectTrue(response["result"].contains("structuredContent") == (!legacy || value.is_object()),
                            "non-object JSON encoding follows protocol era");
            if (!legacy || value.is_object()) {
                test.expectTrue(response["result"]["structuredContent"] == value, "JSON wire type preserved");
            }
        }
        next = ToolResult::toolError("failure");
        call(legacy);
        test.expectTrue(response["result"]["isError"] == true && response["result"]["content"][0]["text"] == "failure",
                        "tool failure is not a JSON-RPC error");
        next = ToolResult::text("header bytes\n");
    }
    const Annotations annotations{{Role::User, Role::Assistant}, 0.5, "2026-11-21T00:00:00Z"};
    next = ToolResult::content({TextContent{"text", annotations},
                                ImageContent{"aW1hZ2U=", "image/png", {}},
                                AudioContent{"YXVkaW8=", "audio/wav", {}},
                                ResourceLink{"test://link", "link", "description", "text/plain", 4, {}},
                                EmbeddedResource{TextResource{"test://text", "text/plain", "resource"}, {}},
                                EmbeddedResource{BlobResource{"test://blob", "application/octet-stream", "AA=="}, {}}});
    call(false);
    const Json& blocks = response["result"]["content"];
    test.expectTrue(blocks.size() == 6 && blocks[0]["annotations"]["audience"] == Json::array({"user", "assistant"}) &&
                        blocks[0]["annotations"]["priority"] == 0.5 && blocks[0]["annotations"].contains("lastModified"),
                    "mixed content preserves annotations");
    test.expectTrue(blocks[1]["type"] == "image" && blocks[1]["data"] == "aW1hZ2U=" && blocks[2]["type"] == "audio" &&
                        blocks[3]["type"] == "resource_link" && blocks[3]["size"] == 4 && blocks[4]["resource"]["text"] == "resource" &&
                        blocks[5]["resource"]["blob"] == "AA==",
                    "all tool content kinds encoded");
    Server schema("schema", "test-version");
    schema.tool({"value", {}, objectSchema, nlohmann::ordered_json::parse(R"json(
{
    "type": "integer"
}
)json")},
                [](const auto&, const auto& reply) {
                    reply->result(ToolResult::json("wrong type"));
                });
    detail::Access::registry(schema).dispatch({{"jsonrpc", "2.0"}, {"id", 2}, {"method", "tools/call"}, {"params", {{"name", "value"}}}},
                                              detail::ProtocolEra::Initialization20251125,
                                              [&response](const Json& value, bool) {
                                                  response = value;
                                              });
    test.expectTrue(response["result"]["isError"] == true, "legacy validates canonical value before content-only encoding");
    core::SNodeC::free();
    return test.processResult();
}
