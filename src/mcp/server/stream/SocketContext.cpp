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

#include "mcp/server/stream/SocketContext.h"

#include "Log.h"
#include "core/socket/stream/SocketConnection.h"
#include "mcp/server/Config.h"
#include "mcp/server/Mcp.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS
#include <algorithm>
#include <nlohmann/json.hpp>
#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace mcp::server::stream {

    SocketContext::SocketContext(core::socket::stream::SocketConnection* connection, Server& server, Config& config)
        : SocketContext(connection,
                        config,
                        new Mcp(detail::Access::registry(server), config.getDeadline(), {}, {}, {}, config.getMaximumMessageBytes())) {
    }

    SocketContext::SocketContext(core::socket::stream::SocketConnection* connection, Config& config, Mcp* protocol)
        : Super(connection, protocol)
        , config(config)
        , protocol(protocol) {
    }

    SocketContext::~SocketContext() = default;

    void SocketContext::sendMessage(const std::string& message) {
        if (!failed) {
            const std::string bytes = protocol->wire(Json::parse(message)).dump();
            const auto* connection = core::socket::stream::SocketContext::getSocketConnection();
            const auto queued = connection->getTotalQueued() - connection->getTotalSent();
            if (bytes.size() + 1 > config.getMaximumOutputBytes() - std::min(queued, config.getMaximumOutputBytes())) {
                snode::log::framework("mcp.server", snode::log::Boundary::Context).error() << "stdio output limit exceeded";
                failed = true;
                protocol->onSignal(0);
                core::socket::stream::SocketContext::close();
            } else {
                Super::sendMessage(bytes);
            }
        }
    }

} // namespace mcp::server::stream
