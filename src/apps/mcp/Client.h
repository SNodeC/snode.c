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

#ifndef APPS_MCP_CLIENT_H
#define APPS_MCP_CLIENT_H

#include "core/timer/Timer.h"
#include "jsonrpc/messages/Id.h"
#include "mcp/Types.h"
#include "mcp/client/Mcp.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef>
#include <functional>
#include <string>
#include <vector>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace apps::mcp {

    class ClientConfig;

    /// Scripted application using the ordinary public client hooks.
    class Client final : public ::mcp::client::Mcp {
    public:
        Client(bool& success, const ClientConfig& config);
        ~Client() override;
        void onConnected() override;

    private:
        enum class Outcome { Result, Error, Timeout, Cancelled };

        struct Step {
            std::string feature;
            std::string request;
            std::function<void()> run;
            std::function<bool(const ::mcp::Json&)> check;
            Outcome outcome;
        };

        void plan();
        void command();
        bool legacy() const;
        void add(std::string feature,
                 std::string request,
                 std::function<void()> run,
                 std::function<bool(const ::mcp::Json&)> check = {},
                 Outcome outcome = Outcome::Result);
        void execute();
        void finish(::mcp::Json result, Outcome outcome = Outcome::Result);
        void onReady(::mcp::ProtocolEra era, const ::mcp::Json& capabilities) override;
        void onTools(const jsonrpc::messages::Id&, const ::mcp::ToolsListResult&) override;
        void onResources(const jsonrpc::messages::Id&, const ::mcp::ResourcesListResult&) override;
        void onResourceTemplates(const jsonrpc::messages::Id&, const ::mcp::ResourceTemplatesListResult&) override;
        void onPrompts(const jsonrpc::messages::Id&, const ::mcp::PromptsListResult&) override;
        void onToolResult(const jsonrpc::messages::Id&, const ::mcp::ToolResult&) override;
        void onResourceResult(const jsonrpc::messages::Id&, const ::mcp::ResourceResult&) override;
        void onPromptResult(const jsonrpc::messages::Id&, const ::mcp::PromptResult&) override;
        void onCompletion(const jsonrpc::messages::Id&, const ::mcp::CompletionResult&) override;
        void onOtherResult(const jsonrpc::messages::Id&, const std::string&, const ::mcp::Json&) override;
        void onRemoteError(const jsonrpc::messages::Error&) override;
        void onTimeout(const jsonrpc::messages::Id&, const std::string&) override;
        void onCancelled(const jsonrpc::messages::Id&, const std::string&) override;
        void onElicitation(const std::string&,
                           const std::variant<::mcp::FormInput, ::mcp::UrlInput>&,
                           const std::shared_ptr<::mcp::client::ElicitationReply>&) override;
        void onProgress(const ::mcp::ProgressToken&, const ::mcp::Progress&) override;
        void onToolsListChanged(const std::optional<jsonrpc::messages::Id>&) override;
        void onResourcesListChanged(const std::optional<jsonrpc::messages::Id>&) override;
        void onPromptsListChanged(const std::optional<jsonrpc::messages::Id>&) override;
        void onResourceUpdated(const std::string&, const std::optional<jsonrpc::messages::Id>&) override;
        void onSubscriptionAcknowledged(const jsonrpc::messages::Id&, const ::mcp::Json&) override;
        void onLogMessage(const ::mcp::LogMessage&) override;
        void onTaskCreated(const jsonrpc::messages::Id&, const ::mcp::TaskInfo&) override;
        void onTaskStatus(const ::mcp::TaskInfo&) override;
        void onTasks(const jsonrpc::messages::Id&, const std::vector<::mcp::TaskInfo>&, const std::optional<std::string>&) override;

        bool& success;
        const ClientConfig& config;
        std::vector<Step> steps;
        std::size_t current = 0;
        bool pending = false;
        unsigned progress = 0;
        unsigned inputs = 0;
        unsigned toolChanges = 0;
        unsigned resourceChanges = 0;
        unsigned promptChanges = 0;
        unsigned updates = 0;
        unsigned logs = 0;
        unsigned tasks = 0;
        unsigned statuses = 0;
        unsigned taskAcknowledgements = 0;
        std::string lastTask;
        core::timer::Timer actionTimer;
    };

} // namespace apps::mcp

#endif // APPS_MCP_CLIENT_H
