/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

// IWYU pragma: no_include <nlohmann/detail/json_ref.hpp>

#include "core/SNodeC.h"
#include "mcp/Protocol.h"
#include "mcp/Types.h"
#include "mcp/server/Reply.h"
#include "mcp/server/Request.h"
#include "mcp/server/Results.h"
#include "mcp/server/Server.h"
#include "mcp/server/detail/Server.h"
#include "tests/support/TestResult.h"

#include <cmath>
#include <map>
#include <optional>
#include <variant>
#include <vector>

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

using namespace mcp::server;

int main(int argc, char* argv[]) {
    const auto objectSchema = nlohmann::ordered_json::parse(R"json(
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
            server.tool({"bad", {}, nlohmann::ordered_json::parse(R"json(
{
    "type": "array"
}
)json")},
                        handler);
        },
        "tool bad: inputSchema:");
    reject(
        [&] {
            server.tool({"bad", {}, schema, nlohmann::ordered_json::parse(R"json(
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
    detail::Access::registry(server).attach();
    detail::Access::registry(server).attach();
    server.tool({"later", {}, schema}, handler);
    response = call("tools/list");
    test.expectTrue(response["result"]["tools"].size() == 2, "registration remains available after attachment");
    test.expectTrue(server.remove(RegistrationKind::Tool, "later") && !server.remove(RegistrationKind::Tool, "later"),
                    "runtime removal reports whether a registration existed");
    response = call("tools/list");
    test.expectTrue(response["result"]["tools"].size() == 1, "removed registration disappears from discovery");
    core::SNodeC::free();

    return test.processResult();
}
