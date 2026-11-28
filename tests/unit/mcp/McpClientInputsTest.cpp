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

// IWYU pragma: no_include <nlohmann/detail/json_ref.hpp>

#include "core/SNodeC.h"
#include "core/timer/Timer.h"
#include "jsonrpc/JsonRpcContext.h"
#include "mcp/Protocol.h"
#include "mcp/Types.h"
#include "mcp/client/Mcp.h"
#include "mcp/server/Reply.h"
#include "mcp/server/Request.h"
#include "mcp/server/Server.h"
#include "mcp/server/detail/Server.h"
#include "tests/support/TestResult.h"

#include <cmath>
#ifndef DOXYGEN_SHOULD_SKIP_THIS
#include <exception>
#include <functional>
#include <initializer_list>
#include <map>
#include <memory>
#include <nlohmann/json.hpp>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <variant>
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
        std::optional<double> clientProgress;
        std::function<void()> progressObserver;

        void call(const mcp::server::ToolCall& request, const std::shared_ptr<mcp::server::ToolReply>& reply) final {
            request.context.trackClientProgress("sample", [this](const mcp::Progress& progress) {
                clientProgress = progress.value;
                if (progressObserver) {
                    progressObserver();
                }
            });
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
            reply->taskStatus(mcp::TaskState::Working, "Waiting for client inputs");
            const auto sample = request.context.getSamplingResponse("sample", input);
            const auto root = request.context.getRootsResponse("roots");
            const auto form = request.context.getInputResponses().find("form");
            if (request.arguments.value("urlRequired", false)) {
                reply->error(mcp::ErrorCode::UrlElicitationRequired,
                             "Demonstration URL required",
                             {{"elicitations",
                               {{{"mode", "url"},
                                 {"message", "Demonstration"},
                                 {"url", "https://example.com/"},
                                 {"elicitationId", "required-url-id"}}}}});
            } else if (sample && root && form != request.context.getInputResponses().end()) {
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
        using mcp::client::Mcp::setClientCapabilities;

        Peer(bool legacy, bool enabled = true, bool tools = true, bool context = true, bool hostTasks = false, bool removeSampling = false)
            : Mcp([legacy] {
                mcp::client::Options options;
                options.negotiation = legacy ? mcp::client::Negotiation::Legacy : mcp::client::Negotiation::Modern;
                options.elicitationTimeout = utils::Timeval(.01);
                return options;
            }()) {
            if (enabled) {
                setSamplingHandler(
                    [this](const std::string&, const mcp::SamplingRequest&, const std::shared_ptr<mcp::client::ClientReply>& reply) {
                        reply->progress({1, 2, "client work"});
                        reply->progress({1, 2, "duplicate ignored"});
                        reply->progress({0, 2, "decrease ignored"});
                        reply->progress({2, 2, "client complete"});
                        if (retain) {
                            replies.push_back(reply);
                            reply->onCancel([this, weak = std::weak_ptr<mcp::client::ClientReply>(reply)] {
                                if (const auto current = weak.lock()) {
                                    cancelledStatus = cancelledStatus || current->taskStatus(mcp::TaskState::Working, "too late");
                                }
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
                setClientTaskSupport(hostTasks, hostTasks);
                if (removeSampling) {
                    setSamplingHandler({});
                }
                retainForms = hostTasks;
                setTaskObserver([this](const Json& task) {
                    if (task.value("statusMessage", "") == "Waiting for client inputs") {
                        taskMetadata = true;
                    }
                });
                setElicitationCompleteHandler([this](const std::string& id) {
                    completions.push_back(id);
                });
            }
        }

        ~Peer() final;
        bool retain = false;
        bool retainForms = false;
        bool refuse = false;
        bool throwing = false;
        bool forget = false;
        int cancelled = 0;
        bool cancelledStatus = false;
        bool taskMetadata = false;
        int errors = 0;
        std::function<void()> updated;
        Json rootsValue = roots;
        std::string taskId;
        mcp::TaskState status = mcp::TaskState::Working;
        std::vector<Json> results;
        std::vector<std::shared_ptr<mcp::client::ClientReply>> replies;
        std::vector<std::shared_ptr<mcp::client::ElicitationReply>> forms;
        std::vector<std::string> completions;

    private:
        void onElicitation(const std::string&,
                           const std::variant<mcp::FormInput, mcp::UrlInput>&,
                           const std::shared_ptr<mcp::client::ElicitationReply>& reply) final {
            if (retainForms) {
                forms.push_back(reply);
            } else {
                reply->result({mcp::ElicitationAction::Decline});
            }
        }

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
            } else if (value["method"] == "notifications/progress") {
                channel->progress(value.at("params"));
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
        const auto& description = context.peer->getServerDescription();
        const Json info = legacy ? description.at("serverInfo") : description.at("_meta").at("io.modelcontextprotocol/serverInfo");
        test.expectTrue(info.at("name") == "inputs" && description.at("capabilities") == context.peer->getServerCapabilities(),
                        "client retains the complete negotiated identity without duplicating capability state");
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
        test.expectTrue(tool->clientProgress == (legacy ? std::optional<double>(2) : std::nullopt),
                        "reverse sampling progress uses its request token and is not emitted by modern continuations");
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
            auto* declared = new Peer(true);
            declared->setClientCapabilities({{"sampling", Json::object()}, {"tasks", {{"requests", {{"sampling", Json::object()}}}}}});
            Context methodMissing(declared, server, true);
            methodMissing.onMessage(
                Json{{"jsonrpc", "2.0"}, {"id", "method-not-advertised"}, {"method", "sampling/createMessage"}, {"params", params}}.dump());
            test.expectTrue(methodMissing.output.back()["result"] == sampled,
                            "a task category without its method does not advertise augmentation support");
            for (const std::string method : {"tasks/list", "tasks/cancel"}) {
                methodMissing.onMessage(Json{{"jsonrpc", "2.0"}, {"id", method}, {"method", method}, {"params", Json::object()}}.dump());
                test.expectTrue(methodMissing.output.back()["error"]["code"] == -32601,
                                "unadvertised client task controls remain unavailable");
            }
        }
        if (legacy) {
            Context disabledSampling(new Peer(true, true, true, true, true, true), server, true);
            const auto& advertised = disabledSampling.output.front()["params"]["capabilities"];
            test.expectTrue(!advertised.contains("sampling") && !advertised["tasks"]["requests"].contains("sampling") &&
                                advertised["tasks"]["requests"].contains("elicitation"),
                            "removing a sampling handler retires its task capability without disabling elicitation tasks");
            Context ordinaryRoots(new Peer(true, true, true, true, true), server, true);
            ordinaryRoots.onMessage(
                Json{{"jsonrpc", "2.0"}, {"id", "roots-not-taskable"}, {"method", "roots/list"}, {"params", {{"task", {{"ttl", 1000}}}}}}
                    .dump());
            test.expectTrue(ordinaryRoots.output.back()["result"] == ordinaryRoots.peer->rootsValue,
                            "sampling and elicitation task support cannot accidentally task-augment roots");
            Context hosted(new Peer(true, true, true, true, true), server, true);
            hosted.peer->retain = true;
            const auto send = [&hosted](const std::string& id, const std::string& method, Json params) {
                hosted.onMessage(Json{{"jsonrpc", "2.0"}, {"id", id}, {"method", method}, {"params", std::move(params)}}.dump());
            };
            Json params = sampling().toJson();
            params["task"] = {{"ttl", 5000}};
            params["_meta"]["io.modelcontextprotocol/related-task"] = {{"taskId", "outer-tool-task"}};
            send("hosted-sample", "sampling/createMessage", params);
            const Json ticket = hosted.output.back();
            test.expectTrue(ticket["result"].contains("task") && hosted.peer->replies.size() == 1,
                            "advertised client sampling task returns a ticket before its retained reply completes");
            const std::string taskId = ticket["result"]["task"]["taskId"];
            test.expectTrue(ticket["result"]["_meta"]["io.modelcontextprotocol/related-task"]["taskId"] == taskId &&
                                hosted.peer->replies.back()->getTaskId() == taskId,
                            "nested client task ticket retains its own task association");
            Json expectedSample = sampled;
            expectedSample["_meta"]["io.modelcontextprotocol/related-task"] = {{"taskId", taskId}};
            hosted.onMessage(
                Json{{"jsonrpc", "2.0"}, {"method", "notifications/cancelled"}, {"params", {{"requestId", "hosted-sample"}}}}.dump());
            test.expectTrue(hosted.peer->cancelled == 0,
                            "cancelling an answered augmentation request cannot cancel its separately owned task");
            send("hosted-get", "tasks/get", {{"taskId", taskId}});
            test.expectTrue(hosted.output.back()["result"]["status"] == "working", "client-owned task exposes live status");
            test.expectTrue(hosted.peer->replies.back()->taskStatus(mcp::TaskState::InputRequired, "Awaiting local consent"),
                            "application owns client task consent status");
            send("hosted-list", "tasks/list", Json::object());
            test.expectTrue(hosted.output.back()["result"]["tasks"].size() == 1 &&
                                hosted.output.back()["result"]["tasks"][0]["statusMessage"] == "Awaiting local consent",
                            "client task list contains the same task and application status");
            for (unsigned index = 0; index < 128; ++index) {
                const std::string id = "abandoned-result-" + std::to_string(index);
                send(id, "tasks/result", {{"taskId", taskId}});
                hosted.onMessage(Json{{"jsonrpc", "2.0"}, {"method", "notifications/cancelled"}, {"params", {{"requestId", id}}}}.dump());
            }
            send("hosted-result", "tasks/result", {{"taskId", taskId}});
            test.expectTrue(hosted.output.back()["id"] != "hosted-result", "task result waits for actual completion");
            hosted.peer->replies.back()->result(mcp::SamplingResult(sampled));
            test.expectTrue(hosted.output.back()["id"] == "hosted-result" && hosted.output.back()["result"] == expectedSample,
                            "cancelled result requests release capacity; the live result owner delivers sampled task metadata");
            send("completed-get", "tasks/get", {{"taskId", taskId}});
            test.expectTrue(hosted.output.back()["result"]["status"] == "completed", "sampling reply completes its client task");
            send("terminal-cancel", "tasks/cancel", {{"taskId", taskId}});
            test.expectTrue(hosted.output.back()["error"]["code"] == -32602, "legacy terminal task cancellation is rejected");
            send("hosted-form",
                 "elicitation/create",
                 {{"mode", "form"},
                  {"message", "Consent"},
                  {"requestedSchema", {{"type", "object"}, {"properties", Json::object()}}},
                  {"task", {{"ttl", 1000}}}});
            const std::string formId = hosted.output.back()["result"]["task"]["taskId"];
            test.expectTrue(hosted.peer->forms.back()->getTaskId() == formId,
                            "elicitation handlers expose the client task identifier through their existing reply owner");
            int cancelledForms = 0;
            hosted.peer->forms.back()->onCancel([&cancelledForms] {
                ++cancelledForms;
            });
            send("form-cancel", "tasks/cancel", {{"taskId", formId}});
            test.expectTrue(hosted.output.back()["result"]["status"] == "cancelled" && cancelledForms == 1 &&
                                hosted.peer->forms.back()->result({mcp::ElicitationAction::Accept, Json::object()}) ==
                                    mcp::Completion::Inactive,
                            "client task cancellation invalidates elicitation handles and cancels application work once");
            Context stranger(new Peer(true, true, true, true, true), server, true);
            stranger.onMessage(
                Json{{"jsonrpc", "2.0"}, {"id", "foreign-task"}, {"method", "tasks/get"}, {"params", {{"taskId", taskId}}}}.dump());
            test.expectTrue(stranger.output.back()["error"]["code"] == -32602, "client tasks cannot cross initialized sessions");
            send("hosted-url",
                 "elicitation/create",
                 {{"mode", "url"},
                  {"message", "Demonstration"},
                  {"url", "https://example.com/"},
                  {"elicitationId", "opaque-url-id"},
                  {"task", {{"ttl", 1000}}}});
            const std::string urlId = hosted.output.back()["result"]["task"]["taskId"];
            const auto url = hosted.peer->forms.back();
            test.expectTrue(url->getElicitationId() == "opaque-url-id" &&
                                url->taskStatus(mcp::TaskState::InputRequired, "Awaiting acknowledgement"),
                            "URL tasks expose their opaque elicitation identifier and consent status");
            url->result({mcp::ElicitationAction::Decline});
            send("url-result", "tasks/result", {{"taskId", urlId}});
            test.expectTrue(hosted.output.back()["result"]["action"] == "decline", "URL task results retain their elicitation disposition");
            hosted.onMessage(
                Json{{"jsonrpc", "2.0"}, {"method", "notifications/elicitation/complete"}, {"params", {{"elicitationId", "opaque-url-id"}}}}
                    .dump());
            test.expectTrue(hosted.peer->completions.empty(), "declined URL task input cannot receive a completion notification");
            send("accepted-url",
                 "elicitation/create",
                 {{"mode", "url"},
                  {"message", "Demonstration acknowledgement"},
                  {"url", "https://example.com/"},
                  {"elicitationId", "accepted-url-id"},
                  {"task", {{"ttl", 1000}}}});
            hosted.peer->forms.back()->result({mcp::ElicitationAction::Accept});
            hosted.onMessage(Json{
                {"jsonrpc", "2.0"}, {"method", "notifications/elicitation/complete"}, {"params", {{"elicitationId", "accepted-url-id"}}}}
                                 .dump());
            test.expectTrue(hosted.peer->completions == std::vector<std::string>{"accepted-url-id"},
                            "optional URL completion preserves the identifier received by the application");
            for (const std::string id : {"opaque-url-id", "accepted-url-id", "unknown-url-id"}) {
                hosted.onMessage(
                    Json{{"jsonrpc", "2.0"}, {"method", "notifications/elicitation/complete"}, {"params", {{"elicitationId", id}}}}.dump());
            }
            test.expectTrue(hosted.peer->completions == std::vector<std::string>{"accepted-url-id"},
                            "unknown and already completed URL identifiers cannot invoke application callbacks");
            hosted.peer->callTool("inputs", {{"urlRequired", true}});
            for (const std::string id : {"required-url-id", "required-url-id"}) {
                hosted.onMessage(
                    Json{{"jsonrpc", "2.0"}, {"method", "notifications/elicitation/complete"}, {"params", {{"elicitationId", id}}}}.dump());
            }
            test.expectTrue(hosted.peer->completions == std::vector<std::string>{"accepted-url-id", "required-url-id"},
                            "URL-required error descriptions establish completion identifiers exactly once");
            send("missing-url-id", "elicitation/create", {{"mode", "url"}, {"message", "Demonstration"}, {"url", "https://example.com/"}});
            test.expectTrue(hosted.output.back()["error"]["code"] == -32602, "legacy URL request requires its specified identifier");
            params["task"] = {{"ttl", 0}};
            const auto handled = hosted.peer->replies.size();
            send("invalid-task", "sampling/createMessage", params);
            test.expectTrue(hosted.output.back()["error"]["code"] == -32602 && hosted.peer->replies.size() == handled,
                            "invalid negotiated task metadata is rejected before application work");
        }
        if (legacy) {
            Context bounded(new Peer(true), server, true);
            bounded.peer->retainForms = true;
            const auto url = [](Context& target, const std::string& id) {
                target.onMessage(Json{
                    {"jsonrpc", "2.0"},
                    {"id", "url-" + id},
                    {"method", "elicitation/create"},
                    {"params", {{"mode", "url"}, {"message", "Demonstration"}, {"url", "https://example.com/"}, {"elicitationId", id}}}}
                                     .dump());
            };
            bool accepted = true;
            for (unsigned index = 0; index < 128; ++index) {
                url(bounded, std::to_string(index));
                bounded.peer->forms.back()->result({mcp::ElicitationAction::Accept});
                accepted = accepted && bounded.output.back()["result"]["action"] == "accept";
            }
            url(bounded, "overflow");
            test.expectTrue(accepted && bounded.output.back()["error"]["code"] == -32602,
                            "uncompleted URL correlation is bounded without retaining input replies");
            bounded.onMessage(
                Json{{"jsonrpc", "2.0"}, {"method", "notifications/elicitation/complete"}, {"params", {{"elicitationId", "0"}}}}.dump());
            url(bounded, "after-completion");
            bounded.peer->forms.back()->result({mcp::ElicitationAction::Accept});
            test.expectTrue(bounded.output.back()["result"]["action"] == "accept",
                            "consuming a URL completion releases correlation capacity");
            Context declined(new Peer(true), server, true);
            Context cancelled(new Peer(true), server, true);
            cancelled.peer->retainForms = true;
            bool released = true;
            for (unsigned index = 0; index < 129; ++index) {
                url(declined, std::to_string(index));
                url(cancelled, std::to_string(index));
                cancelled.peer->forms.back()->result({mcp::ElicitationAction::Cancel});
                released = released && declined.output.back().value("result", Json::object()).value("action", "") == "decline" &&
                           cancelled.output.back().value("result", Json::object()).value("action", "") == "cancel";
            }
            test.expectTrue(released, "declined and cancelled URL input releases capacity without optional completion notifications");
        }
        if (legacy) {
            Context closing(new Peer(true, true, true, true, true), server, true);
            closing.peer->retain = true;
            Json params = sampling().toJson();
            params["task"] = {{"ttl", 1000}};
            closing.onMessage(
                Json{{"jsonrpc", "2.0"}, {"id", "close-on-error"}, {"method", "sampling/createMessage"}, {"params", params}}.dump());
            const auto reply = closing.peer->replies.back();
            reply->onCancel([&closing] {
                closing.peer->disconnect();
            });
            mcp::Completion completed = mcp::Completion::Inactive;
            try {
                completed = reply->error(mcp::ErrorCode::InternalError, "Application stopped task");
            } catch (const std::exception&) {
            }
            test.expectTrue(completed == mcp::Completion::Accepted &&
                                reply->result(mcp::SamplingResult(sampled)) == mcp::Completion::Inactive,
                            "task error observers can disconnect without changing completion ownership or throwing");
        }
        if (legacy) {
            Context reentrant(new Peer(true), server, true);
            reentrant.peer->retain = true;
            const auto origin = reentrant.peer->callTool("inputs");
            tool->progressObserver = [&reentrant, origin] {
                reentrant.peer->cancel(origin);
            };
            reentrant.peer->replies.back()->progress({3, 3, "cancel during observation"});
            tool->progressObserver = {};
            test.expectTrue(reentrant.peer->cancelled == 2 &&
                                reentrant.peer->replies.back()->result(mcp::SamplingResult(sampled)) == mcp::Completion::Inactive,
                            "progress observers can retire the parent request without invalidating router iteration");
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
    }
    for (const bool legacy : {false, true}) {
        timeoutServers.push_back(std::make_unique<mcp::server::Server>("timeout", "1"));
        timeoutServers.back()->tool(std::make_shared<InputsTool>());
        timeouts.push_back(std::make_unique<Context>(new Peer(legacy), *timeoutServers.back(), legacy));
        timeouts.back()->peer->retain = true;
        tasks.push_back(std::make_unique<Context>(new Peer(legacy), *timeoutServers.back(), legacy));
    }
    Context expiry(new Peer(true, true, true, true, true), *timeoutServers[0], true);
    expiry.peer->retain = true;
    Json expiring = sampling().toJson();
    expiring["task"] = {{"ttl", 10}};
    expiry.onMessage(Json{{"jsonrpc", "2.0"}, {"id", "expiring"}, {"method", "sampling/createMessage"}, {"params", expiring}}.dump());
    const std::string expiringId = expiry.output.back()["result"]["task"]["taskId"];
    expiry.onMessage(
        Json{{"jsonrpc", "2.0"}, {"id", "expiring-result"}, {"method", "tasks/result"}, {"params", {{"taskId", expiringId}}}}.dump());
    Context completedExpiry(new Peer(true, true, true, true, true), *timeoutServers[0], true);
    Context cancelledExpiry(new Peer(true, true, true, true, true), *timeoutServers[0], true);
    cancelledExpiry.peer->retain = true;
    for (unsigned index = 0; index < 16; ++index) {
        const std::string id = "retained-" + std::to_string(index);
        for (Context* target : {&completedExpiry, &cancelledExpiry}) {
            const auto before = target->output.size();
            target->onMessage(Json{{"jsonrpc", "2.0"}, {"id", id}, {"method", "sampling/createMessage"}, {"params", expiring}}.dump());
            const std::string task = target->output.at(before)["result"]["task"]["taskId"];
            if (target == &cancelledExpiry) {
                target->onMessage(
                    Json{{"jsonrpc", "2.0"}, {"id", "cancel-" + id}, {"method", "tasks/cancel"}, {"params", {{"taskId", task}}}}.dump());
            }
        }
    }
    bool retentionElapsed = false;
    Context refused(new Peer(false), *timeoutServers[0], false);
    refused.peer->refuse = true;
    const auto completed = [&] {
        bool done = refused.peer->errors == 1 && expiry.peer->cancelled == 1 && retentionElapsed;
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
    expiry.peer->updated = completed;
    core::timer::Timer::singleshotTimer(
        [&] {
            retentionElapsed = true;
            completed();
        },
        utils::Timeval(0.03));
    core::timer::Timer::singleshotTimer(
        [] {
            core::SNodeC::stop();
        },
        utils::Timeval(2));
    // Start timed input/task work only after synchronous setup is complete.
    for (const auto& context : timeouts) {
        context->peer->callTool("inputs");
    }
    mcp::client::RequestOptions taskOptions;
    taskOptions.task = true;
    for (const auto& context : tasks) {
        context->peer->callTool("inputs", {{"task", true}}, taskOptions);
    }
    refused.peer->callTool("inputs", {{"task", true}});
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
        test.expectTrue(context->peer->results.size() == 1 && context->peer->taskMetadata &&
                            context->peer->results[0]["structuredContent"] ==
                                Json{{"sample", expectedSample}, {"roots", expectedRoots}, {"state", "owned-state"}},
                        "task input/update/result preserves all payloads and related-task metadata in both eras");
    }
    refused.peer->getTask(refused.peer->taskId);
    test.expectTrue(refused.peer->errors == 1 && refused.peer->status == mcp::TaskState::Cancelled,
                    "refused modern task input cancels the server task, retires polling and reports one error");
    test.expectTrue(expiry.peer->cancelled == 1 && !expiry.peer->cancelledStatus && expiry.output.back()["id"] == "expiring-result" &&
                        expiry.output.back()["error"]["code"] == -32602 &&
                        expiry.peer->replies.back()->result(mcp::SamplingResult(sampled)) == mcp::Completion::Inactive,
                    "client task TTL retires pending results, cancels work and rejects late completion");
    for (Context* target : {&completedExpiry, &cancelledExpiry}) {
        const auto before = target->output.size();
        Json parameters = sampling().toJson();
        parameters["task"] = {{"ttl", 1000}};
        target->onMessage(
            Json{{"jsonrpc", "2.0"}, {"id", "after-retention"}, {"method", "sampling/createMessage"}, {"params", parameters}}.dump());
        test.expectTrue(target->output.at(before).contains("result") && target->output.at(before)["result"].contains("task"),
                        "completed and cancelled client tasks retain their expiry owner and release session capacity");
    }
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
