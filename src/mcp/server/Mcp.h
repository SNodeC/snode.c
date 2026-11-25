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

#ifndef MCP_SERVER_MCP_H
#define MCP_SERVER_MCP_H

#include "jsonrpc/JsonRpc.h"
#include "mcp/Types.h"
#include "mcp/server/Types.h"
#include "mcp/server/detail/Server.h"
#include "utils/Timeval.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef>
#include <memory>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace mcp {
    enum class ProtocolEra;
}

namespace mcp::server {

    /// MCP semantics over a JSON-RPC peer; HTTP owns exchanges, streams own connections.
    class Mcp final : public jsonrpc::JsonRpc {
    private:
        using Super = jsonrpc::JsonRpc;

    public:
        Mcp(const detail::Access::Registry& registry,
            const utils::Timeval& timeout,
            std::shared_ptr<detail::Channel> channel = {},
            std::optional<detail::ProtocolEra> era = {},
            std::optional<Principal> principal = {},
            std::size_t maximumMessageSize = 1024 * 1024,
            std::size_t pageSize = 128);
        ~Mcp() final;

        void onConnected() final;
        void onDisconnected() final;
        bool onSignal(int sig) final;
        std::shared_ptr<detail::Response> receive(const Json& request, detail::Response::Output output);
        void deliver(const std::string& message);
        Json wire(Json value) const;
        static Json wire(Json value, detail::ProtocolEra era);
        bool respond(const Json& message);
        void forget(const Json& id);
        static bool validRequest(const Json& request);
        static Json errorResponse(const Json& id, detail::ProtocolEra era, int code, const std::string& message, Json data);

    private:
        void onRequest(const jsonrpc::messages::Request& request) final;
        void onNotification(const jsonrpc::messages::Notification& notification) final;
        bool validateResult(const jsonrpc::messages::Result& result) const final;
        void onResult(const jsonrpc::messages::Result& result) final;
        void onError(const jsonrpc::messages::Error& error) final;
        void output(const Json& value, bool terminal, const detail::Response::Output& sink, const Json& id);
        void finish();

        struct State;
        std::shared_ptr<State> state;
    };

} // namespace mcp::server
#endif // MCP_SERVER_MCP_H
