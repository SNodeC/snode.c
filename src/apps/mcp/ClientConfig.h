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

#ifndef APPS_MCP_CLIENTCONFIG_H
#define APPS_MCP_CLIENTCONFIG_H

#include "mcp/Types.h"
#include "utils/SubCommand.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS
#include <string>
#include <string_view>
#include <vector>
#endif

namespace apps::mcp {

    class ClientConfig final : public utils::SubCommand {
    public:
        static constexpr std::string_view NAME = "client";
        static constexpr std::string_view DESCRIPTION = "MCP client operations and feature tour";

        explicit ClientConfig(utils::SubCommand* parent);
        ~ClientConfig() override;
        std::string getServer() const;
        std::vector<std::string> getServerArguments() const;
        std::string getCommand() const;
        std::string getNameValue() const;
        ::mcp::Json getArguments() const;
        bool getLegacy() const;
        bool getTask() const;
        bool getWatch() const;
        std::string getTransport() const;
        std::string getPath() const;
        std::string getAnswer() const;
        ::mcp::ElicitationAction getAnswerAction() const;
        std::string getBearerToken() const;

    private:
        CLI::Option* server;
        CLI::Option* serverArguments;
        CLI::Option* command;
        CLI::Option* name;
        CLI::Option* arguments;
        CLI::Option* legacy;
        CLI::Option* task;
        CLI::Option* watch;
        CLI::Option* transport;
        CLI::Option* path;
        CLI::Option* answer;
        CLI::Option* action;
        CLI::Option* token;
    };

} // namespace apps::mcp

#endif // APPS_MCP_CLIENTCONFIG_H
