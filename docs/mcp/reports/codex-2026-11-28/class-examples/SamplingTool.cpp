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

ToolResult clientInputResult(const RequestContext& context, const std::string& key, Json value) {
    auto result = ToolResult::json(std::move(value));
    if (const auto progress = context.getClientProgress(key)) {
        result.setMeta({
            {"showcase/clientProgress", progress->value}
        });
    }
    return result;
}

class SamplingTool final : public Tool {
public:
    SamplingTool()
        : Tool({.name = "sampling",
                .description = "Client-controlled sampling, media, context, model options and a complete tool loop.",
                .inputSchema = Json::parse(R"json(
{
    "type": "object",
    "properties": {
        "toolUse": {
            "type": "boolean"
        },
        "toolLoop": {
            "type": "boolean"
        },
        "toolChoice": {
            "enum": [
                "auto",
                "none",
                "required"
            ]
        },
        "context": {
            "enum": [
                "none",
                "thisServer",
                "allServers"
            ]
        },
        "media": {
            "enum": [
                "text",
                "image",
                "audio"
            ]
        },
        "options": {
            "type": "boolean"
        }
    }
}
)json"),
                .title = "sampling",
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
                .meta = Json::parse(R"json(
{
    "showcase/feature": "sampling"
}
)json")}) {
    }

    void call(const ToolCall& request, const std::shared_ptr<ToolReply>& reply) final {
        request.context.trackClientProgress("sample");
        request.context.trackClientProgress("final");
        Json content{
            {"type", "text"},
            {"text", "Return the demonstration answer 42."}
        };
        const std::string media = request.arguments.value("media", "text");
        if (media != "text") {
            content = {
                {"type", media},
                {"data", media == "image"
                            ? "iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAQAAAC1HAwCAAAAC0lEQVR42mP8/x8AAwMCAO+ip1sAAAAASUVORK5CYII="
                            : "UklGRiYAAABXQVZFZm10IBAAAAABAAEARKwAAIhYAQACABAAZGF0YQIAAAAAAA=="},
                {"mimeType", media == "image" ? "image/png" : "audio/wav"}
            };
        }
        Json params{
            {"messages", {{{"role", "user"}, {"content", content}}}},
            {"maxTokens", 64},
            {"includeContext", request.arguments.value("context", "none")}
        };
        const bool loop = request.arguments.value("toolLoop", false);
        if (request.arguments.value("toolUse", false) || loop || request.arguments.contains("toolChoice")) {
            params["tools"] = {
                {
                    {"name", "answer"},
                    {"inputSchema", Json{{"type", "object"}, {"additionalProperties", false}}},
                    {"outputSchema", {
                        {"type", "object"},
                        {"properties", {
                            {"answer", {
                                {"type", "integer"}
                            }}
                        }},
                        {"required", {"answer"}}
                    }}
                }};
            params["toolChoice"] = {
                {"mode", loop ? "required" : request.arguments.value("toolChoice", "required")}
            };
        }
        if (request.arguments.value("options", false)) {
            params["systemPrompt"] = "Demonstration only; do not access external data.";
            params["temperature"] = 0.25;
            params["stopSequences"] = {"END"};
            params["modelPreferences"] = {
                {"hints", {{{"name", "demonstration"}}}},
                {"costPriority", 0.5},
                {"speedPriority", 0.5},
                {"intelligencePriority", 0.5}
            };
            params["metadata"] = {
                {"showcase", true}
            };
        }
        const ::mcp::SamplingRequest sampling(params);
        const auto& state = request.context.getRequestState();
        const auto firstResult = loop && state && state->starts_with("{")
                                     ? std::optional<::mcp::SamplingResult>(::mcp::SamplingResult(Json::parse(*state)))
                                     : request.context.getSamplingResponse("sample", sampling);
        if (loop && firstResult) {
            const Json first = firstResult->toJson();
            params["messages"].push_back({
                {"role", "assistant"},
                {"content", first.at("content")}
            });
            const Json blocks = first.at("content").is_array() ? first.at("content") : Json::array({first.at("content")});
            Json results = Json::array();
            for (const auto& block : blocks) {
                if (block.at("type") == "tool_use") {
                    results.push_back({
                        {"type", "tool_result"},
                        {"toolUseId", block.at("id")},
                        {"content", {{{"type", "text"}, {"text", "42"}}}},
                        {"structuredContent", {
                            {"answer", 42}
                        }}
                    });
                }
            }
            params["messages"].push_back({
                {"role", "user"},
                {"content", results}
            });
            params["toolChoice"] = {
                {"mode", "none"}
            };
            const ::mcp::SamplingRequest final(params);
            if (const auto response = request.context.getSamplingResponse("final", final)) {
                reply->result(clientInputResult(request.context, "final", response->toJson()));
            } else {
                reply->clientInputRequired({{
                    {"final", final}
                }, first.dump(4)});
            }
        } else if (firstResult) {
            reply->result(clientInputResult(request.context, "sample", firstResult->toJson()));
        } else {
            reply->clientInputRequired({{
                {"sample", sampling}
            }, "sampling"});
        }
    }
};

} // namespace apps::mcp::features
