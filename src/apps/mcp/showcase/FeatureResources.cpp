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

#include "Features.h"
#include "mcp/Types.h"
#include "mcp/server/Reply.h"
#include "mcp/server/Request.h"
#include "mcp/server/Results.h"
#include "mcp/server/Server.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cmath>
#include <cstdint>
#include <initializer_list>
#include <map>
#include <memory>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#endif

namespace apps::mcp::features {

    using namespace ::mcp::server;

    namespace {

        ResourceDefinition textDefinition() {
            ResourceDefinition text{
                "showcase://text", "text", "Text resource, metadata and explicit public caching.", "text/plain", "Showcase text", {icon()}};
            text.annotations = {{Role::User}, 0.7, "2026-01-01T00:00:00Z"};
            text.meta = {{"showcase/resource", "text"}};
            return text;
        }

        class TextResourceItem final : public Resource {
        public:
            TextResourceItem()
                : Resource(textDefinition()) {
            }

            void read(const ResourceRead& call, const std::shared_ptr<ResourceReply>& reply) override {
                reply->result(ResourceResult{{TextResource{call.uri, "text/plain", "SNode.C\n", {{"showcase/content", true}}}},
                                             {{"showcase/result", true}},
                                             CacheHints{1000, CacheScope::Public}});
            }
        };

        class BlobResourceItem final : public Resource {
        public:
            BlobResourceItem()
                : Resource({"showcase://blob", "blob", "Generated binary resource with private cache hints.", "application/octet-stream"}) {
            }

            void read(const ResourceRead& call, const std::shared_ptr<ResourceReply>& reply) override {
                reply->result(ResourceResult{{BlobResource{call.uri, "application/octet-stream", encoded(std::string("\0\1\2", 3))}},
                                             {},
                                             CacheHints{500, CacheScope::Private}});
            }
        };

        class CounterResource final : public Resource {
        public:
            explicit CounterResource(Server& server)
                : Resource({"showcase://counter", "counter", "Changing resource; subscribe/listen before calling update.", "text/plain"})
                , server(server) {
            }

            void read(const ResourceRead& call, const std::shared_ptr<ResourceReply>& reply) override {
                reply->result(ResourceResult{{TextResource{call.uri, "text/plain", std::to_string(revision)}}});
            }

            std::uint64_t update() {
                ++revision;
                server.resourceUpdated(getDefinition().uri);
                return revision;
            }

        private:
            Server& server;
            std::uint64_t revision = 0;
        };

        ToolDefinition updateDefinition() {
            auto tool = toolDefinition("update", "Increment the resource and emit resourceUpdated to subscribed consumers.", emptySchema());
            tool.annotations["readOnlyHint"] = false;
            tool.annotations["idempotentHint"] = false;
            return tool;
        }

        class UpdateTool final : public Tool {
        public:
            explicit UpdateTool(std::shared_ptr<CounterResource> counter)
                : Tool(updateDefinition())
                , counter(std::move(counter)) {
            }

            void call(const ToolCall&, const std::shared_ptr<ToolReply>& reply) override {
                reply->result(ToolResult::json({{"revision", counter->update()}}));
            }

        private:
            std::shared_ptr<CounterResource> counter;
        };

        void completeName(const CompletionRequest& call, const std::shared_ptr<CompletionReply>& reply) {
            CompletionResult result;
            const std::string prefix = call.arguments.contains("prefix") ? call.arguments.at("prefix") : std::string{};
            for (const auto& value : {"alpha", "beta", "gamma"}) {
                if (std::string(value).starts_with(call.value)) {
                    result.values.push_back(prefix + value);
                }
            }
            result.total = result.values.size();
            result.meta = {{"showcase/completion", true}};
            reply->result(std::move(result));
        }

        class GreetingTemplate final : public ResourceTemplate {
        public:
            GreetingTemplate()
                : ResourceTemplate({"showcase://greeting/{name}",
                                    "greeting",
                                    "Template variables and contextual completion.",
                                    "text/plain",
                                    {},
                                    {{"name", completeName}}}) {
            }

            void read(const ResourceRead& call, const std::shared_ptr<ResourceReply>& reply) override {
                reply->result(ResourceResult{{TextResource{call.uri, "text/plain", "Hello " + call.variables.at("name")}}});
            }
        };

        class PathTemplate final : public ResourceTemplate {
        public:
            PathTemplate()
                : ResourceTemplate({"showcase://path/{+path}", "path", "Reserved URI-template expansion.", "text/plain"}) {
            }

            void read(const ResourceRead& call, const std::shared_ptr<ResourceReply>& reply) override {
                reply->result(ResourceResult{{TextResource{call.uri, "text/plain", call.variables.at("path")}}});
            }
        };

        class ConversationPrompt final : public Prompt {
        public:
            ConversationPrompt()
                : Prompt({"conversation",
                          "Prompt arguments, completion, roles, multiple messages and embedded resources.",
                          {{"name", "Name to greet", Requirement::Required, completeName}}}) {
            }

