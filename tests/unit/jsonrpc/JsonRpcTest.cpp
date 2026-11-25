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

#include "jsonrpc/JsonRpc.h"

#include "core/SNodeC.h"
#include "core/timer/Timer.h"
#include "jsonrpc/JsonRpcContext.h"
#include "jsonrpc/messages/Error.h"
#include "jsonrpc/messages/ErrorCode.h"
#include "jsonrpc/messages/Notification.h"
#include "jsonrpc/messages/Request.h"
#include "jsonrpc/messages/Result.h"
#include "tests/support/TestResult.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS
#include <cstdint>
#include <memory>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <vector>
#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace {

    using nlohmann::json;

    class Peer final : public jsonrpc::JsonRpc {
    public:
        using Super = jsonrpc::JsonRpc;
        using Super::sendRequest;
        using Super::setBatchesEnabled;
        using Super::setMaximumMessageSize;
        using Super::setMaximumNestingDepth;
        ~Peer() final;
        int requests = 0;
        int notifications = 0;
        int results = 0;
        int errors = 0;
        bool reuseResultId = false;
        bool reused = false;
        std::vector<json> timeouts;
        std::vector<json> deferred;

    private:
        void onRequest(const jsonrpc::messages::Request& request) final {
            ++requests;
            const auto params = request.params.value_or(json::array());
            if (request.method == "subtract") {
                const auto a = params.is_array() ? params[0] : params["minuend"];
                const auto b = params.is_array() ? params[1] : params["subtrahend"];
                sendResult(request.id, a.get<int>() - b.get<int>());
            } else if (request.method == "sum") {
                int value = 0;
                for (const auto& number : params) {
                    value += number.get<int>();
                }
                sendResult(request.id, value);
            } else if (request.method == "get_data") {
                sendResult(request.id, json::array({"hello", 5}));
            } else if (request.method == "defer") {
                deferred.push_back(request.id);
            } else {
                Super::onRequest(request);
            }
        }

        void onNotification([[maybe_unused]] const jsonrpc::messages::Notification& notification) final {
            ++notifications;
        }

        bool validateResult(const jsonrpc::messages::Result& result) const final {
            return result.result != "invalid";
        }

        void onResult(const jsonrpc::messages::Result& result) final {
            ++results;
            if (reuseResultId) {
                reused = sendRequest(jsonrpc::messages::Request{result.id, "reused"});
            }
        }

        void onError([[maybe_unused]] const jsonrpc::messages::Error& error) final {
            ++errors;
        }

        void onRequestTimeout(const jsonrpc::messages::Id& id) final {
            timeouts.push_back(id);
            core::SNodeC::stop();
        }
    };

    Peer::~Peer() = default;

    class Context final : public jsonrpc::JsonRpcContext {
    public:
        explicit Context(jsonrpc::JsonRpc* peer)
            : JsonRpcContext(peer) {
        }

        ~Context() final;
        std::vector<json> output;

        void sendMessage(const std::string& message) final {
            output.push_back(json::parse(message));
        }

        void end() final {
        }

        void close() final {
        }

        core::socket::stream::SocketConnection* getSocketConnection() const final {
            return nullptr;
        }
    };

    Context::~Context() = default;

    void examples(tests::support::TestResult& test) {
        auto* peer = new Peer;
        Context context(peer);
        // JSON-RPC specification examples: https://www.jsonrpc.org/specification
        for (const auto& value :
             std::vector<std::string>{R"({"jsonrpc":"2.0","method":"subtract","params":[42,23],"id":1})",
                                      R"({"jsonrpc":"2.0","method":"subtract","params":[23,42],"id":2})",
                                      R"({"jsonrpc":"2.0","method":"subtract","params":{"subtrahend":23,"minuend":42},"id":3})",
                                      R"({"jsonrpc":"2.0","method":"subtract","params":{"minuend":42,"subtrahend":23},"id":4})"}) {
            context.onMessage(value);
        }
        test.expectTrue(context.output.size() == 4 && context.output[0]["result"] == 19 && context.output[1]["result"] == -19 &&
                            context.output[2]["result"] == 19 && context.output[3]["result"] == 19,
                        "all positional and named parameter examples");
        context.output.clear();
        context.onMessage(R"({"jsonrpc":"2.0","method":"update","params":[1,2,3,4,5]})");
        context.onMessage(R"({"jsonrpc":"2.0","method":"foobar"})");
        test.expectTrue(context.output.empty() && peer->notifications == 2, "notification examples have no response");
        context.onMessage(R"({"jsonrpc":"2.0","method":"foobar","id":"1"})");
        test.expectTrue(context.output.back() ==
                            json::parse(R"({"jsonrpc":"2.0","error":{"code":-32601,"message":"Method not found"},"id":"1"})"),
                        "unknown method example");
        for (const auto& value :
             std::vector<std::string>{R"({"jsonrpc":"2.0","method":"foobar,"params":"bar","baz])",
                                      R"([{"jsonrpc":"2.0","method":"sum","params":[1,2,4],"id":"1"},{"jsonrpc":"2.0","method"])"}) {
            context.onMessage(value);
            test.expectTrue(context.output.back()["error"]["code"] == -32700 && context.output.back()["id"].is_null(),
                            "single and batch parse-error examples");
        }
        context.onMessage(R"({"jsonrpc":"2.0","method":1,"params":"bar"})");
        test.expectTrue(context.output.back()["error"]["code"] == -32600 && context.output.back()["id"].is_null(),
                        "invalid request example");
        context.onMessage("[]");
        test.expectTrue(context.output.back().is_object() && context.output.back()["error"]["code"] == -32600, "empty batch example");
        context.onMessage("[1]");
        test.expectTrue(context.output.back().is_array() && context.output.back().size() == 1 &&
                            context.output.back()[0]["error"]["code"] == -32600,
                        "single invalid batch example");
        context.onMessage("[1,2,3]");
        test.expectTrue(context.output.back().size() == 3 && context.output.back()[2]["error"]["code"] == -32600, "invalid batch example");
        context.onMessage(R"([{"jsonrpc":"2.0","method":"sum","params":[1,2,4],"id":"1"},
                             {"jsonrpc":"2.0","method":"notify_hello","params":[7]},
                             {"jsonrpc":"2.0","method":"subtract","params":[42,23],"id":"2"},
                             {"foo":"boo"},
                             {"jsonrpc":"2.0","method":"foo.get","params":{"name":"myself"},"id":"5"},
                             {"jsonrpc":"2.0","method":"get_data","id":"9"}])");
        const auto batch = context.output.back();
        test.expectTrue(batch.size() == 5 && batch[0]["result"] == 7 && batch[1]["result"] == 19 && batch[2]["error"]["code"] == -32600 &&
                            batch[2]["id"].is_null() && batch[3]["error"]["code"] == -32601 &&
                            batch[4]["result"] == json::array({"hello", 5}),
                        "mixed batch example");
        context.output.clear();
        context.onMessage(R"([{"jsonrpc":"2.0","method":"notify_sum","params":[1,2,4]},
                             {"jsonrpc":"2.0","method":"notify_hello","params":[7]}])");
        test.expectTrue(context.output.empty(), "all-notification batch example");
    }

    void messages(tests::support::TestResult& test) {
        const jsonrpc::messages::Request request{"id", "method", std::optional<json>(json::object())};
        const jsonrpc::messages::Notification notification{"method"};
        const jsonrpc::messages::Result result{std::uint64_t(18446744073709551615ULL), nullptr};
        const jsonrpc::messages::Error error{nullptr, -32603, "Internal error", std::optional<json>(nullptr)};
        test.expectTrue(json::parse(request.serialize()) == request.toJson() && request.toJson()["id"] == "id",
                        "request serialization round trip");
        test.expectTrue(json::parse(notification.serialize()) == notification.toJson() && !notification.toJson().contains("id") &&
                            !notification.toJson().contains("params"),
                        "notification omits id and absent params");
        test.expectTrue(json::parse(result.serialize()) == result.toJson() && result.toJson()["result"].is_null(),
                        "result serialization retains null result and full integer id");
        test.expectTrue(json::parse(error.serialize()) == error.toJson() && error.toJson()["error"].contains("data") &&
                            !jsonrpc::messages::Error{nullptr, -32603, "Internal error"}.toJson()["error"].contains("data"),
                        "optional error data retains explicit null");
    }

    void uncorrelatedErrors(tests::support::TestResult& test) {
        auto* peer = new Peer;
        Context context(peer);
        const auto id = peer->sendRequest("pending");
        const auto before = context.output.size();
        context.onMessage(jsonrpc::messages::Error{nullptr, -32700, "Parse error"}.serialize());
        test.expectTrue(peer->errors == 1 && context.output.size() == before, "null-id error reaches the hook without a reply");
        context.onMessage(jsonrpc::messages::Result{id, 1}.serialize());
        test.expectTrue(peer->results == 1, "uncorrelated error preserves outstanding requests");
        context.onMessage(jsonrpc::messages::Error{id, -32603, "Internal error"}.serialize());
        test.expectTrue(peer->errors == 1, "unknown non-null error id remains dropped");
        context.onMessage(jsonrpc::messages::Error{nullptr, -32600, "Invalid Request"}.serialize());
        test.expectTrue(peer->errors == 2, "each uncorrelated error reaches the protocol");
    }

    void invariants(tests::support::TestResult& test) {
        auto* peer = new Peer;
        Context context(peer);
        context.onConnected();
        for (const auto& invalid : std::vector<std::string>{"null",
                                                            "true",
                                                            "42",
                                                            "{}",
                                                            R"({"jsonrpc":"1.0","method":"sum","id":1})",
                                                            R"({"jsonrpc":"2.0","method":"sum","id":false})",
                                                            R"({"jsonrpc":"2.0","method":"sum","id":1.5})",
                                                            R"({"jsonrpc":"2.0","method":"sum","id":{},"params":[]})",
                                                            R"({"jsonrpc":"2.0","method":"sum","id":7,"params":null})"}) {
            context.onMessage(invalid);
            test.expectTrue(context.output.back()["error"]["code"] == -32600, "invalid envelope rejected");
        }
        test.expectTrue(context.output.back()["id"] == 7, "recoverable invalid-request id retained");
        context.output.clear();
        context.onMessage(R"({"jsonrpc":"2.0","method":"sum","params":null})");
        test.expectTrue(context.output.empty(), "invalid notification params are never answered");
        context.onMessage(R"({"jsonrpc":"2.0","id":null,"method":"sum","params":[1,2]})");
        test.expectTrue(context.output.back()["result"] == 3 && context.output.back()["id"].is_null(),
                        "null request id is answered as required");
        context.onMessage(R"({"jsonrpc":"2.0","id":"extension","method":"sum","params":[1,2],"result":0,"error":{}})");
        test.expectTrue(context.output.back()["result"] == 3 && context.output.back()["id"] == "extension",
                        "request validation ignores additional members");
        context.output.clear();
        context.onMessage(R"([{"jsonrpc":"2.0","id":"a","method":"defer"},{"jsonrpc":"2.0","id":"b","method":"defer"},1])");
        context.onMessage(R"([{"jsonrpc":"2.0","id":"c","method":"defer"}])");
        peer->sendResult("c", 3);
        test.expectTrue(context.output.size() == 1 && context.output[0][0]["id"] == "c", "independent asynchronous batch can finish first");
        peer->sendResult("b", 2);
        test.expectTrue(context.output.size() == 1, "batch waits for every asynchronous answer");
        peer->sendError("a", -32602, "Invalid params");
        test.expectTrue(context.output.size() == 2 && context.output[1].size() == 3,
                        "batch combines automatic errors and asynchronous replies");
        context.onMessage(R"({"jsonrpc":"2.0","id":"duplicate","method":"defer"})");
        context.onMessage(R"({"jsonrpc":"2.0","id":"duplicate","method":"defer"})");
        test.expectTrue(context.output.back()["error"]["code"] == -32600, "duplicate unanswered incoming id is rejected");
        peer->sendResult("duplicate", 4);
        const auto before = context.output.size();
        peer->sendResult("a", 9);
        peer->sendError("missing", -32603, "Internal error");
        test.expectTrue(context.output.size() == before, "duplicate and unknown answers have no wire output");
        test.expectTrue(!peer->sendRequest(jsonrpc::messages::Request{nullptr, "invalid"}) && context.output.size() == before,
                        "outgoing null-id request is rejected without wire output");
        test.expectTrue(peer->sendRequest(jsonrpc::messages::Request{1, "reserved"}), "non-null explicit outgoing id is accepted");
        const auto id = peer->sendRequest("sum", json::array({1, 2}));
        test.expectTrue(id == 2, "integer allocation skips outstanding ids");
        context.onMessage(jsonrpc::messages::Result{id, 3}.serialize());
        context.onMessage(jsonrpc::messages::Result{id, 3}.serialize());
        context.onMessage(jsonrpc::messages::Error{1, -32601, "Method not found"}.serialize());
        context.onMessage(jsonrpc::messages::Error{1, -32601, "Method not found"}.serialize());
        test.expectTrue(peer->results == 1 && peer->errors == 1, "outstanding responses correlate and erase before hooks");
        const auto retry = peer->sendRequest("retry");
        const auto accepted = peer->results;
        context.onMessage(jsonrpc::messages::Result{retry, "invalid"}.serialize());
        context.onMessage(jsonrpc::messages::Result{retry, 3}.serialize());
        test.expectTrue(peer->results == accepted + 1, "protocol validation can reject a result without consuming its correlation");
        peer->reuseResultId = true;
        const auto reuse = peer->sendRequest("reuse");
        context.onMessage(jsonrpc::messages::Result{reuse, 3}.serialize());
        test.expectTrue(peer->reused, "response entry is erased before invoking the result hook");
        context.output.clear();
        peer->setBatchesEnabled(false);
        context.onMessage(R"([{"jsonrpc":"2.0","method":"sum","params":[],"id":1}])");
        test.expectTrue(context.output.size() == 1 && context.output[0].is_object() && context.output[0]["error"]["code"] == -32600,
                        "batch disable switch rejects whole batch");
        peer->setMaximumMessageSize(8);
        const auto requests = peer->requests;
        context.onMessage(R"({"jsonrpc":"2.0","id":99,"method":"sum","params":[]})");
        test.expectTrue(peer->requests == requests && context.output.back()["error"]["code"] == -32600 &&
                            context.output.back()["id"].is_null(),
                        "message limit before dispatch");
        peer->setMaximumMessageSize(1024);
        peer->setMaximumNestingDepth(2);
        context.onMessage(R"({"jsonrpc":"2.0","id":99,"method":"sum","params":[[]]})");
        test.expectTrue(peer->requests == requests && context.output.back()["error"]["code"] == -32600, "depth limit before dispatch");
        context.onDisconnected();
    }

} // namespace

