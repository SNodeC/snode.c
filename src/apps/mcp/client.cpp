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

#include "web/http/legacy/in/Client.h"

#include "core/SNodeC.h"
#include "jsonrpc/messages/Error.h"
#include "jsonrpc/messages/Id.h"
#include "mcp/Protocol.h"
#include "mcp/Types.h"
#include "mcp/client/Mcp.h"
#include "mcp/client/Stdio.h"
#include "mcp/client/http/Client.h"
#include "utils/Config.h"
#ifndef DOXYGEN_SHOULD_SKIP_THIS
#include <cstdlib>
#include <exception>
#include <iostream>
#include <nlohmann/json.hpp>
#include <optional>
#include <utility>
#include <variant>
// IWYU pragma: no_include <nlohmann/detail/json_ref.hpp>
#endif
namespace {
    using mcp::Json;

    class Client final : public mcp::client::Mcp {
    public:
        explicit Client(bool& success)
            : success(success) {
        }

        void configure() {
            setNegotiation(utils::Config::configRoot.getOption("--legacy")->as<bool>() ? mcp::client::Negotiation::Legacy
                                                                                       : mcp::client::Negotiation::Auto);
        }

    private:
        void done(const Json& result) {
            std::cout << result.dump() << std::endl;
            success = true;
            if (!utils::Config::configRoot.getOption("--watch")->as<bool>()) {
                disconnect();
                core::SNodeC::stop();
            }
        }

        void onReady(mcp::ProtocolEra era, const Json&) final {
            std::cerr << "MCP ready " << (era == mcp::ProtocolEra::PerRequest20260728 ? mcp::protocolVersion2026 : mcp::protocolVersion2025)
                      << std::endl;
            const std::string command = utils::Config::configRoot.getOption("--command")->as<std::string>();
            const std::string name = utils::Config::configRoot.getOption("--name")->as<std::string>();
            const Json arguments = Json::parse(utils::Config::configRoot.getOption("--arguments")->as<std::string>());
            if (command == "tools") {
                listTools();
            } else if (command == "resources") {
                listResources();
            } else if (command == "templates") {
                listResourceTemplates();
            } else if (command == "prompts") {
                listPrompts();
            } else if (command == "call") {
                mcp::client::RequestOptions options;
                options.task = utils::Config::configRoot.getOption("--task")->as<bool>();
                options.progressToken = "cli";
                callTool(name, arguments, options);
            } else if (command == "read") {
                readResource(name);
            } else if (command == "prompt") {
                getPrompt(name, arguments);
            } else if (command == "listen") {
                listen(arguments);
            } else if (command == "log") {
                setLoggingLevel(mcp::LogLevel::Debug);
            } else {
                request(command, arguments);
            }
        }

        void onTools(const jsonrpc::messages::Id&, const mcp::ToolsListResult& result) final {
            done(Json{{"tools", result.tools}});
        }

        void onResources(const jsonrpc::messages::Id&, const mcp::ResourcesListResult& result) final {
            done(Json{{"resources", result.resources}});
        }

        void onResourceTemplates(const jsonrpc::messages::Id&, const mcp::ResourceTemplatesListResult& result) final {
            done(Json{{"resourceTemplates", result.resourceTemplates}});
        }

        void onPrompts(const jsonrpc::messages::Id&, const mcp::PromptsListResult& result) final {
            done(Json{{"prompts", result.prompts}});
        }

        void onToolResult(const jsonrpc::messages::Id&, const mcp::ToolResult& result) final {
            done(result.toJson());
        }

        void onResourceResult(const jsonrpc::messages::Id&, const mcp::ResourceResult& result) final {
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
            done(Json{{"contents", contents}});
        }

        void onPromptResult(const jsonrpc::messages::Id&, const mcp::PromptResult& result) final {
            done(Json{{"messages", result.messages.size()}});
        }

        void onOtherResult(const jsonrpc::messages::Id&, const std::string& method, const Json& result) final {
            done(Json{{"method", method}, {"result", result}});
        }

        void onRemoteError(const jsonrpc::messages::Error& error) final {
            std::cerr << error.serialize() << std::endl;
            core::SNodeC::stop();
        }

        void onTimeout(const jsonrpc::messages::Id&, const std::string& method) final {
            std::cerr << "Timeout: " << method << std::endl;
            core::SNodeC::stop();
        }

