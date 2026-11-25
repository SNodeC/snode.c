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

#include "mcp/server/detail/Tasks.h"

#include "core/timer/Timer.h"
#include "jsonrpc/messages/Notification.h"
#include "jsonrpc/messages/Result.h"
#include "mcp/Protocol.h"
#include "mcp/Schema.h"
#include "mcp/detail/Utilities.h"
#include "mcp/server/Mcp.h"
#include "mcp/server/Server.h"
#include "mcp/server/Types.h"
#include "mcp/server/detail/Server.h"

#include <cmath>
#include <compare>
#ifndef DOXYGEN_SHOULD_SKIP_THIS
#include <algorithm>
#include <map>
#include <memory>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <utility>
#include <vector>
// IWYU pragma: no_include <nlohmann/detail/iterators/iter_impl.hpp>
// IWYU pragma: no_include <nlohmann/detail/iterators/iteration_proxy.hpp>
// IWYU pragma: no_include <nlohmann/detail/json_ref.hpp>
#endif
namespace mcp::server::detail {
    bool Task::terminal() const {
        return state["status"] == "completed" || state["status"] == "failed" || state["status"] == "cancelled";
    }

    bool Task::visible(const Response& request) const {
        const bool modern = execution->era == ProtocolEra::PerRequest20260728;
        return execution->era == request.era && execution->principal.has_value() == request.principal.has_value() &&
               (!execution->principal || execution->principal->subject == request.principal->subject) &&
               (modern || (execution->channel.lock() && execution->channel.lock() == request.channel.lock()));
    }

    Json Task::info(bool detailed) const {
        Json value = state;
        if (detailed) {
            if (state["status"] == "input_required") {
                value["inputRequests"] = inputs;
            } else if (payload.contains("error")) {
                value["error"] = payload["error"];
            } else if (payload.contains("result")) {
                value["result"] = payload["result"];
            }
        }
        return value;
    }

    Tasks::~Tasks() {
        shutdown();
    }

    void Tasks::shutdown() {
        closing = true;
        notify = {};
        const auto pending = std::move(entries);
        for (const auto& [id, task] : pending) {
            static_cast<void>(id);
            task->execution->timer.cancel();
            task->execution->cancel(CancelReason::Shutdown);
            for (const auto& weak : task->waiters) {
                if (const auto waiter = weak.lock()) {
                    waiter->error(-32603, "Task store shutting down");
                }
            }
        }
    }

    bool Tasks::negotiated(const Json& capabilities) {
        const auto extensions = capabilities.value("extensions", Json());
        return extensions.is_object() && extensions.contains("io.modelcontextprotocol/tasks") &&
               extensions["io.modelcontextprotocol/tasks"].is_object();
    }

