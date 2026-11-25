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

#pragma once
#include "jsonrpc/JsonRpc.h"
#include "jsonrpc/messages/Id.h"
#include "mcp/Protocol.h"
#include "mcp/Results.h" // IWYU pragma: export
#include "mcp/Types.h"
#include "utils/Timeval.h"
#ifndef DOXYGEN_SHOULD_SKIP_THIS
#include <cstdint>
#include <memory>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <variant>
#include <vector>
#endif
namespace jsonrpc::messages {
    struct Error;
    struct Notification;
    struct Request;
    struct Result;
} // namespace jsonrpc::messages

namespace mcp::client {
    enum class Negotiation { Auto, Modern, Legacy };

    struct Options {
        Implementation implementation{"snodec-client", "1"};
        Negotiation negotiation = Negotiation::Auto;
        bool followPagination = true;
        bool pollTasks = true;
    };

    struct RequestOptions {
        utils::Timeval timeout{30};
        std::optional<ProgressToken> progressToken{};
        bool task = false;
        std::uint64_t ttlMs = 60000;
    };

    using mcp::PromptsListResult;
    using mcp::ResourcesListResult;
    using mcp::ResourceTemplatesListResult;
    using mcp::TaskInfo;
    using mcp::TaskState;
    using mcp::ToolsListResult;

    /// Symmetric JSON-RPC engine with intrinsic MCP negotiation and typed, safe hooks.
    class Mcp : public jsonrpc::JsonRpc {
    public:
        using Super = jsonrpc::JsonRpc;
        explicit Mcp(Options options = {}, const std::string& connectionName = "mcp.client");
        ~Mcp() override;
        void onConnected() override;
        void onDisconnected() override;
        bool onSignal(int signal) override;
        ProtocolEra getProtocolEra() const;
        const Json& getServerCapabilities() const;
        void disconnect();
        jsonrpc::messages::Id listTools();
        jsonrpc::messages::Id listResources();
        jsonrpc::messages::Id listResourceTemplates();
        jsonrpc::messages::Id listPrompts();
        jsonrpc::messages::Id callTool(const std::string& name, Json arguments = Json::object(), RequestOptions options = {});
        jsonrpc::messages::Id readResource(const std::string& uri, RequestOptions options = {});
        jsonrpc::messages::Id getPrompt(const std::string& name, Json arguments = Json::object(), RequestOptions options = {});
        jsonrpc::messages::Id complete(Json reference, const std::string& name, const std::string& value, Json arguments = Json::object());
        jsonrpc::messages::Id subscribe(const std::string& uri);
        jsonrpc::messages::Id unsubscribe(const std::string& uri);
        jsonrpc::messages::Id listen(Json notifications);
        jsonrpc::messages::Id setLoggingLevel(LogLevel level);
        jsonrpc::messages::Id getTask(const std::string& id);
        jsonrpc::messages::Id getTaskResult(const std::string& id);
        jsonrpc::messages::Id listTasks(const std::optional<std::string>& cursor = {});
        jsonrpc::messages::Id cancelTask(const std::string& id);
        jsonrpc::messages::Id updateTask(const std::string& id, Json inputResponses);
        void cancel(const jsonrpc::messages::Id& id, const std::string& reason = {});
        /// Future extensions use the same metadata and correlation, then onOtherResult.
        jsonrpc::messages::Id request(const std::string& method, Json params = Json::object(), RequestOptions options = {});

    protected:
        void setClientCapabilities(Json capabilities);
        virtual void onReady(ProtocolEra era, const Json& capabilities);
        virtual void onTools(const jsonrpc::messages::Id& id, const ToolsListResult& result);
        virtual void onResources(const jsonrpc::messages::Id& id, const ResourcesListResult& result);
        virtual void onResourceTemplates(const jsonrpc::messages::Id& id, const ResourceTemplatesListResult& result);
        virtual void onPrompts(const jsonrpc::messages::Id& id, const PromptsListResult& result);
        virtual void onToolResult(const jsonrpc::messages::Id& id, const ToolResult& result);
        virtual void onResourceResult(const jsonrpc::messages::Id& id, const ResourceResult& result);
        virtual void onPromptResult(const jsonrpc::messages::Id& id, const PromptResult& result);
        virtual void onCompletion(const jsonrpc::messages::Id& id, const CompletionResult& result);
        virtual void onToolsListChanged(const std::optional<jsonrpc::messages::Id>& subscription);
        virtual void onResourcesListChanged(const std::optional<jsonrpc::messages::Id>& subscription);
        virtual void onPromptsListChanged(const std::optional<jsonrpc::messages::Id>& subscription);
        virtual void onResourceUpdated(const std::string& uri, const std::optional<jsonrpc::messages::Id>& subscription);
        virtual void onSubscriptionAcknowledged(const jsonrpc::messages::Id& id, const Json& notifications);
        virtual void onLogMessage(const LogMessage& message);
        virtual void onTaskCreated(const jsonrpc::messages::Id& id, const TaskInfo& task);
        virtual void onTaskStatus(const TaskInfo& task);
        virtual void onTasks(const jsonrpc::messages::Id& id, const std::vector<TaskInfo>& tasks, const std::optional<std::string>& cursor);
        virtual InputResponse onElicitation(const std::string& key, const std::variant<FormInput, UrlInput>& request);
        virtual void onProgress(const ProgressToken& token, const Progress& progress);
        virtual void onCancelled(const jsonrpc::messages::Id& id, const std::string& reason);
        virtual void onRemoteError(const jsonrpc::messages::Error& error);
        virtual void onTimeout(const jsonrpc::messages::Id& id, const std::string& method);
        virtual void onOtherResult(const jsonrpc::messages::Id& id, const std::string& method, const Json& result);

    private:
        struct Operation;
        struct State;
        std::unique_ptr<State> state;
        void onRequest(const jsonrpc::messages::Request& request) final;
        void onNotification(const jsonrpc::messages::Notification& notification) final;
        void onResult(const jsonrpc::messages::Result& result) final;
        void onError(const jsonrpc::messages::Error& error) final;
        void onRequestTimeout(const jsonrpc::messages::Id& id) final;
        void _onRequest(const jsonrpc::messages::Request& request);
        void _onNotification(const jsonrpc::messages::Notification& notification);
        void _onResult(const jsonrpc::messages::Result& result);
        void _onError(const jsonrpc::messages::Error& error);
        void initialize();
        jsonrpc::messages::Id send(Operation operation);
        void complete(Operation operation, const Json& result);
        Json inputs(const Json& requests);
        void scheduleTask(const std::string& id);
        void taskStatus(const TaskInfo& task);
        TaskInfo parseTask(const Json& value) const;
        void invalid(const jsonrpc::messages::Id& id, const std::string& message);
        using Super::sendError;
        using Super::sendNotification;
        using Super::sendRequest;
        using Super::sendResult;
    };
} // namespace mcp::client
