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
#include "mcp/Protocol.h"
#include "mcp/client/Mcp.h"
#include "tests/support/TestResult.h"
#ifndef DOXYGEN_SHOULD_SKIP_THIS
#include <memory>
#include <nlohmann/json.hpp>
#include <string>
#include <variant>
#include <vector>
#endif
namespace {
    using mcp::Json;

    mcp::client::Options options(bool modern) {
        mcp::client::Options value;
        value.negotiation = modern ? mcp::client::Negotiation::Modern : mcp::client::Negotiation::Legacy;
        value.elicitationTimeout = utils::Timeval(.02);
        value.pollTasks = false;
        return value;
    }

    class Peer final : public mcp::client::Mcp {
    public:
        explicit Peer(bool modern)
            : Mcp(options(modern)) {
        }

        ~Peer() final;
        bool retain = true;
        bool delayed = false;
        int errors = 0;
        std::vector<std::shared_ptr<mcp::client::ElicitationReply>> replies;

    private:
        void onElicitation([[maybe_unused]] const std::string& key,
                           [[maybe_unused]] const std::variant<mcp::FormInput, mcp::UrlInput>& input,
                           const std::shared_ptr<mcp::client::ElicitationReply>& reply) final {
            if (delayed) {
                core::timer::Timer::singleshotTimer(
                    [reply] {
                        reply->result({mcp::ElicitationAction::Accept, {{"answer", 42}}});
                    },
                    utils::Timeval(.005));
            } else if (retain) {
                replies.push_back(reply);
            }
        }

        void onRemoteError([[maybe_unused]] const jsonrpc::messages::Error& error) final {
            ++errors;
        }
    };

    Peer::~Peer() = default;

    class Context final : public jsonrpc::JsonRpcContext {
    public:
        Context(bool modern, Peer* peer)
            : JsonRpcContext(peer)
            , modern(modern)
            , peer(peer) {
            onConnected();
        }

        ~Context() final;
        bool modern;
        bool deferEnd = false;
        Peer* peer;
        std::vector<Json> output;

        void sendMessage(const std::string& message) final {
            const auto value = Json::parse(message);
            output.push_back(value);
            if (value.value("method", "") == "initialize") {
                result(value["id"], {{"protocolVersion", mcp::protocolVersion2025}, {"capabilities", Json::object()}});
            } else if (value.value("method", "") == "server/discover") {
                result(value["id"],
                       {{"supportedVersions", {mcp::protocolVersion2026}}, {"capabilities", Json::object()}, {"resultType", "complete"}});
            }
        }

        void result(const Json& id, const Json& value) {
            onMessage(Json{{"jsonrpc", "2.0"}, {"id", id}, {"result", value}}.dump());
        }

        Json inputs() const {
            return {{"question",
                     {{"method", "elicitation/create"},
                      {"params",
                       {{"mode", "form"},
                        {"message", "Answer"},
                        {"requestedSchema",
                         {{"type", "object"}, {"properties", {{"answer", {{"type", "integer"}}}}}, {"required", {"answer"}}}}}}}}};
        }

        Json ask() {
            const auto id = peer->callTool("ask");
            if (modern) {
                result(id, {{"resultType", "input_required"}, {"requestState", "opaque"}, {"inputRequests", inputs()}});
            } else {
                onMessage(Json{
                    {"jsonrpc", "2.0"}, {"id", "question"}, {"method", "elicitation/create"}, {"params", inputs()["question"]["params"]}}
                              .dump());
            }
            return id;
        }

        Json answer() const {
            return modern ? output.back()["params"]["inputResponses"]["question"] : output.back()["result"];
        }

