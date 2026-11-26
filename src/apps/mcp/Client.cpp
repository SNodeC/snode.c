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

#include "Client.h"

#include "ClientConfig.h"
#include "core/EventReceiver.h"
#include "core/SNodeC.h"
#include "core/timer/Timer.h"
#include "jsonrpc/messages/Error.h"
#include "jsonrpc/messages/Id.h"
#include "mcp/Protocol.h"
#include "mcp/Types.h"
#include "mcp/client/Mcp.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cmath>
#include <exception>
#include <initializer_list>
#include <iostream>
#include <map>
#include <memory>
#include <nlohmann/detail/json_ref.hpp>
#include <nlohmann/json.hpp>
#include <optional>
#include <utility>
#include <variant>
#endif

namespace apps::mcp {

    using ::mcp::Json;

    namespace {

        bool hasName(const Json& entries, const std::string& name) {
            bool found = false;
            for (const auto& entry : entries) {
                found = found || entry.value("name", "") == name;
            }
            return found;
        }

    } // namespace

    Client::Client(bool& success, const ClientConfig& config)
        : success(success)
        , config(config) {
    }

    void Client::onConnected() {
        setNegotiation(config.getLegacy()              ? ::mcp::client::Negotiation::Legacy
                       : config.getCommand() == "tour" ? ::mcp::client::Negotiation::Modern
                                                       : ::mcp::client::Negotiation::Auto);
        if (config.getCommand() == "tour") {
            setSamplingHandler(
                [](const std::string&, const ::mcp::SamplingRequest& request, const std::shared_ptr<::mcp::client::ClientReply>& reply) {
                    const bool tools = request.toJson().contains("tools");
                    const Json content =
                        tools ? Json{{"type", "tool_use"}, {"id", "demo-call"}, {"name", "answer"}, {"input", Json::object()}}
                              : Json{{"type", "text"}, {"text", "42"}};
                    reply->result(::mcp::SamplingResult({{"role", "assistant"},
                                                         {"content", content},
                                                         {"model", "deterministic-test-model"},
                                                         {"stopReason", tools ? "toolUse" : "endTurn"}}));
                },
                true);
            setRootsHandler(
                [](const std::string&, const std::shared_ptr<::mcp::client::ClientReply>& reply) {
                    reply->result(
                        ::mcp::RootsResult({{"roots", {{{"uri", "file:///showcase/workspace"}, {"name", "Demonstration workspace"}}}}}));
                },
                true);
        }
        Mcp::onConnected();
    }

    bool Client::legacy() const {
        return getProtocolEra() == ::mcp::ProtocolEra::Initialization20251125;
    }

    Client::~Client() {
        actionTimer.cancel();
    }

    void Client::command() {
        const std::string operation = config.getCommand();
        const std::string name = config.getNameValue();
        const Json arguments = config.getArguments();
        if (operation == "tools") {
            listTools();
        } else if (operation == "resources") {
            listResources();
        } else if (operation == "templates") {
            listResourceTemplates();
        } else if (operation == "prompts") {
            listPrompts();
        } else if (operation == "call") {
            ::mcp::client::RequestOptions request;
            request.task = config.getTask();
            request.progressToken = "cli";
            callTool(name, arguments, request);
        } else if (operation == "read") {
            readResource(name);
        } else if (operation == "prompt") {
            getPrompt(name, arguments);
        } else if (operation == "listen") {
            listen(arguments);
        } else if (operation == "log") {
            setLoggingLevel(::mcp::LogLevel::Debug);
        } else {
            request(operation, arguments);
        }
    }

    void Client::add(
        std::string feature, std::string request, std::function<void()> run, std::function<bool(const Json&)> check, Outcome outcome) {
        steps.push_back({std::move(feature), std::move(request), std::move(run), std::move(check), outcome});
    }