    Completion Tasks::begin(const std::shared_ptr<Response>& execution, TaskOptions options) {
        Completion accepted = Completion::Inactive;
        const bool modern = execution->era == ProtocolEra::PerRequest20260728;
        if (!execution->done && execution->task.expired()) {
            if (modern && !negotiated(execution->clientCapabilities)) {
                execution->error(-32021,
                                 "Missing required client capability",
                                 {{"requiredCapabilities", {{"extensions", {{"io.modelcontextprotocol/tasks", Json::object()}}}}}});
            } else if (options.ttlMs == 0 || options.pollIntervalMs == 0 || options.pollIntervalMs > 60000) {
                execution->error(-32602, "Invalid task parameters");
            } else if ((!modern && execution->channel.expired()) || closing) {
                execution->error(-32602, "No active task requestor");
            } else if (entries.size() >= 128 ||
                       static_cast<std::size_t>(std::count_if(entries.begin(), entries.end(), [&](const auto& entry) {
                           const auto& task = entry.second;
                           return task->execution->era == execution->era &&
                                  (modern ? task->visible(*execution) : task->execution->channel.lock() == execution->channel.lock());
                       })) >= perRequestor) {
                execution->error(-32000, "Task capacity exceeded");
            } else {
                options.ttlMs = std::min(options.ttlMs, maximumTtlMs);
                auto task = std::make_shared<Task>();
                const std::string id = mcp::detail::randomToken();
                const std::string now = mcp::detail::timestamp();
                task->state = {{"taskId", id},
                               {"status", "working"},
                               {"createdAt", now},
                               {"lastUpdatedAt", now},
                               {modern ? "ttlMs" : "ttl", options.ttlMs},
                               {modern ? "pollIntervalMs" : "pollInterval", options.pollIntervalMs}};
                task->execution = execution;
                task->store = shared_from_this();
                task->expires = utils::Timeval::currentTime() + utils::Timeval(static_cast<double>(options.ttlMs) / 1000.0);
                entries.emplace(id, task);
                execution->task = task;
                execution->deadline = task->expires;
                auto previous = std::move(execution->timer);
                previous.cancel();
                const std::weak_ptr<Tasks> weak = shared_from_this();
                execution->timer = core::timer::Timer::singleshotTimer(
                    [weak, id] {
                        if (const auto store = weak.lock()) {
                            store->expire(id);
                        }
                    },
                    utils::Timeval(static_cast<double>(options.ttlMs) / 1000.0));
                const auto send = execution->output;
                const std::weak_ptr<Task> current = task;
                execution->output = [weak, current](const Json& value, bool terminal) {
                    if (const auto store = weak.lock()) {
                        if (const auto record = current.lock()) {
                            store->output(record, value, terminal);
                        }
                    }
                };
                Json handle = modern ? task->info(false) : Json{{"task", task->info(false)}};
                if (modern) {
                    handle["resultType"] = "task";
                    handle["_meta"]["io.modelcontextprotocol/serverInfo"] = execution->serverInfo;
                } else {
                    handle["_meta"]["io.modelcontextprotocol/related-task"] = {{"taskId", id}};
                }
                accepted = Completion::Accepted;
                send(jsonrpc::messages::Result{execution->id, std::move(handle)}.toJson(), true);
            }
        }
        return accepted;
    }

    void Tasks::expire(const std::string& id, CancelReason reason) {
        const auto found = entries.find(id);
        if (found != entries.end()) {
            const auto task = found->second;
            entries.erase(found);
            task->execution->timer.cancel();
            task->execution->cancel(reason);
            for (const auto& weak : task->waiters) {
                if (const auto waiter = weak.lock()) {
                    waiter->error(-32602, "Task expired");
                }
            }
        }
    }

    void Tasks::release(const std::shared_ptr<Channel>& channel) {
        std::vector<std::string> owned;
        for (const auto& [id, task] : entries) {
            if (task->execution->channel.lock() == channel) {
                owned.push_back(id);
            }
        }
        for (const auto& id : owned) {
            expire(id, CancelReason::Disconnected);
        }
    }

    void Tasks::working(const std::shared_ptr<Task>& task) {
        if (!task->terminal()) {
            task->state["status"] = "working";
            task->inputs = Json::object();
            publish(task);
        }
    }

    void Tasks::input(const std::shared_ptr<Task>& task, const Json& value) {
        if (!task->terminal()) {
            const auto requests = value.value("inputRequests", Json::object());
            bool valid =
                requests.is_object() && !requests.empty() && task->inputs.empty() && task->usedInputs.size() + requests.size() <= 1024;
            for (const auto& [key, request] : requests.items()) {
                static_cast<void>(request);
                valid = valid && !task->usedInputs.contains(key);
            }
            if (!valid) {
                task->execution->error(-32603, "Task input keys must be unique over the task lifetime");
            } else {
                for (const auto& [key, request] : requests.items()) {
                    static_cast<void>(request);
                    task->usedInputs.insert(key);
                }
                task->inputs = requests;
                if (value.contains("requestState")) {
                    task->execution->requestState = value["requestState"].get<std::string>();
                }
                task->state["status"] = "input_required";
                publish(task);
            }
        }
    }

