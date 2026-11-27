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
#include "jsonrpc/JsonRpcContext.h"
#include "jsonrpc/messages/Error.h"
#include "mcp/ClientInputs.h"
#include "mcp/Protocol.h"
#include "mcp/client/Mcp.h"
#include "mcp/server/Request.h"
#include "mcp/server/Server.h"
#include "mcp/server/Tool.h"
#include "mcp/server/detail/Server.h"
#include "tests/support/TestResult.h"
#ifndef DOXYGEN_SHOULD_SKIP_THIS
#include <exception>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
#endif
namespace {
    using mcp::Json;
    namespace detail = mcp::server::detail;
    const Json sampled{{"role", "assistant"},
                       {"model", "test-model"},
                       {"content", {{"type", "text"}, {"text", "42"}}},
                       {"_meta", {{"test/sample", true}}}};
    const Json roots{{"roots", {{{"uri", "file:///test/workspace"}, {"name", "Test"}, {"_meta", {{"test/root", true}}}}}},
                     {"_meta", {{"test/roots", true}}}};

    mcp::SamplingRequest sampling() {
        return mcp::SamplingRequest(
            {{"messages", {{{"role", "user"}, {"content", {{"type", "text"}, {"text", "Answer"}}}}}}, {"maxTokens", 32}});
    }

    class InputsTool final : public mcp::server::Tool {
    public:
        InputsTool()
            : Tool([] {
                mcp::server::ToolDefinition definition{"inputs", {}, {{"type", "object"}}};
                definition.taskSupport = mcp::TaskSupport::Optional;
                return definition;
            }()) {
        }

        ~InputsTool() final;
        int changes = 0;

        void call(const mcp::server::ToolCall& request, const std::shared_ptr<mcp::server::ToolReply>& reply) final {
            Json parameters = sampling().toJson();
            if (request.arguments.value("tools", false)) {
                parameters["tools"] = Json::array();
            }
            if (request.arguments.value("context", false)) {
                parameters["includeContext"] = "thisServer";
            }
            const mcp::SamplingRequest input(parameters);
            if (request.arguments.value("task", false) && !request.context.getTaskId()) {
                reply->startTask({1000, 1});
            }
            const auto sample = request.context.getSamplingResponse("sample", input);
            const auto root = request.context.getRootsResponse("roots");
            const auto form = request.context.getInputResponses().find("form");
            if (sample && root && form != request.context.getInputResponses().end()) {
                reply->result(mcp::ToolResult::json(
                    {{"sample", sample->toJson()}, {"roots", root->toJson()}, {"state", *request.context.getRequestState()}}));
            } else {
                request.context.onRootsChanged([this] {
                    ++changes;
                });
                reply->clientInputRequired({{{"sample", input},
                                             {"roots", mcp::RootsRequest{}},
                                             {"form", mcp::FormInput{"Label", {{"type", "object"}, {"properties", Json::object()}}}}},
                                            "owned-state"});
            }
        }
    };

    InputsTool::~InputsTool() = default;

    class Peer final : public mcp::client::Mcp {
    public:
        Peer(bool legacy, bool enabled = true, bool tools = true, bool context = true)
            : Mcp([legacy] {
                mcp::client::Options options;
                options.negotiation = legacy ? mcp::client::Negotiation::Legacy : mcp::client::Negotiation::Modern;
                options.elicitationTimeout = utils::Timeval(.01);
                return options;
            }()) {
            if (enabled) {
                setSamplingHandler(
                    [this](const std::string&, const mcp::SamplingRequest&, const std::shared_ptr<mcp::client::ClientReply>& reply) {
                        if (retain) {
                            replies.push_back(reply);
                            reply->onCancel([this] {
                                ++cancelled;
                                if (updated) {
                                    updated();
                                }
                            });
                        } else if (refuse) {
                            reply->error(mcp::ErrorCode::InvalidParams, "Denied by application");
                        } else if (throwing) {
                            throw std::runtime_error("test");
                        } else if (!forget) {
                            reply->result(mcp::SamplingResult(sampled));
                        }
                    },
                    tools,
                    context);
                setRootsHandler(
                    [this](const std::string&, const std::shared_ptr<mcp::client::ClientReply>& reply) {
                        if (retain) {
                            replies.push_back(reply);
                            reply->onCancel([this] {
                                ++cancelled;
                                if (updated) {
                                    updated();
                                }
                            });
                        } else {
                            reply->result(mcp::RootsResult(rootsValue));
                        }
                    },
                    true);
            }
        }

        ~Peer() final;
        bool retain = false;
        bool refuse = false;
        bool throwing = false;
        bool forget = false;
        int cancelled = 0;
        int errors = 0;
        std::function<void()> updated;
        Json rootsValue = roots;
        std::string taskId;
        mcp::TaskState status = mcp::TaskState::Working;
        std::vector<Json> results;
        std::vector<std::shared_ptr<mcp::client::ClientReply>> replies;