    void Client::plan() {
        const auto text = [](const Json& result) {
            return result.at("content").at(0).at("text").is_string() && !result.value("isError", false);
        };
        const auto completed = [](const Json& result) {
            return result.at("structuredContent").at("completed") == 3;
        };
        add(
            "tools.pagination",
            "tools/list; follow every page",
            [this] {
                listTools();
            },
            [](const Json& value) {
                return value.size() == 35 && hasName(value, "schema") && hasName(value, "catalogue.8") && !hasName(value, "dynamic.tool");
            });
        add(
            "resources.pagination",
            "resources/list; follow every page",
            [this] {
                listResources();
            },
            [](const Json& value) {
                return value.size() == 15;
            });
        add(
            "templates.pagination",
            "resources/templates/list; follow every page",
            [this] {
                listResourceTemplates();
            },
            [](const Json& value) {
                return value.size() == 12;
            });
        add(
            "prompts.pagination",
            "prompts/list; follow every page",
            [this] {
                listPrompts();
            },
            [](const Json& value) {
                return value.size() == 12;
            });
        for (const bool tools : {false, true}) {
            add(
                tools ? "sampling.tools" : "sampling.text",
                "tools/call sampling",
                [this, tools] {
                    callTool("sampling", {{"toolUse", tools}});
                },
                [tools](const Json& result) {
                    const auto& value = result.at("structuredContent");
                    return value.at("model") == "deterministic-test-model" &&
                           value.at("content").at("type") == (tools ? "tool_use" : "text");
                });
        }
        add(
            "roots",
            "tools/call roots",
            [this] {
                callTool("roots");
            },
            [](const Json& result) {
                return result.at("structuredContent").at("roots").at(0).at("uri") == "file:///showcase/workspace";
            });
        add(
            "text",
            "tools/call text",
            [this] {
                callTool("text");
            },
            text);
        add(
            "tool.error",
            "tools/call tool.error; expect isError",
            [this] {
                callTool("tool.error");
            },
            [](const Json& value) {
                return value.at("isError") == true;
            });
        add(
            "protocol.error",
            "tools/call protocol.error; expect InvalidParams",
            [this] {
                callTool("protocol.error");
            },
            [](const Json& value) {
                return value.at("code") == -32602;
            },
            Outcome::Error);
        add(
            "structured.valid",
            "tools/call structured.valid; validate outputSchema",
            [this] {
                callTool("structured.valid");
            },
            [](const Json& value) {
                return value.at("structuredContent").at("answer") == 42;
            });
        add(
            "structured.invalid",
            "tools/call structured.invalid; expect output rejection",
            [this] {
                callTool("structured.invalid");
            },
            [](const Json& value) {
                return value.at("isError") == true && value.at("content")[0].at("text") == "Tool output failed its registered schema";
            },
            Outcome::Result);
        add(
            "schema.valid",
            "tools/call schema; valid Unicode and choice",
            [this] {
                callTool("schema", {{"word", "Grüße世界"}, {"choice", "alpha"}});
            },
            text);
        add(
            "schema.invalid",
            "tools/call schema; reject extra property",
            [this] {
                callTool("schema", {{"word", "123"}, {"choice", 3}, {"extra", true}});
            },
            [](const Json& value) {
                return value.at("isError") == true;
            },
            Outcome::Result);
        add(
            "headers",
            "tools/call headers; project the region parameter",
            [this] {
                callTool("headers", {{"region", "Hello, 世界"}});
            },
            [](const Json& value) {
                return value.at("structuredContent").at("region") == "Hello, 世界";
            });
        add(
            "content",
            "tools/call content; all content variants",
            [this] {
                callTool("content");
            },
            [](const Json& value) {
                const auto& content = value.at("content");
                return content.size() == 6 && content[0]["type"] == "text" && content[1]["type"] == "image" &&
                       content[2]["type"] == "audio" && content[3]["type"] == "resource_link" && content[4]["resource"].contains("text") &&
                       content[5]["resource"].contains("blob");
            });
        add(
            "principal",
            "tools/call principal",
            [this] {
                callTool("principal");
            },
            [](const Json& value) {
                return value.at("structuredContent").at("subject").is_string();
            });
        add(
            "resource.text",
            "resources/read showcase://text",
            [this] {
                readResource("showcase://text");
            },
            [](const Json& value) {
                return value.at("contents")[0].at("text") == "SNode.C\n";
            });
        add(
            "resource.blob",
            "resources/read showcase://blob",
            [this] {
                readResource("showcase://blob");
            },
            [](const Json& value) {
                return value.at("contents")[0].at("blob") == "AAEC";
            });
        add(
            "template.scalar",
            "resources/read showcase://greeting/{name}",
            [this] {
                readResource("showcase://greeting/Ada");
            },
            [](const Json& value) {
                return value.at("contents")[0].at("text") == "Hello Ada";
            });
        add(
            "template.reserved",
            "resources/read showcase://path/a/b",
            [this] {
                readResource("showcase://path/a/b");
            },
            [](const Json& value) {
                return value.at("contents")[0].at("text") == "a/b";
            });
        add(
            "prompt",
            "prompts/get conversation(name)",
            [this] {
                getPrompt("conversation", {{"name", "Ada"}});
            },
            [](const Json& value) {
                return value.at("messages") == 2;
            });
        add(
            "completion.prompt",
            "completion/complete ref/prompt conversation; prefix=a",
            [this] {
                complete({{"type", "ref/prompt"}, {"name", "conversation"}}, "name", "a");
            },
            [](const Json& value) {
                return value.at("values") == Json::array({"alpha"});
            });
        add(
            "completion.template",
            "completion/complete ref/resource greeting; prefix=b and context",
            [this] {
                complete({{"type", "ref/resource"}, {"uri", "showcase://greeting/{name}"}}, "name", "b", {{"prefix", "X"}});
            },
            [](const Json& value) {
                return value.at("values") == Json::array({"Xbeta"});
            });
        add(
            "logging.level",
            "logging/setLevel debug; legacy only",
            [this] {
                setLoggingLevel(::mcp::LogLevel::Debug);
            },
            {},
            legacy() ? Outcome::Result : Outcome::Error);
        add(
            "logging",
            "tools/call logging",
            [this] {
                callTool("logging");
            },
            [this](const Json& value) {
                return value.at("structuredContent").at("requestLogAdmitted") == legacy();
            });
        add("subscriptions", "subscribe/listen to counter and inventory updates", [this] {
            if (legacy()) {
                subscribe("showcase://counter");
            } else {
                listen({{"resourceSubscriptions", {"showcase://counter"}},
                        {"toolsListChanged", true},
                        {"resourcesListChanged", true},
                        {"promptsListChanged", true}});
            }
        });
        add(
            "resource.update",
            "tools/call update",
            [this] {
                callTool("update");
            },
            [](const Json& value) {
                return value.at("structuredContent").at("revision").is_number_unsigned();
            });
        add(
            "dynamic.add",
            "tools/call dynamic; action=add",
            [this] {
                callTool("dynamic", {{"action", "add"}});
            },
            text);
        add(
            "dynamic.tools",
            "tools/list; find dynamic.tool",
            [this] {
                listTools();
            },
            [](const Json& value) {
                return hasName(value, "dynamic.tool");
            });
        add(
            "dynamic.resources",
            "resources/list; find dynamic",
            [this] {
                listResources();
            },
            [](const Json& value) {
                return hasName(value, "dynamic");
            });
        add(
            "dynamic.templates",
            "resources/templates/list; find dynamic",
            [this] {
                listResourceTemplates();
            },
            [](const Json& value) {
                return hasName(value, "dynamic");
            });
        add(
            "dynamic.prompts",
            "prompts/list; find dynamic",
            [this] {
                listPrompts();
            },
            [](const Json& value) {
                return hasName(value, "dynamic");
            });
        add(
            "dynamic.tool.call",
            "tools/call dynamic.tool",
            [this] {
                callTool("dynamic.tool");
            },
            text);
        add("dynamic.resource.read", "resources/read showcase://dynamic", [this] {
            readResource("showcase://dynamic");
        });
        add("dynamic.template.read", "resources/read showcase://dynamic/{value}", [this] {
            readResource("showcase://dynamic/hello");
        });
        add("dynamic.prompt.get", "prompts/get dynamic", [this] {
            getPrompt("dynamic");
        });
        add(
            "dynamic.remove",
            "tools/call dynamic; action=remove",
            [this] {
                callTool("dynamic", {{"action", "remove"}});
            },
            text);
        add(
            "dynamic.absent",
            "tools/list; confirm removal",
            [this] {
                listTools();
            },
            [](const Json& value) {
                return !hasName(value, "dynamic.tool");
            });
        add(
            "dynamic.resources.absent",
            "resources/list; confirm resource removal",
            [this] {
                listResources();
            },
            [](const Json& value) {
                return value.size() == 15 && !hasName(value, "dynamic");
            });
        add(
            "dynamic.templates.absent",
            "resources/templates/list; confirm template removal",
            [this] {
                listResourceTemplates();
            },
            [](const Json& value) {
                return value.size() == 12 && !hasName(value, "dynamic");
            });
        add(
            "dynamic.prompts.absent",
            "prompts/list; confirm prompt removal",
            [this] {
                listPrompts();
            },
            [](const Json& value) {
                return value.size() == 12 && !hasName(value, "dynamic");
            });
        const auto input = [this](const Json& value) {
            return value.at("structuredContent").at("action") == (config.getAnswerAction() == ::mcp::ElicitationAction::Accept ? "accept"
                                                                  : config.getAnswerAction() == ::mcp::ElicitationAction::Cancel
                                                                      ? "cancel"
                                                                      : "decline");
        };
        add(
            "input.form",
            "tools/call input.form; asynchronous form",
            [this] {
                callTool("input.form");
            },
            input);
        add(
            "input.url",
            "tools/call input.url; asynchronous URL acknowledgement",
            [this] {
                callTool("input.url");
            },
            input);
        add(
            "input.rounds",
            "tools/call input.rounds; two continuations",
            [this] {
                callTool("input.rounds");
            },
            [this](const Json& value) {
                return config.getAnswerAction() == ::mcp::ElicitationAction::Accept
                           ? value.at("structuredContent").at("state") == "second"
                           : value.at("structuredContent").at("action") != "accept";
            });
        add(
            "input.parallel",
            "tools/call input.parallel; two independent responders",
            [this] {
                callTool("input.parallel");
            },
            [this](const Json& value) {
                const std::string expected = config.getAnswerAction() == ::mcp::ElicitationAction::Accept   ? "accept"
                                             : config.getAnswerAction() == ::mcp::ElicitationAction::Cancel ? "cancel"
                                                                                                            : "decline";
                return value.at("structuredContent").at("first").at("action") == expected &&
                       value.at("structuredContent").at("second").at("action") == expected;
            });
        add(
            "input.resource",
            "resources/read showcase://input; asynchronous form",
            [this] {
                readResource("showcase://input");
            },
            [this](const Json& value) {
                return Json::parse(value.at("contents")[0].at("text").get<std::string>()).at("action") ==
                       (config.getAnswerAction() == ::mcp::ElicitationAction::Accept   ? "accept"
                        : config.getAnswerAction() == ::mcp::ElicitationAction::Cancel ? "cancel"
                                                                                       : "decline");
            });
        add(
            "input.prompt",
            "prompts/get input; asynchronous form",
            [this] {
                getPrompt("input");
            },
            [this](const Json& value) {
                return value.at("messages") == 1 && Json::parse(value.at("firstText").get<std::string>()).at("action") ==
                                                        (config.getAnswerAction() == ::mcp::ElicitationAction::Accept   ? "accept"
                                                         : config.getAnswerAction() == ::mcp::ElicitationAction::Cancel ? "cancel"
                                                                                                                        : "decline");
            });
        add(
            "work.progress",
            "tools/call work; three progress values",
            [this] {
                ::mcp::client::RequestOptions request;
                request.progressToken = "tour";
                callTool("work", {{"steps", 3}}, request);
            },
            [this, completed](const Json& value) {
                return progress >= 3 && completed(value);
            });
        add("work.cancel", "tools/call work; cancel pending request", [this] {
            const auto id = callTool("work", {{"steps", 20}, {"delayMs", 100}});
            actionTimer = core::timer::Timer::singleshotTimer(
                [this, id] {
                    auto timer = std::move(actionTimer);
                    cancel(id, "Tour cancellation");
                    finish({{"cancelSent", true}});
                },
                utils::Timeval(0.1));
        });
        add("work.afterCancel", "tools/list; connection remains usable", [this] {
            listTools();
        });
        add(
            "work.deadline",
            "tools/call work; exceed server deadline",
            [this] {
                callTool("work", {{"steps", 20}, {"delayMs", 1000}});
            },
            [](const Json& value) {
                return value.at("isError") == true && value.at("content")[0].at("text") == "Request deadline exceeded";
            },
            Outcome::Result);
        add(
            "work.clientTimeout",
            "tools/call work; exceed client timeout",
            [this] {
                ::mcp::client::RequestOptions request;
                request.timeout = utils::Timeval(0.05);
                callTool("work", {{"steps", 10}, {"delayMs", 100}}, request);
            },
            {},
            Outcome::Timeout);
        for (const std::string name : {"task.optional", "task.required", "input.task"}) {
            add(
                name,
                "tools/call " + name + "; await task/input completion",
                [this, name] {
                    ::mcp::client::RequestOptions request;
                    request.task = legacy();
                    request.ttlMs = 60000;
                    request.progressToken = name;
                    callTool(name, name == "input.task" ? Json::object() : Json{{"steps", 3}}, request);
                },
                name == "input.task" ? std::function<bool(const Json&)>(input) : std::function<bool(const Json&)>(completed));
        }
        add(
            "task.get",
            "tasks/get last completed task",
            [this] {
                getTask(lastTask);
            },
            [](const Json& value) {
                return value.at("status") == static_cast<int>(::mcp::TaskState::Completed);
            });
        add(
            "task.list",
            "tasks/list; legacy only",
            [this] {
                listTasks();
            },
            {},
            legacy() ? Outcome::Result : Outcome::Error);
        add(
            "task.result",
            "tasks/result; legacy only",
            [this] {
                getTaskResult(lastTask);
            },
            {},
            legacy() ? Outcome::Result : Outcome::Error);
        add(
            "task.cancel",
            "tools/call task.required; cancel resulting task",
            [this] {
                ::mcp::client::RequestOptions request;
                request.task = legacy();
                callTool("task.required", {{"steps", 20}, {"delayMs", 100}}, request);
            },
            {},
            Outcome::Cancelled);
        add("unsubscribe", "unsubscribe/cancel counter interest", [this] {
            unsubscribe("showcase://counter");
            if (!legacy()) {
                finish({{"cancelSent", true}});
            }
        });
        add(
            "notifications.summary",
            "verify observed notifications and input/task counts",
            [this] {
                finish({{"tools", toolChanges},
                        {"resources", resourceChanges},
                        {"prompts", promptChanges},
                        {"updated", updates},
                        {"logs", logs},
                        {"inputs", inputs},
                        {"tasks", tasks},
                        {"statuses", statuses}});
            },
            [this](const Json&) {
                return toolChanges == 2 && resourceChanges == 2 && promptChanges == 2 && updates == 1 && logs == (legacy() ? 2U : 0U) &&
                       progress == (legacy() ? 9U : 3U) &&
                       inputs == (config.getAnswerAction() == ::mcp::ElicitationAction::Accept ? 9U : 8U) && tasks == 4 && statuses > 0 &&
                       (legacy() || taskAcknowledgements == 1);
            });
    }