    void Tasks::output(const std::shared_ptr<Task>& task, const Json& message, bool terminal) {
        if (!closing && !task->terminal() && entries.contains(task->state["taskId"].get<std::string>())) {
            if (!terminal) {
                if (task->execution->era == ProtocolEra::Initialization20251125 && task->execution->protocol) {
                    task->execution->protocol->sendTaskMessage(message);
                }
            } else {
                Json value = message;
                if (!value.is_null() && value.dump().size() > 1048576) {
                    value = {{"error", {{"code", -32603}, {"message", "Task result limit exceeded"}}}};
                }
                task->payload = value;
                task->state["status"] =
                    value.is_null()                                                                                          ? "cancelled"
                    : value.contains("error")                                                                                ? "failed"
                    : task->execution->era == ProtocolEra::Initialization20251125 && value["result"].value("isError", false) ? "failed"
                                                                                                                             : "completed";
                publish(task);
            }
        }
    }

    void Tasks::publish(const std::shared_ptr<Task>& task) {
        task->state["lastUpdatedAt"] = mcp::detail::timestamp();
        if (!closing && notify) {
            notify(task);
        }
        if (task->terminal()) {
            const auto pending = std::move(task->waiters);
            for (const auto& weak : pending) {
                if (const auto waiter = weak.lock()) {
                    result(task, waiter);
                }
            }
        }
    }

    void Tasks::result(const std::shared_ptr<Task>& task, const std::shared_ptr<Response>& reply) {
        if (task->payload.contains("error")) {
            const auto& error = task->payload["error"];
            reply->error(error["code"].get<int>(), error["message"].get<std::string>(), error.value("data", Json()));
        } else if (task->payload.contains("result")) {
            reply->result(task->payload["result"]);
        } else {
            reply->error(-32603, "Task cancelled");
        }
    }

