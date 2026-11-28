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

Json labelSchema() {
    return Json::parse(R"json(
{
    "type": "object",
    "properties": {
        "label": {
            "type": "string",
            "minLength": 1,
            "maxLength": 64
        }
    },
    "required": [
        "label"
    ]
}
)json");
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

class ClientTaskTool final : public Tool {
public:
    ClientTaskTool()
        : Tool({.name = "client.task",
                .description = "Legacy client-owned sampling or elicitation task and lifecycle controls.",
                .inputSchema =
                    Json::parse(R"json(
{
    "type": "object",
    "properties": {
        "kind": {
            "enum": [
                "sampling",
                "elicitation"
            ]
        },
        "cancel": {
            "type": "boolean"
        }
    }
}
)json"),
                .title = "client.task",
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
    "showcase/feature": "client.task"
}
)json")}) {
    }

    void call(const ToolCall& request, const std::shared_ptr<ToolReply>& reply) final {
        request.context.trackClientProgress("created");
        const bool sampling = request.arguments.value("kind", "sampling") == "sampling";
        const auto& state = request.context.getRequestState();
        if (request.context.getProtocolEra() != ::mcp::ProtocolEra::Initialization20251125) {
            reply->result(ToolResult::toolError({TextContent{
                "Client-owned task augmentation belongs to 2025-11-25; use task input continuations in 2026-07-28.", {}}}));
        } else if (!state) {
            Json params =
                sampling
                    ? Json::parse(R"json(
{
    "messages": [
        {
            "role": "user",
            "content": {
                "type": "text",
                "text": "42"
            }
        }
    ],
    "maxTokens": 64
}
)json")
                    : Json{
                        {"mode", "form"},
                        {"message", "Client task demonstration"},
                        {"requestedSchema", labelSchema()}
                    };
            params["task"] = {
                {"ttl", 60000}
            };
            reply->clientRequestsRequired(
                {
                    {"created", {
                        {"method", sampling ? "sampling/createMessage" : "elicitation/create"},
                        {"params", params}
                    }}
                },
                "created");
        } else if (*state == "created") {
            const auto* ticket = request.context.getClientResponse("created");
            if (ticket && ticket->contains("task")) {
                const std::string id = ticket->at("task").at("taskId");
                reply->clientRequestsRequired({
                    {"status", {
                        {"method", "tasks/get"},
                        {"params", {
                            {"taskId", id}
                        }}
                    }},
                    {"inventory", {
                        {"method", "tasks/list"},
                        {"params", Json::object()}
                    }}
                },
                                              Json{
                                                  {"phase", "inspect"},
                                                  {"taskId", id}
                                              }.dump(4));
            } else {
                reply->error(ErrorCode::InvalidParams, "Client returned no task ticket");
            }
        } else {
            Json data = Json::parse(*state);
            const std::string phase = data.at("phase");
            if (phase == "inspect") {
                const bool cancel = request.arguments.value("cancel", false);
                data["phase"] = cancel ? "cancelled" : "completed";
                reply->clientRequestsRequired(
                    {
                        {"payload", {
                            {"method", cancel ? "tasks/cancel" : "tasks/result"},
                            {"params", {
                                {"taskId", data.at("taskId")}
                            }}
                        }}
                    },
                    data.dump(4));
            } else {
                const auto* payload = request.context.getClientResponse("payload");
                if (payload) {
                    if (phase == "completed" && sampling) {
                        static_cast<void>(::mcp::SamplingResult(*payload));
                    } else if (phase == "completed") {
                        static_cast<void>(request.context.getInputResponse("payload", labelSchema()));
                    }
                    reply->result(clientInputResult(request.context, "created", {
                        {"status", phase},
                        {"payload", *payload}
                    }));
                } else {
                    reply->error(ErrorCode::InvalidParams, "Missing client task result");
                }
            }
        }
    }
};

} // namespace apps::mcp::features