        void onToolsListChanged(const std::optional<jsonrpc::messages::Id>&) final {
            std::cout << "tools/list_changed" << std::endl;
        }

        void onResourcesListChanged(const std::optional<jsonrpc::messages::Id>&) final {
            std::cout << "resources/list_changed" << std::endl;
        }

        void onPromptsListChanged(const std::optional<jsonrpc::messages::Id>&) final {
            std::cout << "prompts/list_changed" << std::endl;
        }

        void onResourceUpdated(const std::string& uri, const std::optional<jsonrpc::messages::Id>&) final {
            std::cout << "resources/updated " << uri << std::endl;
        }

        void onLogMessage(const mcp::LogMessage& message) final {
            std::cout << "log " << mcp::loggingLevelName(message.level) << " " << message.data.dump() << std::endl;
        }

        void onTaskCreated(const jsonrpc::messages::Id&, const mcp::TaskInfo& task) final {
            std::cerr << "task " << task.taskId << std::endl;
        }

        void onTaskStatus(const mcp::TaskInfo& task) final {
            std::cerr << "task status " << task.taskId << " " << static_cast<int>(task.status) << std::endl;
        }

        void onProgress(const mcp::ProgressToken&, const mcp::Progress& progress) final {
            std::cerr << "progress " << progress.value << std::endl;
        }

        bool& success;
    };
} // namespace

int main(int argc, char* argv[]) {
    int result = 1;
    bool success = false;
    auto& config = utils::Config::configRoot;
    config.addOption("--server", "Server executable (stdio)", "path", std::string{}, CLI::Validator{});
    config.addOption("--server-arguments", "Server argv as a JSON array", "json", std::string{"[]"}, CLI::Validator{});
    config.addOption("--host", "HTTP host", "host", std::string{"127.0.0.1"}, CLI::Validator{});
    config.addOption("--port", "HTTP port", "port", unsigned{8080}, CLI::Validator{});
    config.addOption("--path", "HTTP endpoint", "path", std::string{"/mcp"}, CLI::Validator{});
    config.addOption("--command",
                     "tools/resources/templates/prompts/call/read/prompt/listen/log or method",
                     "method",
                     std::string{"tools"},
                     CLI::Validator{});
    config.addOption("--name", "Tool/prompt name or resource URI", "name", std::string{}, CLI::Validator{});
    config.addOption("--arguments", "Arguments as JSON", "json", std::string{"{}"}, CLI::Validator{});
    config.addOption("--legacy", "Use 2025-11-25", "bool", false, CLI::Validator{});
    config.addOption("--task", "Augment legacy call with a task", "bool", false, CLI::Validator{});
    config.addOption("--watch", "Keep receiving notifications", "bool", false, CLI::Validator{});
    mcp::client::Stdio stdio("mcp-client-stdio", [&success]() {
        auto* protocol = new Client(success);
        protocol->configure();
        return protocol;
    });
    auto* protocol = new Client(success);
    mcp::client::http::Options options;
    options.bearerToken = []() {
        const char* token = std::getenv("MCP_BEARER_TOKEN");
        return token != nullptr ? std::string(token) : std::string{};
    };
    options.onError = [](const std::string& error) {
        std::cerr << error << std::endl;
    };
    mcp::client::http::Http http("mcp-client-http", protocol, options);
    try {
        core::SNodeC::init(argc, argv);
        protocol->configure();
        http.setPath(config.getOption("--path")->as<std::string>());
        const std::string executable = config.getOption("--server")->as<std::string>();
        if (!executable.empty()) {
            std::vector<std::string> arguments{executable};
            const auto extra = Json::parse(config.getOption("--server-arguments")->as<std::string>()).get<std::vector<std::string>>();
            arguments.insert(arguments.end(), extra.begin(), extra.end());
            stdio.start(std::move(arguments), [](int status) {
                if (status != 0) {
                    std::cerr << "Child exit: " << status << std::endl;
                }
                core::SNodeC::stop();
            });
        } else {
            http.start(
                net::in::SocketAddress(config.getOption("--host")->as<std::string>(), config.getOption("--port")->as<std::uint16_t>()));
        }
        const int loop = core::SNodeC::start();
        result = success && loop == 0 ? 0 : 1;
    } catch (const std::exception& error) {
        std::cerr << error.what() << std::endl;
        core::SNodeC::free();
    }
    return result;
}