    private:
        void onToolResult(const Json&, const mcp::ToolResult& result) final {
            results.push_back(result.toJson());
            if (updated) {
                updated();
            }
        }

        void onRemoteError(const jsonrpc::messages::Error&) final {
            ++errors;
            if (updated) {
                updated();
            }
        }

        void onTaskCreated(const Json&, const mcp::TaskInfo& task) final {
            taskId = task.taskId;
        }

        void onTaskStatus(const mcp::TaskInfo& task) final {
            status = task.status;
        }
    };

    Peer::~Peer() = default;

    class Context final : public jsonrpc::JsonRpcContext {
    public:
        Context(Peer* peer, mcp::server::Server& server, bool legacy)
            : JsonRpcContext(peer)
            , peer(peer)
            , server(server)
            , legacy(legacy)
            , channel(std::make_shared<detail::Channel>()) {
            channel->output = [this](const Json& value) {
                if (!value.is_null()) {
                    onMessage(value.dump());
                }
            };
            onConnected();
        }

        ~Context() final;
        Peer* peer;
        mcp::server::Server& server;
        bool legacy;
        std::shared_ptr<detail::Channel> channel;
        std::vector<Json> output;
        std::vector<std::shared_ptr<detail::Response>> pending;

        void sendMessage(const std::string& text) final {
            const Json value = Json::parse(text);
            output.push_back(value);
            if (value.contains("result") || value.contains("error")) {
                channel->respond(value);
            } else if (value.contains("id")) {
                const auto reply = detail::Access::registry(server).dispatch(
                    value,
                    legacy ? mcp::ProtocolEra::Initialization20251125 : mcp::ProtocolEra::PerRequest20260728,
                    [this](const Json& message, bool) {
                        if (!message.is_null()) {
                            onMessage(message.dump());
                        }
                    },
                    utils::Timeval(30),
                    {},
                    channel);
                if (!reply->done) {
                    pending.push_back(reply);
                }
            } else if (value["method"] == "notifications/cancelled") {
                for (const auto& response : pending) {
                    if (response->id == value["params"]["requestId"]) {
                        response->cancel();
                    }
                }
            } else if (value["method"] == "notifications/roots/list_changed") {
                channel->rootsChanged();
            }
        }

        void end() final {
            onDisconnected();
        }

        void close() final {
            onDisconnected();
        }

        core::socket::stream::SocketConnection* getSocketConnection() const final {
            return nullptr;
        }
    };

    Context::~Context() {
        onDisconnected();
        detail::Access::registry(server).endChannel(channel);
    }
} // namespace