int main(int argc, char* argv[]) {
    core::SNodeC::init(argc, argv);
    tests::support::TestResult test;
    {
        Context context(new jsonrpc::JsonRpc);
        context.onMessage(R"({"jsonrpc":"2.0","method":"missing","id":1})");
        context.onMessage(R"({"jsonrpc":"2.0","method":"notification"})");
        test.expectTrue(context.output.size() == 1 && context.output[0]["error"]["code"] == -32601,
                        "base protocol hooks are safe and the class is concrete");
    }
    messages(test);
    examples(test);
    uncorrelatedErrors(test);
    invariants(test);
    {
        auto* peer = new Peer;
        Context context(peer);
        auto* disconnected = new Peer;
        Context disconnectedContext(disconnected);
        disconnected->sendRequest("cancelled-on-disconnect", nullptr, utils::Timeval(0.002));
        disconnectedContext.onDisconnected();
        const auto id = peer->sendRequest("timeout", nullptr, utils::Timeval(0.01));
        core::SNodeC::start(utils::Timeval(1));
        test.expectTrue(disconnected->timeouts.empty(), "disconnect cancels outstanding request timers");
        test.expectTrue(peer->timeouts.size() == 1 && peer->timeouts[0] == id, "request timer erases and invokes timeout hook");
        context.onMessage(jsonrpc::messages::Result{id, 1}.serialize());
        test.expectTrue(peer->results == 0, "late response after timeout is dropped");
        context.onDisconnected();
    }
    core::SNodeC::free();
    return test.processResult();
}
