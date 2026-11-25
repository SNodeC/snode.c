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

#include "jsonrpc/SocketContext.h"

#include "jsonrpc/JsonRpc.h"
#include "jsonrpc/messages/Error.h"
#include "jsonrpc/messages/ErrorCode.h"

namespace core::socket::stream {
    class SocketConnection;
}

#include "log/SemanticLogger.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <algorithm>
#include <cctype>
#include <charconv>
#include <nlohmann/json.hpp>
#include <system_error>
#include <utility>
#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace jsonrpc {

    SocketContext::SocketContext(core::socket::stream::SocketConnection* connection,
                                 JsonRpc* jsonRpc,
                                 Framing framing,
                                 std::size_t maximumHeaderSize)
        : Super(connection)
        , JsonRpcContext(jsonRpc)
        , framing(framing)
        , maximumHeaderSize(maximumHeaderSize) {
    }

    SocketContext::~SocketContext() = default;

    void SocketContext::onConnected() {
        JsonRpcContext::onConnected();
    }

    void SocketContext::onDisconnected() {
        JsonRpcContext::onDisconnected();
    }

    bool SocketContext::onSignal(int sig) {
        return JsonRpcContext::onSignal(sig);
    }

    core::socket::stream::SocketConnection* SocketContext::getSocketConnection() const {
        return Super::getSocketConnection();
    }

    void SocketContext::sendMessage(const std::string& message) {
        if (!failed) {
            if (framing == Framing::Newline) {
                if (message.find_first_of("\r\n") != std::string::npos) {
                    framingError();
                } else {
                    sendToPeer(message + "\n");
                }
            } else {
                sendToPeer("Content-Length: " + std::to_string(message.size()) + "\r\n\r\n" + message);
            }
        }
    }

    void SocketContext::end() {
        shutdownWrite();
    }

    void SocketContext::close() {
        Super::close();
    }

    void SocketContext::framingError() {
        Super::frameworkLog().error() << "invalid JSON-RPC stream framing";
        failed = true;
        buffer.clear();
        close();
    }

    void SocketContext::parseHeader() {
        bool valid = true;
        std::size_t position = 0;
        while (valid && position + 2 < buffer.size()) {
            const auto newline = buffer.find("\r\n", position);
            const auto colon = buffer.find(':', position);
            valid = newline != std::string::npos && colon != std::string::npos && colon < newline;
            if (valid) {
                std::string name = buffer.substr(position, colon - position);
                std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c) {
                    return static_cast<char>(std::tolower(c));
                });
                if (name == "content-length") {
                    std::string_view text(buffer.data() + colon + 1, newline - colon - 1);
                    while (!text.empty() && (text.front() == ' ' || text.front() == '\t')) {
                        text.remove_prefix(1);
                    }
                    while (!text.empty() && (text.back() == ' ' || text.back() == '\t')) {
                        text.remove_suffix(1);
                    }
                    std::size_t length = 0;
                    const auto parsed = std::from_chars(text.data(), text.data() + text.size(), length);
                    valid = !contentLength && parsed.ec == std::errc() && parsed.ptr == text.data() + text.size() &&
                            length <= jsonRpc->getMaximumMessageSize();
                    if (valid) {
                        contentLength = length;
                    }
                }
                position = newline + 2;
            }
        }
        buffer.clear();
        if (!valid || !contentLength) {
            framingError();
        }
    }

    void SocketContext::consume(std::string_view bytes) {
        while (!failed && !bytes.empty()) {
            if (framing == Framing::Newline) {
                const auto newline = bytes.find('\n');
                const auto count = newline == std::string_view::npos ? bytes.size() : newline;
                if (count > jsonRpc->getMaximumMessageSize() - std::min(buffer.size(), jsonRpc->getMaximumMessageSize())) {
                    framingError();
                } else {
                    buffer.append(bytes.data(), count);
                    bytes.remove_prefix(count);
                    if (newline != std::string_view::npos) {
                        bytes.remove_prefix(1);
                        if (!buffer.empty() && buffer.back() == '\r') {
                            buffer.pop_back();
                        }
                        const std::string message = std::move(buffer);
                        buffer.clear();
                        JsonRpcContext::onMessage(message);
                    }
                }
            } else if (!contentLength) {
                buffer += bytes.front();
                bytes.remove_prefix(1);
                if (buffer.size() > maximumHeaderSize) {
                    framingError();
                } else if (buffer.ends_with("\r\n\r\n")) {
                    parseHeader();
                    if (!failed && contentLength == 0) {
                        contentLength.reset();
                        JsonRpcContext::onMessage("");
                    }
                }
            } else {
                const auto count = std::min(bytes.size(), *contentLength - buffer.size());
                buffer.append(bytes.data(), count);
                bytes.remove_prefix(count);
                if (buffer.size() == *contentLength) {
                    const std::string message = std::move(buffer);
                    buffer.clear();
                    contentLength.reset();
                    JsonRpcContext::onMessage(message);
                }
            }
        }
    }

    std::size_t SocketContext::onReceivedFromPeer() {
        char bytes[16384];
        const auto count = readFromPeer(bytes, sizeof(bytes));
        consume(std::string_view(bytes, count));
        return count;
    }

    void SocketContext::onReadError(int error) {
        if (error == 0) {
            if (!buffer.empty() || contentLength) {
                sendMessage(messages::Error{nullptr, messages::ParseError, "Parse error"}.serialize());
                buffer.clear();
                contentLength.reset();
            }
            JsonRpcContext::onSignal(0);
            end();
        } else {
            JsonRpcContext::onSignal(error);
            close();
        }
    }

    void SocketContext::onWriteError([[maybe_unused]] int error) {
        JsonRpcContext::onSignal(0);
        framingError();
    }

    void SocketContext::onShutdown() {
        JsonRpcContext::onSignal(0);
        end();
    }

} // namespace jsonrpc