int main(int argc, char* argv[]) {
    core::SNodeC::init(argc, argv);
    tests::support::TestResult test;
    std::vector<std::unique_ptr<mcp::server::Server>> timeoutServers;
    std::vector<std::unique_ptr<Context>> timeouts;
    std::vector<std::unique_ptr<Context>> tasks;
    for (const bool legacy : {false, true}) {
        mcp::server::Server server("inputs", "1");
        const auto tool = std::make_shared<InputsTool>();
        server.tool(tool);
        Context context(new Peer(legacy), server, legacy);
        const auto id = context.peer->callTool("inputs");
        if (!legacy) {
            for (const auto& message : context.output) {
                if (message.value("method", "") == "tools/call") {
                    test.expectTrue(!message["params"]["_meta"][mcp::capabilitiesKey]["roots"].contains("listChanged"),
                                    "modern root capabilities omit the removed legacy notification field");
                }
            }
        }
        test.expectTrue(!id.is_null() && context.peer->results.size() == 1,
                        "mixed round completes through public client/server APIs in both eras");
        if (!context.peer->results.empty()) {
            const auto& value = context.peer->results.back()["structuredContent"];
            test.expectTrue(value["sample"] == sampled && value["roots"] == roots && value["state"] == "owned-state",
                            "sampling/root metadata and request state survive round trip");
        }
        test.expectTrue(context.peer->notifyRootsChanged() == (legacy ? mcp::Completion::Accepted : mcp::Completion::Inactive) &&
                            tool->changes == (legacy ? 1 : 0),
                        "roots changes belong only to the advertised legacy session");
        if (legacy) {
            context.channel->capabilities["roots"]["listChanged"] = "invalid";
            context.peer->notifyRootsChanged();
            test.expectTrue(tool->changes == 1, "malformed roots notification capability is ignored without escaping the event loop");
            context.channel->capabilities["roots"]["listChanged"] = true;
        }
        context.peer->rootsValue["roots"][0]["uri"] = "file:///test/changed";
        context.peer->callTool("inputs");
        test.expectTrue(context.peer->results.back()["structuredContent"]["roots"] == context.peer->rootsValue,
                        "later operations obtain changed roots from the application instead of a library cache");
        context.peer->retain = true;
        const auto retained = context.peer->callTool("inputs");
        test.expectTrue(context.peer->replies.size() == 2, "asynchronous sampling and roots handles can be retained independently");
        const auto rootReply = context.peer->replies[0];
        const auto sampleReply = context.peer->replies[1];
        test.expectTrue(rootReply->result(mcp::SamplingResult(sampled)) == mcp::Completion::Inactive,
                        "wrong result kind cannot complete a roots request");
        test.expectTrue(sampleReply->result(mcp::SamplingResult(sampled)) == mcp::Completion::Accepted &&
                            rootReply->result(mcp::RootsResult(roots)) == mcp::Completion::Accepted,
                        "out-of-order typed replies complete a mixed round once");
        test.expectTrue(sampleReply->result(mcp::SamplingResult(sampled)) == mcp::Completion::Inactive, "duplicate completion is inactive");
        context.peer->replies.clear();
        const auto cancelled = context.peer->callTool("inputs");
        context.peer->cancel(cancelled);
        test.expectTrue(context.peer->cancelled == 2 &&
                            context.peer->replies.back()->result(mcp::SamplingResult(sampled)) == mcp::Completion::Inactive,
                        "request cancellation invalidates all retained input handles");
        context.peer->replies.clear();
        context.peer->callTool("inputs");
        context.peer->disconnect();
        test.expectTrue(context.peer->cancelled == 4 &&
                            context.peer->replies.back()->result(mcp::SamplingResult(sampled)) == mcp::Completion::Inactive,
                        "disconnect cancels application work and suppresses late answers");
        static_cast<void>(retained);
        if (legacy) {
            Context ordinary(new Peer(true), server, true);
            Json params = sampling().toJson();
            params["task"] = {{"ttl", 1000}};
            ordinary.onMessage(
                Json{{"jsonrpc", "2.0"}, {"id", "unadvertised-sampling-task"}, {"method", "sampling/createMessage"}, {"params", params}}
                    .dump());
            const auto& response = ordinary.output.back();
            test.expectTrue(response.contains("result") && response["result"] == sampled,
                            "unadvertised sampling task augmentation is ignored and returns the ordinary sampled message");
        }
        Context unsupported(new Peer(legacy, false), server, legacy);
        for (const std::string method : {"roots/list", "sampling/createMessage"}) {
            unsupported.onMessage(Json{{"jsonrpc", "2.0"}, {"id", "unsupported"}, {"method", method}, {"params", Json::object()}}.dump());
            test.expectTrue(unsupported.output.back()["error"]["code"] == -32601,
                            "unimplemented client input methods return Method not found");
        }
        for (const int mode : {0, 1, 2, 3}) {
            Context failing(new Peer(legacy, mode != 0), server, legacy);
            failing.peer->refuse = mode == 1;
            failing.peer->throwing = mode == 2;
            failing.peer->forget = mode == 3;
            failing.peer->callTool("inputs");
            test.expectTrue(legacy ? failing.peer->results.size() == 1 && failing.peer->results[0].value("isError", false)
                                   : failing.peer->errors == 1,
                            "missing capability, refusal, exception and abandoned replies terminate without continuation");
        }
        for (const std::string feature : {"tools", "context"}) {
            Context missing(new Peer(legacy, true, false, false), server, legacy);
            missing.peer->callTool("inputs", {{feature, true}});
            test.expectTrue(legacy ? missing.peer->results.size() == 1 && missing.peer->results[0].value("isError", false)
                                   : missing.peer->errors == 1,
                            "sampling tools/context require their own capabilities before any mixed input is emitted");
        }
        timeoutServers.push_back(std::make_unique<mcp::server::Server>("timeout", "1"));
        timeoutServers.back()->tool(std::make_shared<InputsTool>());
        timeouts.push_back(std::make_unique<Context>(new Peer(legacy), *timeoutServers.back(), legacy));
        timeouts.back()->peer->retain = true;
        timeouts.back()->peer->callTool("inputs");
        tasks.push_back(std::make_unique<Context>(new Peer(legacy), *timeoutServers.back(), legacy));
        mcp::client::RequestOptions taskOptions;
        taskOptions.task = true;
        tasks.back()->peer->callTool("inputs", {{"task", true}}, taskOptions);
    }
    Context refused(new Peer(false), *timeoutServers[0], false);
    refused.peer->refuse = true;
    refused.peer->callTool("inputs", {{"task", true}});
    const auto completed = [&] {
        bool done = refused.peer->errors == 1;
        for (const auto& context : tasks) {
            done = done && context->peer->results.size() == 1;
        }
        for (const auto& context : timeouts) {
            done = done && context->peer->cancelled == 2;
        }
        if (done) {
            core::SNodeC::stop();
        }
    };
    for (const auto& context : tasks) {
        context->peer->updated = completed;
    }
    for (const auto& context : timeouts) {
        context->peer->updated = completed;
    }
    refused.peer->updated = completed;
    core::timer::Timer::singleshotTimer(
        [] {
            core::SNodeC::stop();
        },
        utils::Timeval(2));
    core::SNodeC::start();
    for (const auto& context : timeouts) {
        test.expectTrue(context->peer->cancelled == 2, "input timeout cancels application work and sibling inputs");
        test.expectTrue(context->peer->replies.back()->result(mcp::SamplingResult(sampled)) == mcp::Completion::Inactive,
                        "timed-out handles cannot send later results");
    }
    for (const auto& context : tasks) {
        Json expectedSample = sampled;
        Json expectedRoots = roots;
        if (context->legacy && context->peer->results.size() == 1) {
            const auto related = context->peer->results[0]["_meta"]["io.modelcontextprotocol/related-task"];
            test.expectTrue(related["taskId"].is_string() && !related["taskId"].get<std::string>().empty(),
                            "legacy task result has its protocol correlation");
            expectedSample["_meta"]["io.modelcontextprotocol/related-task"] = related;
            expectedRoots["_meta"]["io.modelcontextprotocol/related-task"] = related;
        }
        test.expectTrue(context->peer->results.size() == 1 &&
                            context->peer->results[0]["structuredContent"] ==
                                Json{{"sample", expectedSample}, {"roots", expectedRoots}, {"state", "owned-state"}},
                        "task input/update/result preserves all payloads and related-task metadata in both eras");
    }
    refused.peer->getTask(refused.peer->taskId);
    test.expectTrue(refused.peer->errors == 1 && refused.peer->status == mcp::TaskState::Cancelled,
                    "refused modern task input cancels the server task, retires polling and reports one error");
    const Json valid = sampling().toJson();
    for (const Json& patch : Json::array({Json{{"maxTokens", 0}},
                                          Json{{"messages", {{{"role", "invalid"}, {"content", {{"type", "text"}, {"text", "x"}}}}}}},
                                          Json{{"toolChoice", {{"mode", "invalid"}}}},
                                          Json{{"modelPreferences", {{"costPriority", 2}}}},
                                          Json{{"includeContext", "invalid"}}})) {
        Json value = valid;
        value.merge_patch(patch);
        bool rejected = false;
        try {
            static_cast<void>(mcp::SamplingRequest(value));
        } catch (const std::exception&) {
            rejected = true;
        }
        test.expectTrue(rejected, "invalid sampling parameters rejected before protocol emission");
    }
    for (const Json& value : Json::array({Json{{"roots", {{{"uri", "https://example.com"}}}}},
                                          Json{{"roots", {{{"uri", "file:///same"}}, {{"uri", "file:///same"}}}}},
                                          Json{{"roots", Json::object()}}})) {
        bool rejected = false;
        try {
            static_cast<void>(mcp::RootsResult(value));
        } catch (const std::exception&) {
            rejected = true;
        }
        test.expectTrue(rejected, "invalid and duplicate roots rejected");
    }
    for (const std::string type : {"image", "audio"}) {
        Json value = sampling().toJson();
        value["messages"][0]["content"] = {{"type", type}, {"data", "AAEC"}, {"mimeType", type == "image" ? "image/png" : "audio/wav"}};
        test.expectTrue(mcp::SamplingRequest(value).toJson() == value, "sampling media retains its exact content and MIME type");
    }
    Json loop = sampling().toJson();
    loop["messages"].push_back(
        {{"role", "assistant"}, {"content", {{{"type", "tool_use"}, {"id", "a"}, {"name", "answer"}, {"input", Json::object()}}}}});
    loop["messages"].push_back({{"role", "user"},
                                {"content",
                                 {{{"type", "tool_result"},
                                   {"toolUseId", "a"},
                                   {"content", {{{"type", "text"}, {"text", "42"}}}},
                                   {"_meta", {{"test/cache", true}}}}}}});
    test.expectTrue(mcp::SamplingRequest(loop).toJson() == loop, "balanced sampling tool loop preserves result metadata");
    loop["messages"][2]["content"][0]["toolUseId"] = "unmatched";
    bool rejectedLoop = false;
    try {
        static_cast<void>(mcp::SamplingRequest(loop));
    } catch (const std::exception&) {
        rejectedLoop = true;
    }
    test.expectTrue(rejectedLoop, "sampling tool results must match the preceding tool uses");
    return test.processResult();
}
