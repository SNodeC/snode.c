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

#include <initializer_list>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>
#include <type_traits>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

using namespace mcp::server;

namespace {

    struct EchoTool final : Tool {
        using Tool::Tool;

        void call(const ToolCall& request, const std::shared_ptr<ToolReply>& reply) override {
            reply->result(ToolResult::text(request.arguments.dump()));
        }
    };

    struct StyleResource final : Resource {
        using Resource::Resource;

        void read(const ResourceRead& request, const std::shared_ptr<ResourceReply>& reply) override {
            reply->result(ResourceResult{{TextResource{request.uri, "text/plain", "style"}}});
        }
    };

    struct ReviewPrompt final : Prompt {
        using Prompt::Prompt;

        void get(const PromptGet& request, const std::shared_ptr<PromptReply>& reply) override {
            reply->result(PromptResult{{}, {{Role::User, TextContent{request.arguments.at("code"), {}}}}});
        }
    };

} // namespace

static_assert(!std::is_copy_assignable_v<EchoTool>);

int main(int argc, char* argv[]) {
    const auto objectSchema = nlohmann::ordered_json::parse(R"json(
{
    "type": "object"
}
)json");
    core::SNodeC::init(argc, argv);
    tests::support::TestResult test;
    const Json schema = objectSchema;
    const ToolDefinition definition{"echo", "owned definition", schema};
    std::weak_ptr<Tool> retained;
    {
        Server classes("same", "1");
        Server lambdas("same", "1");
        auto tool = std::make_shared<EchoTool>(definition);
        retained = tool;
        test.expectTrue(&tool->getDefinition() != &definition && tool->getDefinition().name == definition.name,
                        "class owns its immutable registration definition");
        test.expectTrue(classes.tool(tool) == &classes, "class registration chains");
        tool.reset();
        test.expectTrue(!retained.expired(), "server retains the registered instance");
        classes.resource(std::make_shared<StyleResource>(ResourceDefinition{"test://style", "style", {}, "text/plain"}));
        classes.prompt(std::make_shared<ReviewPrompt>(PromptDefinition{"review", {}, {{"code", {}, Requirement::Required}}}));
        lambdas.tool(definition, [](const auto& request, const auto& reply) {
            reply->result(ToolResult::text(request.arguments.dump()));
        });
        lambdas.resource({"test://style", "style", {}, "text/plain"}, [](const auto& request, const auto& reply) {
            reply->result(ResourceResult{{TextResource{request.uri, "text/plain", "style"}}});
        });
        lambdas.prompt({"review", {}, {{"code", {}, Requirement::Required}}}, [](const auto& request, const auto& reply) {
            reply->result(PromptResult{{}, {{Role::User, TextContent{request.arguments.at("code"), {}}}}});
        });
        for (bool modern : {false, true}) {
            for (const auto& [method, input] :
                 std::map<std::string, Json>{{"tools/list", Json::object()},
                                             {"resources/list", Json::object()},
                                             {"prompts/list", Json::object()},
                                             {"tools/call", {{"name", "echo"}, {"arguments", {{"x", 1}}}}},
                                             {"resources/read", {{"uri", "test://style"}}},
                                             {"prompts/get", {{"name", "review"}, {"arguments", {{"code", "x"}}}}}}) {
                Json params = input;
                Json actual;
                Json expected;
                if (modern) {
                    params["_meta"] = {{detail::versionKey, detail::protocolVersion2026}, {detail::capabilitiesKey, Json::object()}};
                }

                const Json request{{"jsonrpc", "2.0"}, {"id", 1}, {"method", method}, {"params", params}};
                const auto era = modern ? detail::ProtocolEra::PerRequest20260728 : detail::ProtocolEra::Initialization20251125;
                detail::Access::registry(classes).dispatch(request, era, [&](const Json& value, bool) {
                    actual = value;
                });
                detail::Access::registry(lambdas).dispatch(request, era, [&](const Json& value, bool) {
                    expected = value;
                });
                test.expectTrue(actual.dump() == expected.dump(), method + " class/lambda wire equivalence");
            }
        }

        const auto reject = [&](auto registration, const std::string& diagnostic) {
            bool rejected = false;
            try {
                registration();
            } catch (const RegistrationError& error) {
                rejected = std::string(error.what()).find(diagnostic) != std::string::npos;
            }
            test.expectTrue(rejected, diagnostic);
        };
        reject(
            [&] {
                classes.tool(definition, [](const auto&, const auto&) {
                });
            },
            "tool echo: name: registration rejected: duplicate");
        reject(
            [&] {
                lambdas.tool(std::make_shared<EchoTool>(definition));
            },
            "tool echo: name: registration rejected: duplicate");
        reject(
            [&] {
                classes.tool(std::make_shared<EchoTool>(ToolDefinition{"bad", {}, nlohmann::ordered_json::parse(R"json(
{
    "type": "array"
}
)json")}));
            },
            "tool bad: inputSchema:");
    }
    test.expectTrue(retained.expired(), "registered instance released with its server");
    core::SNodeC::free();

    return test.processResult();
}
