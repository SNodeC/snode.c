/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021 Volker Christian <me@vchrist.at>
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

#ifndef WEB_WS_SOCKETCONTEXT_H
#define WEB_WS_SOCKETCONTEXT_H

#include "net/socket/stream/SocketContext.h"
#include "web/websocket/Receiver.h"
#include "web/websocket/Transmitter.h"

namespace net::socket::stream {
    class SocketConnection;
} // namespace net::socket::stream

namespace web::websocket {
    class SubProtocol;
} // namespace web::websocket

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef>
#include <cstdint>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::websocket {

    class SocketContext
        : public net::socket::stream::SocketContext
        , protected web::websocket::Receiver
        , protected web::websocket::Transmitter {
    public:
        enum class Role { SERVER, CLIENT };

    protected:
        SocketContext(net::socket::stream::SocketConnection* socketConnection, SubProtocol* subProtocol, Role role);

        SocketContext() = delete;
        SocketContext(const SocketContext&) = delete;
        SocketContext& operator=(const SocketContext&) = delete;

        virtual ~SocketContext() = default;

    public:
        void sendMessage(uint8_t opCode, const char* message, std::size_t messageLength);
        void sendMessageStart(uint8_t opCode, const char* message, std::size_t messageLength);

        void sendMessageFrame(const char* message, std::size_t messageLength);
        void sendMessageEnd(const char* message, std::size_t messageLength);

        void sendPing(const char* reason = nullptr, std::size_t reasonLength = 0);
        void sendPong(const char* reason = nullptr, std::size_t reasonLength = 0);

        void sendClose(uint16_t statusCode = 1000, const char* reason = nullptr, std::size_t reasonLength = 0);

        std::string getLocalAddressAsString() const;
        std::string getRemoteAddressAsString() const;

    protected:
        web::websocket::SubProtocol* subProtocol;

    private:
        void sendClose(const char* reason, std::size_t reasonLength);

        /* WSReceiver */
        void onMessageStart(int opCode) override;
        void onFrameReceived(const char* junk, uint64_t junkLen) override;
        void onMessageEnd() override;
        void onMessageError(uint16_t errnum) override;

        std::size_t readFrameData(char* junk, std::size_t junkLen) override;

        /* Callbacks (API) socketConnection -> WSProtocol */
        void onConnected() override;
        void onDisconnected() override;

        /* Facade to SocketProtocol used from WSTransmitter */
        void sendFrameData(uint8_t data) override;
        void sendFrameData(uint16_t data) override;
        void sendFrameData(uint32_t data) override;
        void sendFrameData(uint64_t data) override;
        void sendFrameData(const char* frame, uint64_t frameLength) override;

        /* SocketProtocol */
        void onReceiveFromPeer() override;
        void onReadError(int errnum) override;
        void onWriteError(int errnum) override;

        bool closeSent = false;

        int opCodeReceived = 0;

        std::string pongCloseData;

        enum OpCode { CLOSE = 0x08, PING = 0x09, PONG = 0x0A };
    };

} // namespace web::websocket

#endif // WEB_WS_SOCKETCONTEXT_H
