/*
 * SNode.C - a slim toolkit for network communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025
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
        , public web::websocket::SubProtocolContext {
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

        core::socket::stream::SocketConnection* getSocketConnection() override;

        /* WSReceiver */
        void onMessageStart(int opCode) override;
        void onMessageData(const char* chunk, uint64_t chunkLen) override;
        void onMessageEnd() override;
        void onMessageError(uint16_t errnum) override;

        std::size_t readFrameData(char* chunk, std::size_t chunkLen) override;

        /* Callbacks (API) socketConnection -> WSProtocol */
        void onConnected() override;
        void onDisconnected() override;

        bool onSignal(int sig) override;

        /* Facade to SocketContext used from WSTransmitter */
        void sendFrameData(uint8_t data) const override;
        void sendFrameData(uint16_t data) const override;
        void sendFrameData(uint32_t data) const override;
        void sendFrameData(uint64_t data) const override;
        void sendFrameData(const char* frame, uint64_t frameLength) const override;

        /* SocketContext */
        std::size_t onReceivedFromPeer() override;

    protected:
        SubProtocol* subProtocol = nullptr;

    private:
        bool closeSent = false;

        int receivedOpCode = 0;

        std::string pongCloseData;
    };

} // namespace web::websocket

#endif // WEB_WEBSOCKET_SOCKETCONTEXT_H