    void Client::execute() {
        if (current < steps.size()) {
            pending = true;
            std::cout << Json{{"feature", steps[current].feature}, {"request", steps[current].request}}.dump() << std::endl;
            steps[current].run();
        } else {
            success = true;
            std::cout << Json{{"tour", "PASS"}, {"steps", steps.size()}}.dump() << std::endl;
            disconnect();
            core::SNodeC::stop();
        }
    }

    void Client::finish(Json result, Outcome outcome) {
        if (config.getCommand() != "tour") {
            std::cout << result.dump() << std::endl;
            success = outcome == Outcome::Result;
            if (!config.getWatch() || !success) {
                disconnect();
                core::SNodeC::stop();
            }
        } else if (pending && current < steps.size()) {
            const auto& step = steps[current];
            bool valid = step.outcome == outcome;
            try {
                valid = valid && (!step.check || step.check(result));
            } catch (const std::exception&) {
                valid = false;
            }
            pending = false;
            std::cout << Json{{"feature", step.feature}, {"outcome", valid ? "PASS" : "FAIL"}, {"result", result}}.dump() << std::endl;
            if (valid) {
                ++current;
                core::EventReceiver::atNextTick([this] {
                    execute();
                });
            } else {
                disconnect();
                core::SNodeC::stop();
            }
        }
    }