            void get(const PromptGet& call, const std::shared_ptr<PromptReply>& reply) override {
                reply->result(
                    PromptResult{"A generated multi-message conversation.",
                                 {{Role::User, TextContent{"Hello " + call.arguments.at("name"), {}}},
                                  {Role::Assistant, EmbeddedResource{TextResource{"showcase://text", "text/plain", "SNode.C"}, {}}}},
                                 {{"showcase/prompt", true}}});
            }
        };

        class CatalogueTool final : public Tool {
        public:
            explicit CatalogueTool(std::string name)
                : Tool(toolDefinition(name, "Catalogue entry demonstrating pagination and dynamic registration.", emptySchema())) {
            }

            void call(const ToolCall&, const std::shared_ptr<ToolReply>& reply) override {
                reply->result(ToolResult::text(getDefinition().name));
            }
        };

        class CatalogueResource final : public Resource {
        public:
            CatalogueResource(const std::string& uri, const std::string& name)
                : Resource({uri, name, "Catalogue entry demonstrating pagination and dynamic registration.", "text/plain"}) {
            }

            void read(const ResourceRead& call, const std::shared_ptr<ResourceReply>& reply) override {
                reply->result(ResourceResult{{TextResource{call.uri, "text/plain", getDefinition().name}}});
            }
        };

        class CataloguePrompt final : public Prompt {
        public:
            explicit CataloguePrompt(const std::string& name)
                : Prompt({name, "Catalogue entry demonstrating pagination and dynamic registration."}) {
            }

            void get(const PromptGet&, const std::shared_ptr<PromptReply>& reply) override {
                reply->result(PromptResult{{}, {{Role::User, TextContent{getDefinition().name, {}}}}});
            }
        };

        class CatalogueTemplate final : public ResourceTemplate {
        public:
            CatalogueTemplate(const std::string& uri, const std::string& name)
                : ResourceTemplate({uri, name, "Catalogue entry demonstrating pagination and dynamic registration.", "text/plain"}) {
            }

            void read(const ResourceRead& call, const std::shared_ptr<ResourceReply>& reply) override {
                reply->result(ResourceResult{{TextResource{call.uri, "text/plain", call.variables.at("value")}}});
            }
        };

        ToolDefinition dynamicDefinition() {
            auto tool =
                toolDefinition("dynamic", "Add/remove all four registration kinds; coalesced list_changed notifications.", schema(R"json(
{
    "type": "object",
    "properties": {
        "action": {
            "enum": [
                "add",
                "remove"
            ]
        }
    },
    "required": [
        "action"
    ],
    "additionalProperties": false
}
)json"));
            tool.annotations["readOnlyHint"] = false;
            tool.annotations["destructiveHint"] = true;
            tool.annotations["idempotentHint"] = false;
            return tool;
        }

        class DynamicTool final : public Tool {
        public:
            explicit DynamicTool(Server& server)
                : Tool(dynamicDefinition())
                , server(server) {
            }

            void call(const ToolCall& call, const std::shared_ptr<ToolReply>& reply) override {
                if (call.arguments.at("action") == "add") {
                    server.tool(std::make_shared<CatalogueTool>("dynamic.tool"));
                    server.resource(std::make_shared<CatalogueResource>("showcase://dynamic", "dynamic"));
                    server.resourceTemplate(std::make_shared<CatalogueTemplate>("showcase://dynamic/{value}", "dynamic"));
                    server.prompt(std::make_shared<CataloguePrompt>("dynamic"));
                } else {
                    server.remove(RegistrationKind::Tool, "dynamic.tool");
                    server.remove(RegistrationKind::Resource, "showcase://dynamic");
                    server.remove(RegistrationKind::ResourceTemplate, "showcase://dynamic/{value}");
                    server.remove(RegistrationKind::Prompt, "dynamic");
                }
                reply->result(ToolResult::text(call.arguments.at("action")));
            }

        private:
            Server& server;
        };

    } // namespace

    void registerResources(Server& server) {
        server.resource(std::make_shared<TextResourceItem>());
        server.resource(std::make_shared<BlobResourceItem>());
        const auto counter = std::make_shared<CounterResource>(server);
        server.resource(counter);
        server.tool(std::make_shared<UpdateTool>(counter));
        server.resourceTemplate(std::make_shared<GreetingTemplate>());
        server.resourceTemplate(std::make_shared<PathTemplate>());
        server.prompt(std::make_shared<ConversationPrompt>());
        for (unsigned i = 0; i < 9; ++i) {
            const std::string number = std::to_string(i);
            server.tool(std::make_shared<CatalogueTool>("catalogue." + number));
            server.resource(std::make_shared<CatalogueResource>("showcase://catalogue/" + number, "catalogue." + number));
            server.prompt(std::make_shared<CataloguePrompt>("catalogue." + number));
            server.resourceTemplate(
                std::make_shared<CatalogueTemplate>("showcase://catalogue/" + number + "/{value}", "catalogue." + number));
        }
        server.tool(std::make_shared<DynamicTool>(server));
    }

} // namespace apps::mcp::features
