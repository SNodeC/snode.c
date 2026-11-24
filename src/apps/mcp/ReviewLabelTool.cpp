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

#include "ReviewLabelTool.h"

#include "ReviewLabelResource.h"
#include "Tools.h"
#include "mcp/server/Request.h"
#include "mcp/server/Results.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <map>
#include <optional>
#include <string>
#include <utility>
#include <variant>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace apps::mcp {

    ReviewLabelTool::ReviewLabelTool(std::shared_ptr<ReviewLabelResource> resource)
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

    void ReviewLabelTool::call(const ::mcp::server::ToolCall& request, const std::shared_ptr<::mcp::server::ToolReply>& reply) {
        const auto& inputs = request.context.getInputResponses();
        const auto* input = request.context.getInputResponse("label", formSchema);
        if (inputs.empty()) {
            reply->inputRequired(::mcp::server::InputRequired{
                {{"label", ::mcp::server::FormInput{"Choose a non-sensitive review label.", formSchema}}}, {}});
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

} // namespace apps::mcp
