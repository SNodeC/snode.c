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
#include "jsonrpc/JsonRpc.h"
#include "jsonrpc/JsonRpcContext.h"
#include "jsonrpc/messages/Error.h"
#include "jsonrpc/messages/Result.h"
#include "mcp/client/Mcp.h"
#include "mcp/server/Server.h"
#include "mcp/server/detail/Server.h"
#include "tests/support/TestResult.h"
#ifndef DOXYGEN_SHOULD_SKIP_THIS
#include <algorithm>
#include <functional>
#include <memory>
#include <nlohmann/json.hpp>
#include <string>
#include <utility>
#include <vector>
#endif
namespace {
    using mcp::Json;
    namespace detail = mcp::server::detail;

    class Peer final : public mcp::client::Mcp {
    public:
        using Super = mcp::client::Mcp;

        Peer(std::function<void()> complete)
            : complete(std::move(complete)) {
        }

        ~Peer() final;
        int ready = 0;
        int errors = 0;
        int timeouts = 0;
        int inputs = 0;
        int resources = 0;
        int prompts = 0;
        int progress = 0;
        int created = 0;
        int statuses = 0;
        int changed = 0;
        int generic = 0;
        bool cancelOnAcknowledged = false;
        Json unsubscribed;
        Json lastId;
        std::vector<Json> tools;
        std::vector<std::string> texts;
        std::function<void()> complete;

    private:
        void onReady([[maybe_unused]] mcp::ProtocolEra era, [[maybe_unused]] const Json& capabilities) final {
            ++ready;
        }

        void onTools(const Json& id, const mcp::ToolsListResult& value) final {
            lastId = id;
            tools = value.tools;
        }

        void onToolResult([[maybe_unused]] const Json& id, const mcp::ToolResult& result) final {
            texts.push_back(result.toJson().at("content")[0]["text"]);
            if (texts.back() == "task complete") {
                complete();
            }
        }

        void onResourceResult([[maybe_unused]] const Json& id, const mcp::ResourceResult& value) final {
            if (!value.contents.empty()) {
                ++resources;
            }
        }

        void onPromptResult([[maybe_unused]] const Json& id, const mcp::PromptResult& value) final {
            if (!value.messages.empty()) {
                ++prompts;
            }
        }

        void onProgress([[maybe_unused]] const mcp::ProgressToken& token, [[maybe_unused]] const mcp::Progress& value) final {
            ++progress;
        }

        void onRemoteError([[maybe_unused]] const jsonrpc::messages::Error& error) final {
            ++errors;
        }

        void onTimeout([[maybe_unused]] const Json& id, [[maybe_unused]] const std::string& method) final {
            ++timeouts;
            complete();
        }

        void onOtherResult([[maybe_unused]] const Json& id,
                           [[maybe_unused]] const std::string& method,
                           [[maybe_unused]] const Json& result) final {
            ++generic;
        }

        void onTaskCreated([[maybe_unused]] const Json& id, [[maybe_unused]] const mcp::TaskInfo& task) final {
            ++created;
        }

        void onTaskStatus([[maybe_unused]] const mcp::TaskInfo& task) final {
            ++statuses;
        }

        void onSubscriptionAcknowledged([[maybe_unused]] const Json& id, [[maybe_unused]] const Json& notifications) final {
            if (cancelOnAcknowledged) {
                unsubscribed = unsubscribe("test://data");
            }
        }

        void onToolsListChanged([[maybe_unused]] const std::optional<Json>& subscription) final {
            ++changed;
        }
    };

    Peer::~Peer() = default;

    class Context final : public jsonrpc::JsonRpcContext {
    public:
        Context(Peer* peer, mcp::server::Server& server, bool legacy)
            : JsonRpcContext(peer)
            , server(server)
            , legacy(legacy)
            , channel(std::make_shared<detail::Channel>()) {
            channel->output = [this](const Json& value) {
                if (!value.is_null()) {
                    onMessage(value.dump());
                }
            };
        }

