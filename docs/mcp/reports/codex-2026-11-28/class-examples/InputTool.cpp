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

std::string actionName(const InputResponse* response) {
    return response != nullptr && response->action == ElicitationAction::Accept   ? "accept"
           : response != nullptr && response->action == ElicitationAction::Cancel ? "cancel"
                                                                                  : "decline";
}

Json inputResult(const InputResponse* response) {
    Json value{
        {"action", actionName(response)}
    };
    if (response != nullptr && response->action == ElicitationAction::Accept) {
        value["label"] = response->content.at("label");
    }
    return value;
}

class InputTool final : public Tool {
public:
    explicit InputTool(std::string mode)
        : Tool({.name = "input." + mode,
                .description = "Elicitation: non-sensitive form, URL or two-round continuation.",
                .inputSchema = mode == "url" ? Json::parse(R"json(
{
    "type": "object",
    "additionalProperties": false,
    "properties": {
        "required": {
            "type": "boolean"
        }
    }
}
)json")
                                             : Json::parse(R"json(
{
    "type": "object",
    "additionalProperties": false
}
)json"),
                .title = "input." + mode,
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
                    {"showcase/feature", "input." + mode}
                },
                .taskSupport = mode == "task" ? TaskSupport::Required : TaskSupport::Forbidden})
        , mode(std::move(mode))
        , form(labelSchema()) {
    }

    void call(const ToolCall& call, const std::shared_ptr<ToolReply>& reply) override {
        if (mode == "task" && !call.context.getTaskId()) {
            reply->startTask(TaskOptions{5000, 20});
        }
        const auto& state = call.context.getRequestState();
        const auto& inputs = call.context.getInputResponses();
        if (mode == "url" && call.arguments.value("required", false) &&
            call.context.getProtocolEra() == ::mcp::ProtocolEra::Initialization20251125) {
            reply->error(ErrorCode::UrlElicitationRequired,
                         "Complete the demonstration URL before retrying.",
                         {
                             {"elicitations", {{{"mode", "url"},
                             {"message", "Demonstration acknowledgement only."},
                             {"url", "https://example.com/"},
                             {"elicitationId", utils::Uuid::getUuid()}}}}
                         });
        } else if (inputs.empty()) {
            InputRequired input{{
                {"first", FormInput{"Enter a non-sensitive label.", form}}
            }, "first"};
            if (mode == "parallel") {
                input.requests["second"] = FormInput{"Enter a second independent label.", form};
            }
            if (mode == "url") {
                input.requests["first"] =
                    UrlInput{"Acknowledge this demonstration URL; no login is performed.", "https://example.com/"};
            }
            reply->inputRequired(std::move(input));
        } else if (mode == "rounds" && state == std::optional<std::string>{"first"}) {
            const auto* first = call.context.getInputResponse("first", form);
            if (first != nullptr && first->action == ElicitationAction::Accept) {
                reply->inputRequired(InputRequired{{
                    {"second", FormInput{"Confirm with a second label.", form}}
                }, "second"});
            } else {
                reply->result(ToolResult::json(inputResult(first)));
            }
        } else if (mode == "parallel") {
            reply->result(ToolResult::json({
                {"first", inputResult(call.context.getInputResponse("first", form))},
                {"second", inputResult(call.context.getInputResponse("second", form))}
            }));
        } else {
            const std::string key = mode == "rounds" ? "second" : "first";
            const auto* response = mode == "url" ? &inputs.at(key) : call.context.getInputResponse(key, form);
            Json value{
                {"action", actionName(response)},
                {"state", state.value_or("")}
            };
            if (mode == "url") {
                const auto id = call.context.getElicitationId(key);
                value["completionNotified"] =
                    id && response->action == ElicitationAction::Accept && call.context.notifyElicitationComplete(*id);
            }
            if (mode != "url" && response != nullptr && response->action == ElicitationAction::Accept) {
                value["label"] = response->content.at("label");
            }
            reply->result(ToolResult::json(std::move(value)));
        }
    }

private:
    std::string mode;
    Json form;
};

} // namespace apps::mcp::features
