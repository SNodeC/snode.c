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

InputRequired labelRequest(const std::string& message) {
    return InputRequired{{
        {"first", FormInput{message, labelSchema()}}
    }, "label"};
}

class InputPrompt final : public Prompt {
public:
    InputPrompt()
        : Prompt({"input", "Prompt-originated elicitation and input_required."})
        , form(labelSchema()) {
    }

    void get(const PromptGet& call, const std::shared_ptr<PromptReply>& reply) override {
        const auto* answer = call.context.getInputResponse("first", form);
        if (answer == nullptr) {
            reply->inputRequired(labelRequest("Choose a non-sensitive label for this prompt."));
        } else {
            reply->result(PromptResult{{}, {{Role::User, TextContent{inputResult(answer).dump(4), {}}}}});
        }
    }

private:
    Json form;
};

} // namespace apps::mcp::features
