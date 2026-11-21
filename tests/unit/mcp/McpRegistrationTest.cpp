// SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
#include "core/SNodeC.h"
#include "mcp/server/detail/Server.h"
#include "tests/support/TestResult.h"
#include <functional>
#include <string>

using namespace mcp::server;
int main(int argc, char* argv[]) {
    const auto objectSchema = mcp::server::Json::parse(R"json(
{
    "type": "object"
}
)json");
    core::SNodeC::init(argc, argv);
    tests::support::TestResult test;
    Server server("owned-name", "application-version", "server instructions");
    const Json schema = objectSchema;
    const Server::ToolHandler handler = [](const auto&, const auto& reply) {
        reply->result(ToolResult::text("ok"));
    };
    const auto reject = [&](const std::function<void()>& attempt, const std::string& diagnostic) {
        bool rejected = false;
        try {
            attempt();
        } catch (const RegistrationError& error) {
            rejected = std::string(error.what()).find(diagnostic) != std::string::npos;
        }
        test.expectTrue(rejected, diagnostic);
    };
    reject(
        [&] {
            server.tool({"", {}, schema}, handler);
        },
        "tool <unnamed>: name:");
    reject(
        [&] {
            server.tool({"no-handler", {}, schema}, {});
        },
        "tool no-handler: handler:");
    reject(
        [&] {
            server.tool({"bad", {}, Json::parse(R"json(
{
    "type": "array"
}
)json")},
                        handler);
        },
        "tool bad: inputSchema:");
    reject(
        [&] {
            server.tool({"bad", {}, schema, Json::parse(R"json(
{
    "$ref": "unsupported"
}
)json")},
                        handler);
        },
        "tool bad: outputSchema:");
    reject(
        [&] {
            server.resource({"", "resource"}, [](const auto&, const auto&) {
            });
        },
        "resource <unnamed>: uri:");
    reject(
        [&] {
            server.prompt({"bad", {}, {{"same", {}, Requirement::Optional}, {"same", {}, Requirement::Required}}},
                          [](const auto&, const auto&) {
                          });
        },
        "prompt bad: arguments.name:");
    ToolDefinition definition{"echo", "description", schema, {}, "display title", Json{{"readOnlyHint", true}}};
    test.expectTrue(server.tool(definition, handler) == &server, "registration returns its registry for chaining");
    definition.name = "mutated after registration";
    reject(
        [&] {
            server.tool({"echo", {}, schema}, handler);
        },
        "duplicate registration");
    int promptCalls = 0;
    server.prompt({"prompt", {}, {{"code", "source code", Requirement::Required}}}, [&](const PromptGet& call, const auto& reply) {
        ++promptCalls;
        reply->result(PromptResult{
            "expanded prompt",
            {{Role::User, TextContent{call.arguments.at("code"), {}}}, {Role::Assistant, ImageContent{"AA==", "image/png", {}}}}});
    });
    server.resource({"test://resource", "resource"}, [](const ResourceRead& read, const auto& reply) {
        reply->result(ResourceResult{
            {TextResource{read.uri, "text/plain", "exact bytes"}, BlobResource{read.uri, "application/octet-stream", "AA=="}}});
    });
    const auto call = [&](const std::string& method, Json params = Json::object(), bool legacy = false) {
        Json response;
        if (!legacy) {
            params["_meta"] = {{detail::versionKey, detail::protocolVersion2026}, {detail::capabilitiesKey, Json::object()}};
        }
        detail::Access::registry(server).dispatch({{"jsonrpc", "2.0"}, {"id", 1}, {"method", method}, {"params", params}},
                                                  legacy ? detail::ProtocolEra::Initialization20251125
                                                         : detail::ProtocolEra::PerRequest20260728,
                                                  [&](const Json& value, bool) {
                                                      response = value;
                                                  });
        return response;
    };
    Json response = call("server/discover");
    test.expectTrue(response["result"]["instructions"] == "server instructions" &&
                        response["result"]["_meta"]["io.modelcontextprotocol/serverInfo"]["version"] == "application-version",
                    "modern discovery uses application identity and instructions");
    response = call("initialize",
                    {{"protocolVersion", detail::protocolVersion2025},
                     {"capabilities", Json::object()},
                     {"clientInfo", {{"name", "client"}, {"version", "1"}}}},
                    true);
    test.expectTrue(response["result"]["serverInfo"]["name"] == "owned-name" &&
                        response["result"]["serverInfo"]["version"] == "application-version" &&
                        response["result"]["instructions"] == "server instructions",
                    "2025 initialize owns identity and instructions");
    response = call("tools/list");
    test.expectTrue(response["result"]["tools"].size() == 1 && response["result"]["tools"][0]["name"] == "echo" &&
                        response["result"]["tools"][0]["title"] == "display title",
                    "failed registrations never modify the registry");
    response = call("prompts/get", {{"name", "prompt"}});
    test.expectTrue(response["error"]["code"] == -32602 && promptCalls == 0, "required prompt arguments checked before handler");
    response = call("prompts/get", {{"name", "prompt"}, {"arguments", {{"code", "header text"}}}});
    test.expectTrue(promptCalls == 1 && response["result"]["description"] == "expanded prompt" &&
                        response["result"]["messages"][0]["content"]["text"] == "header text" &&
                        response["result"]["messages"][1]["role"] == "assistant",
                    "typed prompt expands multiple content kinds");
    response = call("resources/read", {{"uri", "test://resource"}});
    test.expectTrue(response["result"]["contents"][0]["text"] == "exact bytes" && response["result"]["contents"][1]["blob"] == "AA==",
                    "typed resource preserves text and blob entries");
    detail::Access::registry(server).seal();
    detail::Access::registry(server).seal();
    reject(
        [&] {
            server.tool({"later", {}, schema}, handler);
        },
        "tool later: registration: registration rejected: registry is sealed");
    core::SNodeC::free();
    return test.processResult();
}
