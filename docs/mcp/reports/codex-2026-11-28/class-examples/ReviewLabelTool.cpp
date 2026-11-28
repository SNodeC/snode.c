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

#include "core/system/unistd.h"
#include "core/timer/Timer.h"
#include "mcp/Types.h"
#include "mcp/server/Config.h"
#include "mcp/server/Prompt.h"
#include "mcp/server/Reply.h"
#include "mcp/server/Request.h"
#include "mcp/server/Resource.h"
#include "mcp/server/ResourceTemplate.h"
#include "mcp/server/Results.h"
#include "mcp/server/Server.h"
#include "mcp/server/Tool.h"
#include "mcp/server/Types.h"
#include "utils/CLI11.hpp"

#include <algorithm>
#include <array>
#include <cerrno>
#include <cstddef>
#include <cstdio>
#include <deque>
#include <dirent.h>
#include <exception>
#include <fcntl.h>
#include <functional>
#include <map>
#include <memory>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>
#include <optional>
#include <stdexcept>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <utility>
#include <variant>
#include <vector>

#ifndef MCP_HEADER_ROOT
#define MCP_HEADER_ROOT "/usr/local/include/snode.c"
#endif

namespace apps::mcp {

    using ::mcp::server::Json;
    using ::mcp::server::ToolReply;
    using ::mcp::server::ToolResult;

::mcp::server::Json parseSchema(const std::string& name, const char* text, const char* field = "inputSchema") {
    ::mcp::server::Json schema;
    try {
        schema = nlohmann::ordered_json::parse(text);
    } catch (const ::mcp::server::Json::parse_error& error) {
        throw ::mcp::server::RegistrationError(::mcp::server::RegistrationKind::Tool, name, field, error.what());
    }

    return schema;
}

class ReviewLabelResource final : public ::mcp::server::Resource {
public:
    /// Borrow Server through this object's lifetime; construct on the event loop before attachment.
    explicit ReviewLabelResource(::mcp::server::Server& server)
        : Resource({"snodec://development/review-label",
                    "Current review label",
                    "The non-sensitive label selected with review.label.",
                    "text/plain"})
        , server(server) {
    }

    /// Replace the label on the event loop and notify subscribers only when it changes.
    void setLabel(std::string next) {
        if (label != next) {
            label = std::move(next);
            server.resourceUpdated(getDefinition().uri);
        }
    }

    /// Complete once with the current owned label.
    void read(const ::mcp::server::ResourceRead& request, const std::shared_ptr<::mcp::server::ResourceReply>& reply) override {
        reply->result(::mcp::server::ResourceResult{{::mcp::server::TextResource{request.uri, "text/plain", label}}});
    }

private:
    ::mcp::server::Server& server;
    std::string label;
};

class ReviewLabelTool final : public ::mcp::server::Tool {
public:
    /// Retain the resource through all requests; construct on the event loop before attachment.
    explicit ReviewLabelTool(std::shared_ptr<ReviewLabelResource> resource)
        : Tool({"review.label",
                "Choose a non-sensitive review label through elicitation. Does not modify files.",
                parseSchema("review.label", R"json(
{
    "type": "object",
    "additionalProperties": false
}
)json")})
        , resource(std::move(resource))
        , formSchema(parseSchema("review.label",
                                 R"json(
{
    "type": "object",
    "properties": {
        "label": {
            "type": "string",
            "maxLength": 128
        }
    },
    "required": [
        "label"
    ]
}
)json",
                                 "requestedSchema")) {
    }

    /// Borrow request during this call; complete once or request additional form input.
    void call(const ::mcp::server::ToolCall& request, const std::shared_ptr<::mcp::server::ToolReply>& reply) override {
        const auto& inputs = request.context.getInputResponses();
        const auto* input = request.context.getInputResponse("label", formSchema);
        if (inputs.empty()) {
            reply->inputRequired(::mcp::server::InputRequired{
                {
                    {"label", ::mcp::server::FormInput{"Choose a non-sensitive review label.", formSchema}}
                }, {}});
        } else if (input == nullptr || input->action != ::mcp::server::ElicitationAction::Accept) {
            reply->result(::mcp::server::ToolResult::text("No label selected."));
        } else {
            const auto& content = input->content;
            const auto value = content.is_object() ? content.value("label", ::mcp::server::Json()) : ::mcp::server::Json();
            if (!value.is_string() || value.get_ref<const std::string&>().size() > 128) {
                reply->error(::mcp::server::ErrorCode::InvalidParams, "label must be a string of at most 128 bytes");
            } else {
                resource->setLabel(value.get<std::string>());
                reply->result(::mcp::server::ToolResult::text("Review label selected."));
            }
        }
    }

private:
    std::shared_ptr<ReviewLabelResource> resource;
    ::mcp::server::Json formSchema;
};

} // namespace apps::mcp