        ~Context() final;
        mcp::server::Server& server;
        bool legacy;
        std::shared_ptr<detail::Channel> channel;
        std::vector<Json> output;
        std::vector<std::shared_ptr<detail::Response>> pending;
        std::function<void(const Json&)> before;

        void sendMessage(const std::string& text) final {
            const auto value = Json::parse(text);
            output.push_back(value);
            if (before) {
                before(value);
            }
            if (jsonrpc::JsonRpc::isResponse(value)) {
                channel->respond(value);
            } else if (value.contains("id") && value.value("method", "") != "never") {
                auto reply = detail::Access::registry(server).dispatch(
                    value,
                    legacy ? mcp::ProtocolEra::Initialization20251125 : mcp::ProtocolEra::PerRequest20260728,
                    [this](const Json& message, bool) {
                        if (!message.is_null()) {
                            onMessage(message.dump());
                        }
                    },
                    utils::Timeval(30),
                    {},
                    channel,
                    1);
                if (!reply->done) {
                    pending.push_back(reply);
                }
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

    Context::~Context() = default;
} // namespace

int main(int argc, char* argv[]) {
    core::SNodeC::init(argc, argv);
    tests::support::TestResult test;
    test.expectTrue(mcp::client::RequestOptions{}.timeout == utils::Timeval(30), "default request timeout is exactly thirty seconds");
    {
        mcp::server::Server server("client-test", "1");
        const Json schema{{"type", "object"}};
        for (const std::string name : {"c", "b", "a"}) {
            server.tool({name, {}, schema}, [](const auto&, const auto& reply) {
                reply->result(mcp::ToolResult::text("ok"));
            });
        }
        server.tool({"ask", {}, schema}, [](const auto& request, const auto& reply) {
            const Json form{{"type", "object"}, {"properties", Json::object()}};
            if (request.context.getInputResponse("question", form)) {
                reply->result(mcp::ToolResult::text("declined"));
            } else {
                reply->inputRequired({{{"question", mcp::FormInput{"Question", form}}}, "opaque state"});
            }
        });
        server.tool({"progress", {}, schema}, [](const auto&, const auto& reply) {
            reply->progress(mcp::Progress{1, 2, "working"});
            reply->result(mcp::ToolResult::text("progress"));
        });
        mcp::server::ToolDefinition job{"job", {}, schema};
        job.taskSupport = mcp::TaskSupport::Required;
        server.tool(job, [](const auto&, const auto& reply) {
            core::timer::Timer::singleshotTimer(
                [reply] {
                    reply->result(mcp::ToolResult::text("task complete"));
                },
                utils::Timeval(0.003));
        });
        server.resource({"test://data", "data"}, [](const auto& request, const auto& reply) {
            reply->result(mcp::ResourceResult{{mcp::TextResource{request.uri, "text/plain", "data"}}});
        });
        server.prompt({"prompt", {}, {}}, [](const auto&, const auto& reply) {
            reply->result(mcp::PromptResult{{}, {{mcp::Role::User, mcp::TextContent{"prompt", {}}}}});
        });
        int completed = 0;
        auto* modern = new Peer([&] {
            if (++completed == 3) {
                core::SNodeC::stop();
            }
        });
        auto* legacy = new Peer([&] {
            if (++completed == 3) {
                core::SNodeC::stop();
            }
        });
        Context modernContext(modern, server, false);
        Context legacyContext(legacy, server, true);
        modernContext.onConnected();
        legacyContext.onConnected();
        test.expectTrue(modern->ready == 1 && legacy->ready == 1 && modern->getProtocolEra() == mcp::ProtocolEra::PerRequest20260728 &&
                            legacy->getProtocolEra() == mcp::ProtocolEra::Initialization20251125,
                        "modern discovery and legacy fallback negotiate before user hooks");
        modern->cancelOnAcknowledged = true;
        const auto subscribed = modern->subscribe("test://data");
        test.expectTrue(modern->unsubscribed == subscribed && !subscribed.is_null() &&
                            std::any_of(modernContext.output.begin(),
                                        modernContext.output.end(),
                                        [&subscribed](const Json& message) {
                                            return message.value("method", "") == "notifications/cancelled" &&
                                                   message["params"]["requestId"] == subscribed;
                                        }),
                        "acknowledgement hooks can synchronously unsubscribe using the active request authority");
        modern->cancelOnAcknowledged = false;
        const auto listed = modern->listTools();
        test.expectTrue(modern->lastId == listed && modern->tools.size() == 6 && modern->tools[0]["name"] == "a",
                        "pagination aggregates pages under the original request id");
        bool changed = false;
        modernContext.before = [&](const Json& value) {
            if (!changed && value.value("method", "") == "tools/list" && value["params"].contains("cursor")) {
                changed = true;
                server.remove(mcp::server::RegistrationKind::Tool, "b");
            }
        };
        modern->listTools();
        test.expectTrue(changed && modern->tools.size() == 5, "invalidated pagination restarts once without mixing registry revisions");
        modernContext.before = {};
        legacy->listTools();
        test.expectTrue(legacy->tools.size() == 5, "legacy pagination follows nextCursor");
        modern->callTool("ask");
        legacy->callTool("ask");
        test.expectTrue(modern->texts.back() == "declined" && legacy->texts.back() == "declined",
                        "MRTR retry and legacy elicitation use the safe decline default");
        const auto retry = std::find_if(modernContext.output.begin(), modernContext.output.end(), [](const Json& value) {
            return value.contains("params") && value["params"].contains("inputResponses");
        });
        test.expectTrue(retry != modernContext.output.end() && (*retry)["params"]["requestState"] == "opaque state",
                        "MRTR request state echoed without modification");
        modern->readResource("test://data");
        legacy->getPrompt("prompt");
        test.expectTrue(modern->resources == 1 && legacy->prompts == 1, "resource and prompt results decoded into shared owned types");
        mcp::client::RequestOptions options;
        options.progressToken = std::string("progress");
        modern->callTool("progress", Json::object(), options);
        test.expectTrue(modern->progress == 1, "progress matches the outstanding operation token");
        const auto pending = modern->request("never", Json::object(), mcp::client::RequestOptions{utils::Timeval(0)});
        modernContext.onMessage(jsonrpc::messages::Error{nullptr, -32700, "Parse error"}.serialize());
        modernContext.onMessage(jsonrpc::messages::Result{pending, {{"resultType", "complete"}}}.serialize());
        test.expectTrue(modern->errors == 1 && modern->generic == 1,
                        "uncorrelated errors reach the hook without consuming pending requests");
        const auto before = legacyContext.output.size();
        legacyContext.onMessage(R"({"jsonrpc":"2.0","id":"ping","method":"ping"})");
        test.expectTrue(legacyContext.output.size() == before + 1 && legacyContext.output.back()["result"].empty(),
                        "client answers legacy ping intrinsically");
        options.task = true;
        modern->callTool("job", Json::object(), options);
        legacy->callTool("job", Json::object(), options);
        modern->request("never", Json::object(), mcp::client::RequestOptions{utils::Timeval(0.001)});
        auto watchdog = core::timer::Timer::singleshotTimer(
            [] {
                core::SNodeC::stop();
            },
            utils::Timeval(1.5));
        core::SNodeC::start(utils::Timeval(1));
        test.expectTrue(completed == 3 && modern->timeouts == 1 && modern->created == 1 && legacy->created == 1 &&
                            modern->texts.back() == "task complete" && legacy->texts.back() == "task complete",
                        "both task eras poll to the original typed result and timeout hooks clean up request state");
        test.expectTrue(modern->errors == 1 && legacy->errors == 0, "successful protocol flows do not report spurious remote errors");
    }
    core::SNodeC::free();
    return test.processResult();
}