    bool Tasks::dispatch(const std::string& method, const Json& params, const std::shared_ptr<Response>& reply, std::size_t pageSize) {
        const bool modern = reply->era == ProtocolEra::PerRequest20260728;
        const bool handled = method == "tasks/get" || method == "tasks/cancel" ||
                             (!modern && (method == "tasks/list" || method == "tasks/result")) || (modern && method == "tasks/update");
        if (handled) {
            if (modern && !negotiated(reply->clientCapabilities)) {
                reply->error(-32021,
                             "Missing required client capability",
                             {{"requiredCapabilities", {{"extensions", {{"io.modelcontextprotocol/tasks", Json::object()}}}}}});
            } else if (method == "tasks/list") {
                const auto cursor = params.value("cursor", Json());
                const bool validCursor = !params.contains("cursor") || (cursor.is_string() && entries.contains(cursor.get<std::string>()) &&
                                                                        entries.at(cursor.get<std::string>())->visible(*reply));
                if (!validCursor) {
                    reply->error(-32602, "Invalid task cursor");
                } else {
                    Json values = Json::array();
                    std::string last;
                    bool more = false;
                    for (const auto& [id, task] : entries) {
                        if (task->visible(*reply) && (!params.contains("cursor") || id > cursor.get<std::string>()) &&
                            task->expires > utils::Timeval::currentTime()) {
                            if (values.size() < pageSize) {
                                values.push_back(task->info(false));
                                last = id;
                            } else {
                                more = true;
                            }
                        }
                    }
                    Json value{{"tasks", std::move(values)}};
                    if (more) {
                        value["nextCursor"] = last;
                    }
                    reply->result(std::move(value));
                }
            } else {
                const auto id = params.value("taskId", Json());
                auto found = id.is_string() ? entries.find(id.get<std::string>()) : entries.end();
                if (found == entries.end() || !found->second->visible(*reply) || found->second->expires <= utils::Timeval::currentTime()) {
                    reply->error(-32602, "Unknown or expired task");
                } else {
                    const auto task = found->second;
                    if (method == "tasks/get") {
                        reply->result(task->info(modern));
                    } else if (method == "tasks/cancel") {
                        if (!modern && task->terminal()) {
                            reply->error(-32602, "Task already terminal");
                        } else {
                            task->execution->cancel();
                            reply->result(modern ? Json::object() : task->info(false));
                        }
                    } else if (method == "tasks/result") {
                        if (task->terminal()) {
                            result(task, reply);
                        } else if (task->waiters.size() >= 128) {
                            reply->error(-32603, "Task result wait limit exceeded");
                        } else {
                            reply->timer.cancel();
                            reply->deadline = task->expires;
                            task->waiters.push_back(reply);
                        }
                    } else {
                        const Json responses = params.value("inputResponses", Json());
                        bool valid = responses.is_object() && responses.size() <= 16;
                        std::map<std::string, InputResponse> accepted;
                        if (valid) {
                            for (const auto& [key, response] : responses.items()) {
                                if (task->inputs.contains(key)) {
                                    const auto action = response.is_object() ? response.value("action", Json()) : Json();
                                    const auto content = response.is_object() ? response.value("content", Json()) : Json();
                                    valid = valid && (action == "accept" || action == "decline" || action == "cancel") &&
                                            (content.is_null() || content.is_object());
                                    if (valid && action == "accept") {
                                        const auto schema = task->inputs[key]["params"].value("requestedSchema", Json());
                                        std::string error;
                                        valid = schema.is_null() || Schema::validate(schema, content, error);
                                    }
                                    if (valid) {
                                        accepted[key] = InputResponse{action == "accept"    ? ElicitationAction::Accept
                                                                      : action == "decline" ? ElicitationAction::Decline
                                                                                            : ElicitationAction::Cancel,
                                                                      content};
                                    }
                                }
                            }
                        }
                        if (!valid) {
                            reply->error(-32602, "Invalid task input responses");
                        } else {
                            for (auto& [key, response] : accepted) {
                                task->execution->inputResponses[key] = std::move(response);
                                task->inputs.erase(key);
                            }
                            reply->result(Json::object());
                            if (!accepted.empty() && task->inputs.empty() && !task->terminal()) {
                                working(task);
                                const auto resume = task->execution->resume;
                                if (resume) {
                                    resume();
                                }
                            }
                        }
                    }
                }
            }
        }
        return handled;
    }
} // namespace mcp::server::detail

namespace mcp::server {
    Server::Impl::Impl()
        : tasks(std::make_shared<detail::Tasks>()) {
        tasks->notify = [this](const auto& task) {
            taskChanged(task);
        };
    }

    void Server::Impl::endChannel(const std::shared_ptr<detail::Channel>& channel) const {
        channel->active = false;
        channel->resources.clear();
        channel->output = {};
        tasks->release(channel);
    }

    void Server::Impl::taskChanged(const std::shared_ptr<detail::Task>& task) {
        if (task->execution->era == mcp::ProtocolEra::Initialization20251125) {
            if (const auto channel = task->execution->channel.lock(); channel && channel->active && channel->output) {
                channel->output(
                    jsonrpc::messages::Notification{"notifications/tasks/status", std::optional<Json>(task->info(false))}.toJson());
            }
        } else {
            std::vector<std::shared_ptr<detail::Response>> targets;
            for (const auto& weak : subscriptions) {
                if (const auto stream = weak.lock(); stream && !stream->done && task->visible(*stream)) {
                    const Json ids = stream->notifications.value("taskIds", Json::array());
                    if (std::find(ids.begin(), ids.end(), task->state["taskId"]) != ids.end()) {
                        targets.push_back(stream);
                    }
                }
            }
            for (const auto& stream : targets) {
                stream->notify("notifications/tasks", task->info(true));
            }
        }
    }
} // namespace mcp::server
