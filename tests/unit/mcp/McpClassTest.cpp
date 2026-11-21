// SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
#include "core/SNodeC.h"
#include "mcp/server/detail/Server.h"
#include "tests/support/TestResult.h"

#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>
#include <type_traits>

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
        Server classes("same", "1"), lambdas("same", "1");
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
                Json params = input, actual, expected;
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
