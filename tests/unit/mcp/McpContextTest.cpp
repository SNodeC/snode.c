// SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
#include "core/SNodeC.h"
#include "mcp/server/detail/Server.h"
#include "tests/support/TestResult.h"
#include <optional>
#include <limits>
#include <stdexcept>
#include <vector>

using namespace mcp::server;
int main(int argc, char* argv[]) {
    tests::support::TestResult test;
    core::SNodeC::init(argc, argv);
    Server server("context");
    std::shared_ptr<ToolReply> pending;
    std::optional<RequestContext> context;
    std::vector<CancelReason> cancelled;
    std::vector<Json> messages;
    bool stopOnCancel = false;
    server.tool({{"name", "work"}, {"inputSchema", {{"type", "object"}}}}, [&](const ToolCall& call, const auto& reply) {
        context = call.context;
        pending = reply;
        call.context.onCancel([](CancelReason) {
            throw std::runtime_error("observer failure must not hide the next observer");
        });
        call.context.onCancel([&](CancelReason reason) {
            cancelled.push_back(reason);
            if (stopOnCancel) {
                core::SNodeC::stop();
            }
        });
        reply->progress({1, 2, "half"});
        reply->progress({0, 2, "backwards"});
        reply->progress({-1, {}, "negative"});
        reply->progress({std::numeric_limits<double>::infinity(), {}, "infinite"});
        reply->progress({2, 1, "invalid total"});
        if (call.arguments.value("finish", false)) {
            test.expectTrue(reply->result(ToolResult::text("done")) == Completion::Accepted, "live completion accepted");
        }
    });
    const auto dispatch = [&](Json token, bool finish = false, int timeout = 30000) {
        Json params{{"name", "work"},
                    {"arguments", {{"finish", finish}}},
                    {"_meta", {{detail::versionKey, detail::modernVersion}, {detail::capabilitiesKey, Json::object()}}}};
        if (token != "absent") {
            params["_meta"]["progressToken"] = std::move(token);
        }
        return detail::Access::registry(server).dispatch(
            {{"jsonrpc", "2.0"}, {"id", 1}, {"method", "tools/call"}, {"params", params}},
            false,
            [&](const Json& message, bool) {
                messages.push_back(message);
            },
            std::chrono::milliseconds(timeout));
    };
    for (const Json& token : Json::array({"token", 0, -9, INT64_MAX})) {
        messages.clear();
        auto state = dispatch(token, true);
        test.expectTrue(context->getProgressToken().has_value() && !context->getPrincipal(),
                        "context exposes token and anonymous identity");
        test.expectTrue(messages.size() == 2 && messages.front()["method"] == "notifications/progress" &&
                            messages.front()["params"]["progressToken"] == token &&
                            messages.front()["params"]["progressToken"].type() == token.type(),
                        "progress preserves JSON token type");
        test.expectTrue(messages.back()["result"]["content"][0]["text"] == "done", "completion follows progress");
        state->cancel();
        context->onCancel([&](CancelReason reason) {
            cancelled.push_back(reason);
        });
        test.expectTrue(cancelled.empty(), "successful completion never calls cancellation observers");
        test.expectTrue(pending->result(ToolResult::text("late")) == Completion::Inactive, "late completion rejected");
        pending->progress({2, 2, "late"});
        test.expectTrue(messages.size() == 2, "no late progress or duplicate completion");
    }
    for (const Json& token : Json::array({nullptr, 1.5, true, Json::object(), UINT64_MAX})) {
        messages.clear();
        dispatch(token);
        test.expectTrue(messages.size() == 1 && messages[0]["error"]["code"] == -32602, "non-token values rejected before handler");
    }
    for (CancelReason reason : {CancelReason::Client, CancelReason::Disconnected, CancelReason::Shutdown}) {
        messages.clear();
        auto state = dispatch("absent");
        test.expectTrue(!context->getProgressToken() && messages.empty(), "absent token sends no progress");
        const auto before = cancelled.size();
        state->cancel(reason);
        state->cancel(reason);
        test.expectTrue(cancelled.size() == before + 1 && cancelled.back() == reason, "one cancellation despite repeated teardown");
        context->onCancel([&](CancelReason late) {
            test.expectTrue(late == reason, "late observer receives original reason synchronously");
        });
        test.expectTrue(messages.size() == 1 && messages[0].is_null(), "transport cancellation has no protocol error frame");
        test.expectTrue(pending->error(ErrorCode::InternalError, "late") == Completion::Inactive, "late error rejected");
        std::weak_ptr<detail::Response> weak = state;
        pending.reset();
        context.reset();
        state.reset();
        test.expectTrue(weak.expired(), "timer and observers do not keep request state alive");
    }
    messages.clear();
    stopOnCancel = true;
    auto deadline = dispatch("deadline", false, 10);
    const int loopResult = core::SNodeC::start(utils::Timeval({1, 0}));
    test.expectEqual(0, loopResult, "deadline observer can stop the event loop");
    test.expectTrue(!cancelled.empty() && cancelled.back() == CancelReason::Deadline, "framework timer cancels without polling");
    test.expectTrue(messages.size() == 2 && messages.back()["result"]["isError"] == true, "deadline emits one tool error");
    test.expectTrue(pending->result(ToolResult::text("late")) == Completion::Inactive, "deadline prevents late success");
    return test.processResult();
}
