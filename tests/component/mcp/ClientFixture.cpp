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
#include "jsonrpc/messages/Error.h"
#include "mcp/client/Mcp.h"
#include "mcp/client/Stdio.h"
#include "mcp/client/http/Client.h"
#include "utils/Config.h"
#ifndef DOXYGEN_SHOULD_SKIP_THIS
#include <cstdlib>
#include <iostream>
#include <utility>
#endif
namespace {
    using mcp::Json;

    struct FlowState {
        bool legacy = false;
        bool complete = false;
        unsigned tools = 0;
        unsigned resources = 0;
        unsigned prompts = 0;
        unsigned updated = 0;
        unsigned logs = 0;
        unsigned tasks = 0;
        unsigned statuses = 0;
        unsigned cancelled = 0;
        unsigned expired = 0;
        bool cancelling = false;
        bool awaitingExpiry = false;
        std::string cancelledTask;
        bool acknowledged = false;
        bool prompt = false;
        bool mutating = false;
    };

    class Protocol final : public mcp::client::Mcp {
    public:
        explicit Protocol(FlowState& state)
            : state(state) {
            setNegotiation(state.legacy ? mcp::client::Negotiation::Legacy : mcp::client::Negotiation::Modern);
        }

        ~Protocol() final {
            check.cancel();
        }

        void configure() {
            setNegotiation(state.legacy ? mcp::client::Negotiation::Legacy : mcp::client::Negotiation::Modern);
        }

    private:
        void mutate() {
            if (state.prompt && (state.legacy || state.acknowledged) && !state.mutating) {
                state.mutating = true;
                callTool("mutate");
            }
        }

        void onReady(mcp::ProtocolEra, const Json&) final {
            if (state.legacy) {
                subscribe("test://updates");
                setLoggingLevel(mcp::LogLevel::Debug);
            } else {
                listen({{"toolsListChanged", true},
                        {"resourcesListChanged", true},
                        {"promptsListChanged", true},
                        {"resourceSubscriptions", {"test://updates"}}});
            }
            listTools();
        }

        void onTools(const jsonrpc::messages::Id&, const mcp::ToolsListResult& result) final {
            if (result.tools.size() < 4) {
                core::SNodeC::stop();
            } else {
                callTool("headers", {{"nested", {{"region", "Hello, 世界"}}}});
            }
        }

        void onToolResult(const jsonrpc::messages::Id&, const mcp::ToolResult& result) final {
            const auto text = result.toJson().at("content")[0].at("text").get<std::string>();
            if (text == "Hello, 世界") {
                mcp::client::RequestOptions options;
                options.task = state.legacy;
                options.ttlMs = 5000;
                callTool("background", Json::object(), options);
            } else if (text == "background complete") {
                readResource("test://updates");
            } else if (text == "mutated") {
                state.cancelling = true;
                mcp::client::RequestOptions options;
                options.task = state.legacy;
                options.ttlMs = 5000;
                callTool("background", {{"waitForCancel", true}}, options);
            }
        }

        void onResourceResult(const jsonrpc::messages::Id&, const mcp::ResourceResult&) final {
            getPrompt("ask");
        }

        void onPromptResult(const jsonrpc::messages::Id&, const mcp::PromptResult&) final {
            state.prompt = true;
            mutate();
        }

        void onSubscriptionAcknowledged(const jsonrpc::messages::Id&, const Json&) final {
            state.acknowledged = true;
            mutate();
        }

        void onToolsListChanged(const std::optional<jsonrpc::messages::Id>&) final {
            ++state.tools;
        }

        void onResourcesListChanged(const std::optional<jsonrpc::messages::Id>&) final {
            ++state.resources;
        }

        void onPromptsListChanged(const std::optional<jsonrpc::messages::Id>&) final {
            ++state.prompts;
        }

        void onResourceUpdated(const std::string&, const std::optional<jsonrpc::messages::Id>&) final {
            ++state.updated;
        }

        void onLogMessage(const mcp::LogMessage&) final {
            ++state.logs;
        }

        void onTaskCreated(const jsonrpc::messages::Id&, const mcp::TaskInfo& task) final {
            ++state.tasks;
            if (state.cancelling) {
                state.cancelledTask = task.taskId;
                cancelTask(task.taskId);
            }
        }

        void onCancelled(const jsonrpc::messages::Id&, const std::string&) final {
            ++state.cancelled;
            check = core::timer::Timer::singleshotTimer(
                [this]() {
                    const auto timer = std::move(check);
                    state.awaitingExpiry = true;
                    getTask(state.cancelledTask);
                },
                utils::Timeval(5.2));
        }

        void onTaskStatus(const mcp::TaskInfo&) final {
            ++state.statuses;
        }

        void onRemoteError(const jsonrpc::messages::Error& error) final {
            if (state.awaitingExpiry && error.code == -32602) {
                ++state.expired;
                state.complete = state.tools == 1 && state.resources == 1 && state.prompts == 1 && state.updated == 1 && state.tasks == 2 &&
                                 state.statuses > 0 && state.cancelled == 1 && state.expired == 1 && state.logs == (state.legacy ? 1U : 0U);
                disconnect();
            } else {
                std::cerr << error.serialize() << std::endl;
            }
            core::SNodeC::stop();
        }

        void onTimeout(const jsonrpc::messages::Id&, const std::string& method) final {
            std::cerr << "timeout " << method << std::endl;
            core::SNodeC::stop();
        }

        FlowState& state;
        core::timer::Timer check;
    };
} // namespace

int main(int argc, char* argv[]) {
    FlowState state;
    auto& config = utils::Config::configRoot;
    config.addOption("--legacy", "Legacy era", "bool", false, CLI::Validator{});
    config.addOption("--server", "Stdio server executable", "path", std::string{}, CLI::Validator{});
    config.addOption("--port", "HTTP port", "port", unsigned{8080}, CLI::Range(1, 65535));
    mcp::client::Stdio stdio("client-stdio", [&state]() {
        return new Protocol(state);
    });
    auto* protocol = new Protocol(state);
    mcp::client::http::Options options;
    options.bearerToken = []() {
        return std::string("fixture");
    };
    options.onError = [](const std::string& error) {
        std::cerr << error << std::endl;
    };
    mcp::client::http::Http http("client-http", protocol, std::move(options));
    core::SNodeC::init(argc, argv);
    state.legacy = config.getOption("--legacy")->as<bool>();
    protocol->configure();
    const auto executable = config.getOption("--server")->as<std::string>();
    if (!executable.empty()) {
        stdio.start({executable, "--config-file", "/dev/null", "--log-console", "stderr", "mcp", "--page-size", "1"});
    } else {
        http.start(net::in::SocketAddress("127.0.0.1", config.getOption("--port")->as<std::uint16_t>()));
    }
    core::timer::Timer::singleshotTimer(
        []() {
            core::SNodeC::stop();
        },
        utils::Timeval(8));
    core::SNodeC::start();
    std::cout << Json{{"complete", state.complete},
                      {"tools", state.tools},
                      {"resources", state.resources},
                      {"prompts", state.prompts},
                      {"updated", state.updated},
                      {"logs", state.logs},
                      {"tasks", state.tasks},
                      {"statuses", state.statuses},
                      {"cancelled", state.cancelled},
                      {"expired", state.expired}}
                     .dump()
              << std::endl;
    return state.complete ? 0 : 1;
}
