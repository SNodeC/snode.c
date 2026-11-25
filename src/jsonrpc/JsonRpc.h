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

#ifndef JSONRPC_JSONRPC_H
#define JSONRPC_JSONRPC_H

#include "core/timer/Timer.h"
#include "jsonrpc/messages/Id.h"
#include "log/SemanticLogger.h"

namespace jsonrpc {
    class JsonRpcContext;

    namespace messages {
        struct Request;
        struct Notification;
        struct Result;
        struct Error;
    } // namespace messages
} // namespace jsonrpc

#ifndef DOXYGEN_SHOULD_SKIP_THIS
#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace jsonrpc {

    /// A symmetric JSON-RPC peer. The context owns the engine; adapters own framing.
    class JsonRpc {
    public:
        explicit JsonRpc(const std::string& connectionName = {});
        JsonRpc(const JsonRpc&) = delete;
        JsonRpc(JsonRpc&&) = delete;
        JsonRpc& operator=(const JsonRpc&) = delete;
        JsonRpc& operator=(JsonRpc&&) = delete;
        virtual ~JsonRpc();

        virtual void onConnected();
        virtual void onDisconnected();
        virtual bool onSignal(int sig);

        messages::Id sendRequest(const std::string& method,
                                 std::optional<nlohmann::json> params = {},
                                 const utils::Timeval& timeout = utils::Timeval(0));
        void sendNotification(const std::string& method, std::optional<nlohmann::json> params = {});
        void sendResult(const messages::Id& id, const nlohmann::json& result);
        void sendError(const messages::Id& id, std::int64_t code, const std::string& message, std::optional<nlohmann::json> data = {});

        const std::string& getConnectionName() const;
        const JsonRpcContext* getJsonRpcContext() const;
        std::size_t getMaximumMessageSize() const;
        static bool isRequest(const nlohmann::json& value);
        static bool isResponse(const nlohmann::json& value);

    protected:
        virtual void onRequest(const messages::Request& request);
        virtual void onNotification(const messages::Notification& notification);
        virtual void onResult(const messages::Result& result);
        virtual void onError(const messages::Error& error);
        virtual void onRequestTimeout(const messages::Id& id);

        void setBatchesEnabled(bool enabled);
        void setMaximumMessageSize(std::size_t size);
        void setMaximumNestingDepth(std::size_t depth);
        messages::Id getRequestIdentifier();
        /// Derived protocols may supply their own id namespace (e.g. MCP elicitation).
        bool sendRequest(const messages::Request& request, const utils::Timeval& timeout = utils::Timeval(0));
        bool cancelRequest(const messages::Id& id);
        void discardRequest(const messages::Id& id);
        const logger::BoundaryLogger& log() const;

    private:
        struct Batch {
            nlohmann::json responses = nlohmann::json::array();
            std::size_t unanswered = 0;
            bool dispatching = true;
        };

        void setJsonRpcContext(JsonRpcContext* context);
        void onMessage(const std::string& message);
        void dispatch(const nlohmann::json& value, const std::shared_ptr<Batch>& batch);
        void _onRequest(const messages::Request& request, const std::shared_ptr<Batch>& batch);
        void _onNotification(const messages::Notification& notification);
        void _onResult(const messages::Result& result);
        void _onError(const messages::Error& error);
        void answer(const messages::Id& id, const nlohmann::json& response);
        void emit(const nlohmann::json& response, const std::shared_ptr<Batch>& batch);
        void finishBatch(const std::shared_ptr<Batch>& batch);
        void send(const nlohmann::json& value);

        std::string connectionName;
        JsonRpcContext* context = nullptr;
        std::map<messages::Id, std::shared_ptr<Batch>> incoming;
        std::map<messages::Id, core::timer::Timer> outgoing;
        std::int64_t requestIdentifier = 0;
        std::size_t maximumMessageSize = 1024 * 1024;
        std::size_t maximumNestingDepth = 128;
        bool batchesEnabled = true;
        bool connected = false;
        mutable std::optional<logger::BoundaryLogger> log_;
        mutable unsigned long logGeneration_ = 0;

        friend class JsonRpcContext;
    };

} // namespace jsonrpc
#endif // JSONRPC_JSONRPC_H