    void Client::onReady(::mcp::ProtocolEra era, const Json& capabilities) {
        std::cerr << "MCP ready " << (era == ::mcp::ProtocolEra::PerRequest20260728 ? "2026-07-28" : "2025-11-25") << std::endl;
        if (config.getCommand() == "tour") {
            std::cout << Json{{"feature", "negotiation"},
                              {"era", era == ::mcp::ProtocolEra::PerRequest20260728 ? "2026-07-28" : "2025-11-25"},
                              {"capabilities", capabilities}}
                             .dump()
                      << std::endl;
            plan();
            execute();
        } else {
            command();
        }
    }

    void Client::onTools(const jsonrpc::messages::Id&, const ::mcp::ToolsListResult& result) {
        finish(config.getCommand() == "tour" ? Json(result.tools) : Json{{"tools", result.tools}});
    }

    void Client::onResources(const jsonrpc::messages::Id&, const ::mcp::ResourcesListResult& result) {
        finish(config.getCommand() == "tour" ? Json(result.resources) : Json{{"resources", result.resources}});
    }

    void Client::onResourceTemplates(const jsonrpc::messages::Id&, const ::mcp::ResourceTemplatesListResult& result) {
        finish(config.getCommand() == "tour" ? Json(result.resourceTemplates) : Json{{"resourceTemplates", result.resourceTemplates}});
    }

