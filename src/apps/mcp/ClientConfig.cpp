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

#include "ClientConfig.h"

#include <map>

#ifndef DOXYGEN_SHOULD_SKIP_THIS
#include <cstdlib>
#include <nlohmann/json.hpp>
#endif

namespace apps::mcp {

    ClientConfig::ClientConfig(utils::SubCommand* parent)
        : SubCommand(parent, this, "Applications") {
        server = addOption("--server", "Stdio server executable", "path", std::string{}, CLI::Validator{});
        serverArguments = addOption("--server-arguments", "Server argv as JSON array", "json", std::string{"[]"}, CLI::Validator{});
        command = addOption("--command",
                            "tools/resources/templates/prompts/call/read/prompt/listen/log/tour or method",
                            "method",
                            std::string{"tools"},
                            CLI::Validator{});
        name = addOption("--name", "Tool/prompt name or resource URI", "name", std::string{}, CLI::Validator{});
        arguments = addOption("--arguments", "Request arguments as JSON object", "json", std::string{"{}"}, CLI::Validator{});
        legacy = addOption("--legacy", "Use 2025-11-25", "bool", false, CLI::Validator{});
        task = addOption("--task", "Augment legacy calls with a task", "bool", false, CLI::Validator{});
        watch = addOption("--watch", "Keep receiving notifications", "bool", false, CLI::Validator{});
        transport = addOption("--transport",
                              "HTTP or HTTPS (a server executable selects stdio)",
                              "transport",
                              std::string{"http"},
                              CLI::IsMember({"http", "https"}));
        path = addOption("--path", "MCP HTTP endpoint", "path", std::string{"/mcp"}, CLI::Validator{});
        answer = addOption("--answer", "Canned non-sensitive tour elicitation label", "label", std::string{"showcase"}, CLI::Validator{});
        action = addOption("--answer-action",
                           "Canned tour elicitation action",
                           "action",
                           std::string{"accept"},
                           CLI::IsMember({"accept", "decline", "cancel"}));
        token = addOption("--bearer-token", "Optional token; MCP_BEARER_TOKEN otherwise", "token", std::string{}, CLI::Validator{});
    }

    ClientConfig::~ClientConfig() = default;

    std::string ClientConfig::getServer() const {
        return server->as<std::string>();
    }

    std::vector<std::string> ClientConfig::getServerArguments() const {
        return ::mcp::Json::parse(serverArguments->as<std::string>()).get<std::vector<std::string>>();
    }

    std::string ClientConfig::getCommand() const {
        return command->as<std::string>();
    }

    std::string ClientConfig::getNameValue() const {
        return name->as<std::string>();
    }

    ::mcp::Json ClientConfig::getArguments() const {
        return ::mcp::Json::parse(arguments->as<std::string>());
    }

    bool ClientConfig::getLegacy() const {
        return legacy->as<bool>();
    }

    bool ClientConfig::getTask() const {
        return task->as<bool>();
    }

    bool ClientConfig::getWatch() const {
        return watch->as<bool>();
    }

    std::string ClientConfig::getTransport() const {
        return transport->as<std::string>();
    }

    std::string ClientConfig::getPath() const {
        return path->as<std::string>();
    }

    std::string ClientConfig::getAnswer() const {
        return answer->as<std::string>();
    }

    ::mcp::ElicitationAction ClientConfig::getAnswerAction() const {
        const std::string value = action->as<std::string>();
        return value == "accept"    ? ::mcp::ElicitationAction::Accept
               : value == "decline" ? ::mcp::ElicitationAction::Decline
                                    : ::mcp::ElicitationAction::Cancel;
    }

    std::string ClientConfig::getBearerToken() const {
        const char* environment = std::getenv("MCP_BEARER_TOKEN");
        const std::string value = token->as<std::string>();
        return !value.empty() ? value : environment != nullptr ? std::string(environment) : std::string{};
    }

} // namespace apps::mcp
