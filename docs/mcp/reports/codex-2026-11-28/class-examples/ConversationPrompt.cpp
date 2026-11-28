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

void completeName(const CompletionRequest& call, const std::shared_ptr<CompletionReply>& reply) {
    CompletionResult result;
    const std::string prefix = call.arguments.contains("prefix") ? call.arguments.at("prefix") : std::string{};
    for (const auto& value : {"alpha", "beta", "gamma"}) {
        if (std::string(value).starts_with(call.value)) {
            result.values.push_back(prefix + value);
        }
    }
    result.total = result.values.size();
    result.meta = Json::parse(R"json(
{
    "showcase/completion": true
}
)json");
    reply->result(std::move(result));
}

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
                         {
                             {"showcase/prompt", true}
                         }});
    }
};

} // namespace apps::mcp::features