    void Client::onPrompts(const jsonrpc::messages::Id&, const ::mcp::PromptsListResult& result) {
        finish(config.getCommand() == "tour" ? Json(result.prompts) : Json{{"prompts", result.prompts}});
    }

    void Client::onToolResult(const jsonrpc::messages::Id&, const ::mcp::ToolResult& result) {
        finish(result.toJson());
    }

    void Client::onResourceResult(const jsonrpc::messages::Id&, const ::mcp::ResourceResult& result) {
        Json contents = Json::array();
        for (const auto& item : result.contents) {
            std::visit(
                [&contents](const auto& value) {
                    Json content{{"uri", value.uri}};
                    if constexpr (requires { value.text; }) {
                        content["text"] = value.text;
                    } else {
                        content["blob"] = value.base64;
                    }
                    contents.push_back(std::move(content));
                },
                item);
        }
        finish({{"contents", contents}});
    }

    void Client::onPromptResult(const jsonrpc::messages::Id&, const ::mcp::PromptResult& result) {
        const auto* text = result.messages.empty() ? nullptr : std::get_if<::mcp::TextContent>(&result.messages.front().content);
        finish({{"messages", result.messages.size()}, {"firstText", text == nullptr ? std::string{} : text->text}});
    }

    void Client::onCompletion(const jsonrpc::messages::Id&, const ::mcp::CompletionResult& result) {
        finish({{"values", result.values}, {"hasMore", result.hasMore}});
    }