        void end() final {
            if (!deferEnd) {
                onDisconnected();
            }
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
    }
} // namespace

int main(int argc, char* argv[]) {
    core::SNodeC::init(argc, argv);
    tests::support::TestResult test;
    std::vector<std::unique_ptr<Context>> timeouts;
    std::vector<std::unique_ptr<Context>> delayed;
    for (const bool modern : {false, true}) {
        Context context(modern, new Peer(modern));
        const auto id = context.ask();
        const auto count = context.output.size();
        auto reply = context.peer->replies.back();
        test.expectTrue(!context.output.back().contains("result"), "retained responder does not answer synchronously");
        test.expectTrue(reply->result({mcp::ElicitationAction::Accept, {{"answer", "wrong"}}}) == mcp::Completion::Inactive,
                        "invalid form data leaves reply unanswered");
        test.expectTrue(context.output.size() == count, "invalid data produces no wire output");
        test.expectTrue(reply->result({mcp::ElicitationAction::Accept, {{"answer", 42}}}) == mcp::Completion::Accepted,
                        "later answer accepted");
        test.expectTrue(context.answer()["content"]["answer"] == 42, "later answer delivered through protocol");
        if (modern) {
            test.expectTrue(context.output.back()["id"] != id && context.output.back()["params"]["requestState"] == "opaque",
                            "MRTR resumes with a fresh id and exact opaque state");
        }
        const auto sent = context.output.size();
        test.expectTrue(reply->result({mcp::ElicitationAction::Decline}) == mcp::Completion::Inactive && context.output.size() == sent,
                        "duplicate response is inactive and sends nothing");
        context.peer->replies.clear();
        context.peer->retain = false;
        context.ask();
        test.expectTrue(context.answer()["action"] == "decline", "unretained responder declines on destruction");
        context.peer->retain = true;
        context.peer->replies.clear();
        if (modern) {
            Json batch = context.inputs();
            batch["second"] = batch["question"];
            const auto batchId = context.peer->callTool("ask");
            context.result(batchId, {{"resultType", "input_required"}, {"inputRequests", batch}, {"requestState", "batch-state"}});
            const auto before = context.output.size();
            context.peer->replies[0]->result({mcp::ElicitationAction::Decline});
            test.expectTrue(context.output.size() == before, "MRTR waits for every asynchronous input");
            context.peer->replies[1]->result({mcp::ElicitationAction::Accept, {{"answer", 42}}});
            test.expectTrue(context.output.back()["params"]["inputResponses"].size() == 2,
                            "MRTR sends collected responses once all handles answer");
            context.peer->replies.clear();
            const Json task{{"taskId", "task-input"},
                            {"status", "working"},
                            {"createdAt", "2026-07-28T00:00:00Z"},
                            {"lastUpdatedAt", "2026-07-28T00:00:00Z"},
                            {"ttlMs", 1000},
                            {"pollIntervalMs", 10}};
            Json created = task;
            created["resultType"] = "task";
            context.result(context.peer->callTool("job"), created);
            Json input = task;
            input["status"] = "input_required";
            input["inputRequests"] = context.inputs();
            input["resultType"] = "complete";
            context.result(context.peer->getTask("task-input"), input);
            test.expectTrue(context.peer->replies.size() == 1, "task input also creates an asynchronous responder");
            context.peer->replies.back()->result({mcp::ElicitationAction::Accept, {{"answer", 42}}});
            test.expectTrue(context.output.back()["method"] == "tasks/update" &&
                                context.output.back()["params"]["taskId"] == "task-input" &&
                                context.output.back()["params"]["inputResponses"]["question"]["content"]["answer"] == 42,
                            "task responder sends tasks/update with accepted content");
            context.peer->replies.clear();
        }
        const auto cancelledId = context.ask();
        auto cancelled = context.peer->replies.back();
        if (modern) {
            context.peer->cancel(cancelledId);
        } else {
            context.onMessage(
                Json{{"jsonrpc", "2.0"}, {"method", "notifications/cancelled"}, {"params", {{"requestId", "question"}}}}.dump());
        }
        const auto afterCancel = context.output.size();
        test.expectTrue(cancelled->result({mcp::ElicitationAction::Accept, {{"answer", 42}}}) == mcp::Completion::Inactive &&
                            context.output.size() == afterCancel,
                        "cancelled input cannot restart an operation or answer a cancelled request");
        context.ask();
        auto disconnected = context.peer->replies.back();
        context.onDisconnected();
        const auto stopped = context.output.size();
        test.expectTrue(disconnected->result({mcp::ElicitationAction::Decline}) == mcp::Completion::Inactive &&
                            context.output.size() == stopped,
                        "disconnect invalidates retained handles without wire output");
        Context closing(modern, new Peer(modern));
        closing.ask();
        closing.deferEnd = true;
        closing.peer->disconnect();
        test.expectTrue(closing.peer->replies.back()->result({mcp::ElicitationAction::Decline}) == mcp::Completion::Inactive,
                        "logical disconnect invalidates input before physical stream teardown");
        auto timeout = std::make_unique<Context>(modern, new Peer(modern));
        timeout->ask();
        timeouts.push_back(std::move(timeout));
        auto later = std::make_unique<Context>(modern, new Peer(modern));
        later->peer->delayed = true;
        later->ask();
        delayed.push_back(std::move(later));
    }
    core::timer::Timer::singleshotTimer(
        [] {
            core::SNodeC::stop();
        },
        utils::Timeval(.05));
    core::SNodeC::start();
    for (const auto& context : timeouts) {
        test.expectTrue(context->answer()["action"] == "decline", "timeout declines retained input in either era");
        test.expectTrue(context->peer->replies.back()->result({mcp::ElicitationAction::Decline}) == mcp::Completion::Inactive,
                        "expired handle cannot answer again");
        test.expectTrue(context->peer->errors == 0, "input timeout is a valid decline");
    }
    for (const auto& context : delayed) {
        test.expectTrue(context->answer()["content"]["answer"] == 42 && context->peer->errors == 0,
                        "event-loop asynchronous answer precedes timeout");
    }
    return test.processResult();
}
