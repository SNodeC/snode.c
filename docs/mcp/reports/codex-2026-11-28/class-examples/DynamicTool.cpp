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

#include "core/timer/Timer.h"
#include "mcp/Protocol.h"
#include "mcp/Types.h"
#include "mcp/server/Reply.h"
#include "mcp/server/Request.h"
#include "mcp/server/Results.h"
#include "mcp/server/Server.h"
#include "utils/Uuid.h"
#include "utils/base64.h"

#include <cmath>
#include <cstdint>
#include <functional>
#include <initializer_list>
#include <map>
#include <memory>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace apps::mcp::features {

    using namespace ::mcp::server;

std::string encoded(const std::string& bytes) {
    return base64::base64_encode(reinterpret_cast<const unsigned char*>(bytes.data()), bytes.size());
}

Icon icon() {
    const std::string svg =
        R"svg(<svg xmlns="http://www.w3.org/2000/svg" width="16" height="16"><rect width="16" height="16" fill="#247"/><circle cx="8" cy="8" r="5" fill="#fff"/></svg>)svg";
    return Icon{"data:image/svg+xml;base64," + encoded(svg), "image/svg+xml", {"16x16"}, IconTheme::Light};
}

class CatalogueTool final : public Tool {
public:
    explicit CatalogueTool(std::string name)
        : Tool({.name = name,
                .description = "Catalogue entry demonstrating pagination and dynamic registration.",
                .inputSchema = Json::parse(R"json(
{
    "type": "object",
    "additionalProperties": false
}
)json"),
                .title = name,
                .annotations =
                    Json::parse(R"json(
{
    "readOnlyHint": true,
    "destructiveHint": false,
    "idempotentHint": true,
    "openWorldHint": false
}
)json"),
                .icons = {icon()},
                .meta = {
                    {"showcase/feature", name}
                }}) {
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

class DynamicTool final : public Tool {
public:
    explicit DynamicTool(Server& server)
        : Tool({.name = "dynamic",
                .description = "Add/remove all four registration kinds; coalesced list_changed notifications.",
                .inputSchema = Json::parse(R"json(
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
)json"),
                .title = "dynamic",
                .annotations =
                    Json::parse(R"json(
{
    "readOnlyHint": false,
    "destructiveHint": true,
    "idempotentHint": false,
    "openWorldHint": false
}
)json"),
                .icons = {icon()},
                .meta = Json::parse(R"json(
{
    "showcase/feature": "dynamic"
}
)json")})
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

} // namespace apps::mcp::features