    void Client::onOtherResult(const jsonrpc::messages::Id&, const std::string& method, const Json& result) {
        if (method != "tasks/cancel" || config.getCommand() != "tour") {
            finish(config.getCommand() == "tour" ? result : Json{{"method", method}, {"result", result}});
        }
    }

    void Client::onRemoteError(const jsonrpc::messages::Error& error) {
        finish({{"code", error.code}, {"message", error.message}}, Outcome::Error);
    }

    void Client::onTimeout(const jsonrpc::messages::Id&, const std::string& method) {
        finish({{"timeout", method}}, Outcome::Timeout);
    }

    void Client::onCancelled(const jsonrpc::messages::Id&, const std::string& reason) {
        finish({{"cancelled", reason}}, Outcome::Cancelled);
    }

    void Client::onElicitation(const std::string& key,
                               const std::variant<::mcp::FormInput, ::mcp::UrlInput>& input,
                               const std::shared_ptr<::mcp::client::ElicitationReply>& reply) {
        ++inputs;
        std::cout << Json{{"elicitation", key}, {"mode", std::holds_alternative<::mcp::FormInput>(input) ? "form" : "url"}}.dump()
                  << std::endl;
        const bool form = std::holds_alternative<::mcp::FormInput>(input);
        const auto action = config.getCommand() == "tour" ? config.getAnswerAction() : ::mcp::ElicitationAction::Decline;
        const Json content = form && action == ::mcp::ElicitationAction::Accept ? Json{{"label", config.getAnswer()}} : Json();
        core::timer::Timer::singleshotTimer(
            [reply, response = ::mcp::InputResponse{action, content}] {
                reply->result(response);
            },
            utils::Timeval(0.01));
    }

