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

#ifndef WEB_WEBSOCKET_SOCKETCONTEXT_H
#define WEB_WEBSOCKET_SOCKETCONTEXT_H

#include "web/http/SocketContextUpgrade.h" // IWYU pragma: export
#include "web/websocket/SubProtocolContext.h"

namespace core::socket {
    class SocketConnection;
} // namespace core::socket

namespace web::http {
    template <typename RequestT, typename ResponseT>
    class SocketContextUpgradeFactory;
} // namespace web::http

namespace web::websocket {
    template <typename SubProtocolT>
    class SubProtocolFactory;
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#include <cstdint>
#include <endian.h>
#include <iomanip>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#define CLOSE_SOCKET_TIMEOUT 10

namespace web::websocket {

    template <typename SubProtocolT, typename RequestT, typename ResponseT>
    class SocketContextUpgrade
        : public web::http::SocketContextUpgrade<RequestT, ResponseT>
        , public SubProtocolContext {
    public:
        SocketContextUpgrade() = delete;
        SocketContextUpgrade(const SocketContextUpgrade&) = delete;
        SocketContextUpgrade& operator=(const SocketContextUpgrade&) = delete;

    private:
        using Super = web::http::SocketContextUpgrade<RequestT, ResponseT>;

    public:
        using SubProtocol = SubProtocolT;
        using Request = RequestT;
        using Response = ResponseT;

    private:
        using Super::readFromPeer;
        using Super::sendToPeer;
        using Super::setTimeout;
        using Super::shutdownWrite;

    protected:
        enum class Role { SERVER, CLIENT };

        SocketContextUpgrade(core::socket::stream::SocketConnection* socketConnection,
                             web::http::SocketContextUpgradeFactory<Request, Response>* socketContextUpgradeFactory,
                             Role role);

    public:
        ~SocketContextUpgrade() override = default;

    private:
        void sendMessage(uint8_t opCode, const char* message, std::size_t messageLength) override;
        void sendMessageStart(uint8_t opCode, const char* message, std::size_t messageLength) override;
        void sendMessageFrame(const char* message, std::size_t messageLength) override;
        void sendMessageEnd(const char* message, std::size_t messageLength) override;
        void sendPing(const char* reason = nullptr, std::size_t reasonLength = 0) override;
        void sendPong(const char* reason = nullptr, std::size_t reasonLength = 0) override;
        void sendClose(uint16_t statusCode = 1000, const char* reason = nullptr, std::size_t reasonLength = 0) override;
        void sendClose(const char* message, std::size_t messageLength) override;

        void sendFrameChunk(const char* chunk, std::size_t chunkLen) const final;
        std::size_t readFrameChunk(char* chunk, std::size_t chunkLen) const final;

        core::socket::stream::SocketConnection* getSocketConnection() const override;

        /* WSReceiver */
        void onMessageStart(int opCode) override;
        void onMessageData(const char* chunk, uint64_t chunkLen) override;
        void onMessageEnd() override;
        void onMessageError(uint16_t errnum) override;

        /* Callbacks (API) socketConnection -> WSProtocol */
        void onConnected() override;
        void onDisconnected() override;

        bool onSignal(int sig) override;

        /* SocketContext */
        std::size_t onReceivedFromPeer() override;

        std::size_t getPayloadTotalSent() const override;
        std::size_t getPayloadTotalRead() const override;

        std::string getOnlineSince() const override;
        std::string getOnlineDuration() const override;

    protected:
        SubProtocol* subProtocol = nullptr;

    private:
        int receivedOpCode = 0;

        std::string pongCloseData;
    };

} // namespace web::websocket

#endif // WEB_WEBSOCKET_SOCKETCONTEXT_H
