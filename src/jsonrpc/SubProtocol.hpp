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

#include "jsonrpc/JsonRpc.h"
#include "jsonrpc/SubProtocol.h"
#include "web/websocket/SubProtocolContext.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS
#include <algorithm>
#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace jsonrpc {

    template <typename WSRole>
    SubProtocol<WSRole>::SubProtocol(web::websocket::SubProtocolContext* context, const std::string& name, JsonRpc* jsonRpc)
        : Super(context, name, 0)
        , JsonRpcContext(jsonRpc) {
    }

    template <typename WSRole>
    void SubProtocol<WSRole>::sendMessage(const std::string& message) {
        Super::sendMessage(message);
    }

    template <typename WSRole>
    void SubProtocol<WSRole>::end() {
        Super::sendClose();
    }

    template <typename WSRole>
    void SubProtocol<WSRole>::close() {
        Super::sendClose(1002);
    }

    template <typename WSRole>
    void SubProtocol<WSRole>::onConnected() {
        JsonRpcContext::onConnected();
    }

    template <typename WSRole>
    void SubProtocol<WSRole>::onDisconnected() {
        JsonRpcContext::onDisconnected();
    }

    template <typename WSRole>
    bool SubProtocol<WSRole>::onSignal(int sig) {
        const bool handled = JsonRpcContext::onSignal(sig);
        end();
        return handled;
    }

    template <typename WSRole>
    void SubProtocol<WSRole>::onMessageStart(int opCode) {
        message.clear();
        rejected = opCode != web::websocket::SubProtocolContext::OpCode::TEXT;
        if (rejected) {
            Super::sendClose(1003);
        }
    }

    template <typename WSRole>
    void SubProtocol<WSRole>::onMessageData(const char* chunk, std::size_t size) {
        if (!rejected) {
            if (size > jsonRpc->getMaximumMessageSize() - std::min(message.size(), jsonRpc->getMaximumMessageSize())) {
                rejected = true;
                message.clear();
                Super::sendClose(1009);
            } else {
                message.append(chunk, size);
            }
        }
    }

    template <typename WSRole>
    void SubProtocol<WSRole>::onMessageEnd() {
        if (!rejected) {
            const std::string complete = std::move(message);
            message.clear();
            JsonRpcContext::onMessage(complete);
        }
    }

    template <typename WSRole>
    void SubProtocol<WSRole>::onMessageError([[maybe_unused]] std::uint16_t error) {
        rejected = true;
        message.clear();
        close();
    }

    template <typename WSRole>
    core::socket::stream::SocketConnection* SubProtocol<WSRole>::getSocketConnection() const {
        return Super::subProtocolContext->getSocketConnection();
    }

} // namespace jsonrpc
