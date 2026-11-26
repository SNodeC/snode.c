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
#include "core/timer/Timer.h"
#include "mcp/Types.h"
#include "mcp/server/Reply.h"
#include "mcp/server/Request.h"
#include "mcp/server/Results.h"
#include "mcp/server/Server.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>
#include <initializer_list>
#include <map>
#include <memory>
#include <nlohmann/detail/json_ref.hpp>
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
                            self->reply->result(ToolResult::json({{"completed", self->completed}}));
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

        ToolDefinition workDefinition(const std::string& name, TaskSupport support) {
            auto tool =
                toolDefinition(name, "Bounded asynchronous work: progress, cancellation, deadline and task lifecycle.", schema(R"json(
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
)json"));
            tool.taskSupport = support;
            return tool;
        }

        class WorkTool final : public Tool {
        public:
            WorkTool(const std::string& name, TaskSupport support)
                : Tool(workDefinition(name, support)) {
            }

            void call(const ToolCall& call, const std::shared_ptr<ToolReply>& reply) override {
                if (getDefinition().taskSupport != TaskSupport::Forbidden) {
                    reply->startTask(TaskOptions{5000, 20});
                }
                const auto work = std::make_shared<Work>(reply, call.arguments.value("steps", 5U), call.arguments.value("delayMs", 50U));
                work->start(call.context);
            }
        };

        Json labelSchema() {
            return schema(R"json(
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
            Json value{{"action", actionName(response)}};
            if (response != nullptr && response->action == ElicitationAction::Accept) {
                value["label"] = response->content.at("label");
            }
            return value;
        }

        InputRequired labelRequest(const std::string& message) {
            return InputRequired{{{"first", FormInput{message, labelSchema()}}}, "label"};
        }

        ToolDefinition inputDefinition(const std::string& mode) {
            auto tool = toolDefinition("input." + mode, "Elicitation: non-sensitive form, URL or two-round continuation.", emptySchema());
            if (mode == "task") {
                tool.taskSupport = TaskSupport::Required;
            }
            return tool;
        }

        class InputTool final : public Tool {
        public:
            explicit InputTool(std::string mode)
                : Tool(inputDefinition(mode))
                , mode(std::move(mode))
                , form(labelSchema()) {
            }

            void call(const ToolCall& call, const std::shared_ptr<ToolReply>& reply) override {
                if (mode == "task" && !call.context.getTaskId()) {
                    reply->startTask(TaskOptions{5000, 20});
                }
                const auto& state = call.context.getRequestState();
                const auto& inputs = call.context.getInputResponses();
                if (inputs.empty()) {
                    InputRequired input{{{"first", FormInput{"Enter a non-sensitive label.", form}}}, "first"};
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
                        reply->inputRequired(InputRequired{{{"second", FormInput{"Confirm with a second label.", form}}}, "second"});
                    } else {
                        reply->result(ToolResult::json(inputResult(first)));
                    }
                } else if (mode == "parallel") {
                    reply->result(ToolResult::json({{"first", inputResult(call.context.getInputResponse("first", form))},
                                                    {"second", inputResult(call.context.getInputResponse("second", form))}}));
                } else {
                    const std::string key = mode == "rounds" ? "second" : "first";
                    const auto* response = mode == "url" ? &inputs.at(key) : call.context.getInputResponse(key, form);
                    Json value{{"action", actionName(response)}, {"state", state.value_or("")}};
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

        class InputResource final : public Resource {
        public:
            InputResource()
                : Resource({"showcase://input", "input", "Resource-originated elicitation and input_required.", "application/json"})
                , form(labelSchema()) {
            }

            void read(const ResourceRead& call, const std::shared_ptr<ResourceReply>& reply) override {
                const auto* answer = call.context.getInputResponse("first", form);
                if (answer == nullptr) {
                    reply->inputRequired(labelRequest("Choose a non-sensitive label for this resource read."));
                } else {
                    reply->result(ResourceResult{{TextResource{call.uri, "application/json", inputResult(answer).dump(4)}}});
                }
            }

        private:
            Json form;
        };

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

        class LoggingTool final : public Tool {
        public:
            explicit LoggingTool(Server& server)
                : Tool(toolDefinition(
                      "logging", "Legacy protocol logging from RequestContext and Server (deprecated in 2026).", emptySchema()))
                , server(server) {
            }

            void call(const ToolCall& call, const std::shared_ptr<ToolReply>& reply) override {
                const bool admitted = call.context.log(LogLevel::Info, {{"source", "handler"}}, "showcase");
                server.log(LogLevel::Warning, {{"source", "server"}}, "showcase");
                reply->result(ToolResult::json({{"requestLogAdmitted", admitted}}));
            }

        private:
            Server& server;
        };

    } // namespace

    namespace {
        class SamplingTool final : public Tool {
        public:
            SamplingTool()
                : Tool(toolDefinition("sampling",
                                      "Request client-controlled model sampling, optionally with tools.",
                                      {{"type", "object"}, {"properties", {{"toolUse", {{"type", "boolean"}}}}}})) {
            }

            void call(const ToolCall& request, const std::shared_ptr<ToolReply>& reply) final {
                Json params{
                    {"messages", {{{"role", "user"}, {"content", {{"type", "text"}, {"text", "Return the demonstration answer 42."}}}}}},
                    {"maxTokens", 64}};
                if (request.arguments.value("toolUse", false)) {
                    params["tools"] = {{{"name", "answer"}, {"inputSchema", emptySchema()}}};
                    params["toolChoice"] = {{"mode", "required"}};
                }
                const ::mcp::SamplingRequest sampling(std::move(params));
                if (const auto response = request.context.getSamplingResponse("sample", sampling)) {
                    reply->result(ToolResult::json(response->toJson()));
                } else {
                    reply->clientInputRequired({{{"sample", sampling}}, "sampling"});
                }
            }
        };

        class RootsTool final : public Tool {
        public:
            RootsTool()
                : Tool(toolDefinition("roots", "Request client-disclosed informational filesystem roots.", emptySchema())) {
            }

            void call(const ToolCall& request, const std::shared_ptr<ToolReply>& reply) final {
                const std::weak_ptr<unsigned> weak = changes;
                request.context.onRootsChanged([weak] {
                    if (const auto count = weak.lock()) {
                        ++*count;
                    }
                });
                if (const auto response = request.context.getRootsResponse("roots")) {
                    Json value = response->toJson();
                    value["listChanges"] = *changes;
                    reply->result(ToolResult::json(std::move(value)));
                } else {
                    reply->clientInputRequired({{{"roots", ::mcp::RootsRequest{}}}, "roots"});
                }
            }

        private:
            std::shared_ptr<unsigned> changes = std::make_shared<unsigned>(0);
        };
    } // namespace

    void registerInteractions(Server& server) {
        server.tool(std::make_shared<SamplingTool>());
        server.tool(std::make_shared<RootsTool>());
        server.tool(std::make_shared<WorkTool>("work", TaskSupport::Forbidden));
        server.tool(std::make_shared<WorkTool>("task.optional", TaskSupport::Optional));
        server.tool(std::make_shared<WorkTool>("task.required", TaskSupport::Required));
        for (const auto& mode : {"form", "url", "rounds", "parallel", "task"}) {
            server.tool(std::make_shared<InputTool>(mode));
        }
        server.resource(std::make_shared<InputResource>());
        server.prompt(std::make_shared<InputPrompt>());
        server.tool(std::make_shared<LoggingTool>(server));
    }

} // namespace apps::mcp::features
