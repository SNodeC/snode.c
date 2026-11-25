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

#include "core/SNodeC.h"
#include "core/timer/Timer.h"
#include "mcp/Protocol.h"
#include "mcp/Results.h"
#include "mcp/Types.h"
#include "mcp/server/Server.h"
#include "mcp/server/detail/Server.h"
#include "tests/support/TestResult.h"
#ifndef DOXYGEN_SHOULD_SKIP_THIS
#include <memory>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <utility>
#include <vector>
#endif
int main(int argc, char* argv[]) {
    core::SNodeC::init(argc, argv);
    tests::support::TestResult test;
    {
        using mcp::Json;
        namespace detail = mcp::server::detail;
        mcp::server::Server server("tasks", "1");
        std::vector<std::shared_ptr<mcp::server::ToolReply>> pending;
        std::vector<std::string> ids;
        int cancelled = 0;
        mcp::server::ToolDefinition definition{"job", {}, {{"type", "object"}}};
        definition.taskSupport = mcp::TaskSupport::Optional;
        server.tool(definition, [&](const auto& request, const auto& reply) {
            if (request.context.getInputResponse("approve", {{"type", "object"}})) {
                reply->result(mcp::ToolResult::text("approved"));
            } else {
                if (!request.context.getTaskId()) {
                    reply->startTask(mcp::TaskOptions{request.arguments.value("ttl", std::uint64_t(60000)), 5});
                }
                if (const auto id = request.context.getTaskId()) {
                    ids.push_back(*id);
                    request.context.onCancel([&](mcp::CancelReason) {
                        ++cancelled;
                    });
                    pending.push_back(reply);
                }
            }
        });
        auto channel = std::make_shared<detail::Channel>();
        std::vector<Json> legacyNotifications;
        channel->output = [&](const Json& value) {
            legacyNotifications.push_back(value);
        };
        const Json capabilities{{"extensions", {{"io.modelcontextprotocol/tasks", Json::object()}}},
                                {"elicitation", {{"form", Json::object()}}}};
        const auto call = [&](mcp::ProtocolEra era,
                              const std::string& method,
                              Json params,
                              bool negotiated = true,
                              const std::shared_ptr<detail::Channel>& owner = nullptr,
                              std::optional<mcp::Principal> principal = {}) {
            auto messages = std::make_shared<std::vector<Json>>();
            if (era == mcp::ProtocolEra::PerRequest20260728) {
                params["_meta"] = {{mcp::versionKey, mcp::protocolVersion2026},
                                   {mcp::capabilitiesKey, negotiated ? capabilities : Json::object()}};
            }
            auto response = detail::Access::registry(server).dispatch(
                {{"jsonrpc", "2.0"}, {"id", "request"}, {"method", method}, {"params", params}},
                era,
                [messages](const Json& value, bool) {
                    if (!value.is_null()) {
                        messages->push_back(value);
                    }
                },
                utils::Timeval(30),
                principal,
                owner);
            return std::make_pair(response, messages);
        };
        const auto last = [](const auto& exchange) {
            return exchange.second->empty() ? Json() : exchange.second->back();
        };
        using Era = mcp::ProtocolEra;
        call(Era::Initialization20251125,
             "initialize",
             {{"protocolVersion", mcp::protocolVersion2025},
              {"capabilities", capabilities},
              {"clientInfo", {{"name", "test"}, {"version", "1"}}}},
             true,
             channel);
        const auto discover = call(Era::PerRequest20260728, "server/discover", Json::object());
        test.expectTrue(last(discover)["result"]["capabilities"]["extensions"].contains("io.modelcontextprotocol/tasks"),
                        "modern tasks extension advertised");
        for (const auto era : {Era::Initialization20251125, Era::PerRequest20260728}) {
            const bool modern = era == Era::PerRequest20260728;
            Json params{{"name", "job"}};
            if (!modern) {
                params["task"] = Json::object();
            }
            auto job = call(era, "tools/call", params, true, channel);
            const auto handle = modern ? last(job)["result"] : last(job)["result"]["task"];
            const std::string id = handle["taskId"];
            test.expectTrue(handle["status"] == "working" && handle.contains("createdAt") && handle.contains("lastUpdatedAt") &&
                                handle.contains(modern ? "ttlMs" : "ttl"),
                            "era-specific task handle created before execution");
            if (modern) {
                test.expectTrue(handle["resultType"] == "task" && !handle.contains("task"), "modern handle is flat and discriminated");
            }
            auto status = call(era, "tasks/get", {{"taskId", id}}, true, channel);
            test.expectTrue(last(status)["result"]["status"] == "working", "working task can be polled");
            auto isolated =
                call(era, "tasks/get", {{"taskId", id}}, true, std::make_shared<detail::Channel>(), mcp::Principal{"other", {}});
            test.expectTrue(last(isolated)["error"]["code"] == -32602, "task cannot cross principals or legacy sessions");
            auto wait = modern ? call(era, "tasks/get", {{"taskId", id}}, true, channel)
                               : call(era, "tasks/result", {{"taskId", id}}, true, channel);
            test.expectTrue(modern || wait.second->empty(), "legacy tasks/result blocks until terminal");
            auto listen = modern ? call(era, "subscriptions/listen", {{"notifications", {{"taskIds", {id}}}}}, true, channel)
                                 : call(era, "tasks/list", Json::object(), true, channel);
            pending.back()->result(mcp::ToolResult::text("finished"));
            auto completed = call(era, "tasks/get", {{"taskId", id}}, true, channel);
            test.expectTrue(last(completed)["result"]["status"] == "completed", "completion retained for polling");
            if (modern) {
                test.expectTrue(last(completed)["result"]["result"]["content"][0]["text"] == "finished" && listen.second->size() == 2 &&
                                    last(listen)["method"] == "notifications/tasks" &&
                                    last(listen)["params"]["_meta"][mcp::subscriptionKey] == "request",
                                "modern polling and opted-in task notifications include final result");
                listen.first->cancel();
            } else {
                test.expectTrue(last(wait)["result"]["content"][0]["text"] == "finished" &&
                                    last(wait)["result"]["_meta"]["io.modelcontextprotocol/related-task"]["taskId"] == id,
                                "legacy blocked result receives the original result and related-task metadata");
                const auto again = call(era, "tasks/result", {{"taskId", id}}, true, channel);
                test.expectTrue(last(again)["result"] == last(wait)["result"], "retrieving a task result does not consume it");
                test.expectTrue(last(call(era, "tasks/cancel", {{"taskId", id}}, true, channel))["error"]["code"] == -32602,
                                "legacy terminal cancellation rejected");
            }
            auto toolFailure = call(era, "tools/call", params, true, channel);
            const auto failureId = modern ? last(toolFailure)["result"]["taskId"] : last(toolFailure)["result"]["task"]["taskId"];
            pending.back()->result(mcp::ToolResult::toolError("application failure"));
            const auto failureState = last(call(era, "tasks/get", {{"taskId", failureId}}, true, channel))["result"];
            test.expectTrue(failureState["status"] == (modern ? "completed" : "failed"),
                            "tool isError maps to the task status defined by each era");
            auto protocolFailure = call(era, "tools/call", params, true, channel);
            const auto protocolId = modern ? last(protocolFailure)["result"]["taskId"] : last(protocolFailure)["result"]["task"]["taskId"];
            pending.back()->error(mcp::ErrorCode::InternalError, "execution failed");
            test.expectTrue(last(call(era, "tasks/get", {{"taskId", protocolId}}, true, channel))["result"]["status"] == "failed",
                            "JSON-RPC task execution error is terminal failure");
            job = call(era, "tools/call", params, true, channel);
            const auto cancelId = modern ? last(job)["result"]["taskId"] : last(job)["result"]["task"]["taskId"];
            const auto result = call(era, "tasks/cancel", {{"taskId", cancelId}}, true, channel);
            test.expectTrue(last(result).contains("result") &&
                                last(call(era, "tasks/get", {{"taskId", cancelId}}, true, channel))["result"]["status"] == "cancelled",
                            "task cancellation invokes existing cancellation path and retains terminal state");
        }
        test.expectTrue(cancelled == 2 && legacyNotifications.size() >= 2,
                        "cancellation observers and legacy status notifications delivered");
        test.expectTrue(last(call(Era::PerRequest20260728, "tasks/get", {{"taskId", "unknown"}}, false))["error"]["code"] == -32021,
                        "modern task methods require extension declaration on every request");
        test.expectTrue(last(call(Era::PerRequest20260728,
                                  "subscriptions/listen",
                                  {{"notifications", {{"taskIds", {"unknown"}}}}},
                                  false))["error"]["code"] == -32021,
                        "task subscription requires extension declaration");
        const auto inputJob = call(Era::PerRequest20260728, "tools/call", {{"name", "job"}}, true, channel);
        const auto inputId = last(inputJob)["result"]["taskId"];
        pending.back()->inputRequired({{{"approve", mcp::FormInput{"Approve", {{"type", "object"}, {"properties", Json::object()}}}}}});
        test.expectTrue(
            last(call(Era::PerRequest20260728, "tasks/get", {{"taskId", inputId}}))["result"]["inputRequests"].contains("approve"),
            "task exposes outstanding MRTR inputs");
        test.expectTrue(last(call(Era::PerRequest20260728,
                                  "tasks/update",
                                  {{"taskId", inputId},
                                   {"inputResponses", {{"unknown", {{"action", "accept"}}}}}}))["result"]["resultType"] == "complete",
                        "unknown task input keys ignored with acknowledgement");
        call(Era::PerRequest20260728,
             "tasks/update",
             {{"taskId", inputId}, {"inputResponses", {{"approve", {{"action", "accept"}, {"content", Json::object()}}}}}});
        test.expectTrue(last(call(Era::PerRequest20260728, "tasks/get", {{"taskId", inputId}}))["result"]["result"]["content"][0]["text"] ==
                            "approved",
                        "task input resumes the existing asynchronous reply handler");
        auto expiring = call(Era::PerRequest20260728, "tools/call", {{"name", "job"}, {"arguments", {{"ttl", 1}}}}, true, channel);
        const auto expiredId = last(expiring)["result"]["taskId"];
        auto check = core::timer::Timer::singleshotTimer(
            [&] {
                test.expectTrue(last(call(Era::PerRequest20260728, "tasks/get", {{"taskId", expiredId}}))["error"]["code"] == -32602,
                                "expired task removed");
                core::SNodeC::stop();
            },
            utils::Timeval(0.005));
        core::SNodeC::start(utils::Timeval(1));
        test.expectTrue(cancelled >= 3, "expiry follows cancellation observers");
    }
    core::SNodeC::free();
    return test.processResult();
}
