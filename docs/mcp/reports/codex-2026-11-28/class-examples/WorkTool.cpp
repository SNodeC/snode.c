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

class Work final : public std::enable_shared_from_this<Work> {
public:
    Work(std::shared_ptr<ToolReply> reply, unsigned steps, unsigned delay)
        : reply(std::move(reply))
        , steps(steps)
        , delay(delay) {
    }

    void start(const RequestContext& context) {
        const std::weak_ptr<Work> weak = shared_from_this();
        context.onCancel([weak](CancelReason) {
            if (const auto work = weak.lock()) {
                work->timer.cancel();
            }
        });
        schedule();
    }

private:
    void schedule() {
        timer = core::timer::Timer::singleshotTimer(
            [self = shared_from_this()] {
                auto firing = std::move(self->timer);
                ++self->completed;
                self->reply->progress(
                    Progress{static_cast<double>(self->completed), static_cast<double>(self->steps), "bounded work"});
                if (self->completed == self->steps) {
                    self->reply->result(ToolResult::json({
                        {"completed", self->completed}
                    }));
                } else {
                    self->schedule();
                }
            },
            utils::Timeval(static_cast<double>(delay) / 1000.0));
    }

    std::shared_ptr<ToolReply> reply;
    unsigned steps;
    unsigned delay;
    unsigned completed = 0;
    core::timer::Timer timer;
};

class WorkTool final : public Tool {
public:
    WorkTool(const std::string& name, TaskSupport support)
        : Tool({.name = name,
                .description = "Bounded asynchronous work: progress, cancellation, deadline and task lifecycle.",
                .inputSchema = Json::parse(R"json(
{
    "type": "object",
    "properties": {
        "steps": {
            "type": "integer",
            "minimum": 1,
            "maximum": 20
        },
        "delayMs": {
            "type": "integer",
            "minimum": 10,
            "maximum": 1000
        }
    },
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
                },
                .taskSupport = support}) {
    }

    void call(const ToolCall& call, const std::shared_ptr<ToolReply>& reply) override {
        if (getDefinition().taskSupport != TaskSupport::Forbidden) {
            reply->startTask(TaskOptions{5000, 20});
        }
        reply->taskStatus(::mcp::TaskState::Working, "Running bounded demonstration work");
        const auto work = std::make_shared<Work>(reply, call.arguments.value("steps", 5U), call.arguments.value("delayMs", 50U));
        work->start(call.context);
    }
};

} // namespace apps::mcp::features