    void Client::onProgress(const ::mcp::ProgressToken&, const ::mcp::Progress& value) {
        ++progress;
        std::cerr << "progress " << value.value << std::endl;
    }

    void Client::onToolsListChanged(const std::optional<jsonrpc::messages::Id>&) {
        ++toolChanges;
        if (config.getWatch() && config.getCommand() != "tour") {
            std::cout << "tools/list_changed" << std::endl;
        }
    }

    void Client::onResourcesListChanged(const std::optional<jsonrpc::messages::Id>&) {
        ++resourceChanges;
        if (config.getWatch() && config.getCommand() != "tour") {
            std::cout << "resources/list_changed" << std::endl;
        }
    }

    void Client::onPromptsListChanged(const std::optional<jsonrpc::messages::Id>&) {
        ++promptChanges;
        if (config.getWatch() && config.getCommand() != "tour") {
            std::cout << "prompts/list_changed" << std::endl;
        }
    }

    void Client::onResourceUpdated(const std::string& uri, const std::optional<jsonrpc::messages::Id>&) {
        ++updates;
        if (config.getCommand() != "tour") {
            std::cout << "resources/updated " << uri << std::endl;
        }
    }

    void Client::onLogMessage(const ::mcp::LogMessage& message) {
        ++logs;
        if (config.getCommand() != "tour") {
            std::cout << "log " << ::mcp::loggingLevelName(message.level) << " " << message.data.dump() << std::endl;
        }
    }

    void Client::onSubscriptionAcknowledged(const jsonrpc::messages::Id&, const Json& filter) {
        if (filter.contains("taskIds")) {
            ++taskAcknowledgements;
        } else {
            finish(filter);
        }
    }

    void Client::onTaskCreated(const jsonrpc::messages::Id&, const ::mcp::TaskInfo& task) {
        ++tasks;
        std::cerr << "task " << task.taskId << std::endl;
        lastTask = task.taskId;
        if (config.getCommand() == "tour" && !legacy() && steps[current].feature == "task.required") {
            listen({{"taskIds", {task.taskId}}});
        }
        if (config.getCommand() == "tour" && steps[current].feature == "task.cancel") {
            cancelTask(task.taskId);
        }
    }

    void Client::onTaskStatus(const ::mcp::TaskInfo& task) {
        ++statuses;
        std::cerr << "task status " << task.taskId << " " << static_cast<int>(task.status) << std::endl;
        if (config.getCommand() == "tour" && steps[current].feature == "task.get" && task.taskId == lastTask) {
            finish({{"status", static_cast<int>(task.status)}});
        }
    }

    void
    Client::onTasks(const jsonrpc::messages::Id&, const std::vector<::mcp::TaskInfo>& entries, const std::optional<std::string>& cursor) {
        finish({{"tasks", entries.size()}, {"nextCursor", cursor.value_or("")}});
    }

} // namespace apps::mcp
