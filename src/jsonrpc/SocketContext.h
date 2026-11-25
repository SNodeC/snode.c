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

#ifndef JSONRPC_SOCKETCONTEXT_H
#define JSONRPC_SOCKETCONTEXT_H

#include "core/socket/stream/SocketContext.h"
#include "jsonrpc/JsonRpcContext.h"

namespace core::socket::stream {
    class SocketConnection;
}

namespace jsonrpc {
    class JsonRpc;
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace jsonrpc {

    class SocketContext
        : public core::socket::stream::SocketContext
        , private JsonRpcContext {
    private:
        using Super = core::socket::stream::SocketContext;

    public:
        enum class Framing { Newline, ContentLength };

        SocketContext(core::socket::stream::SocketConnection* connection,
                      JsonRpc* jsonRpc,
                      Framing framing = Framing::Newline,
                      std::size_t maximumHeaderSize = 8192);
        ~SocketContext() override;

        core::socket::stream::SocketConnection* getSocketConnection() const final;
        void close() final;

    protected:
        void sendMessage(const std::string& message) override;
        void end() override;

    private:
        void onReadError(int error) final;
        void onWriteError(int error) final;
        void onShutdown() final;
        void onConnected() final;
        std::size_t onReceivedFromPeer() final;
        void onDisconnected() final;
        bool onSignal(int sig) final;
        void consume(std::string_view bytes);
        void parseHeader();
        void framingError();

        Framing framing;
        std::size_t maximumHeaderSize;
        std::string buffer;
        std::optional<std::size_t> contentLength;
        bool failed = false;
    };

} // namespace jsonrpc
#endif // JSONRPC_SOCKETCONTEXT_H
