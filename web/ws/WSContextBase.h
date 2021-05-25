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

#ifndef WEB_WS_WSCONTEXTBASE_H
#define WEB_WS_WSCONTEXTBASE_H

#include "net/socket/stream/SocketProtocol.h"
#include "web/ws/WSReceiver.h"
#include "web/ws/WSTransmitter.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef>
#include <cstdint>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::ws {
    class WSProtocol;
}

namespace web::ws {

    class WSContextBase
        : public net::socket::stream::SocketProtocol
        , public web::ws::WSReceiver
        , public web::ws::WSTransmitter {
    public:
        WSContextBase(web::ws::WSProtocol* wSProtocol);

        virtual ~WSContextBase() = default;

        /* To be overridden in subclass to decide if masking or not */
        virtual void sendMessageStart(uint8_t opCode, const char* message, std::size_t messageLength) = 0;
        virtual void sendMessageFrame(const char* message, std::size_t messageLength) = 0;
        virtual void sendMessageEnd(const char* message, std::size_t messageLength) = 0;
        virtual void sendMessage(uint8_t opCode, const char* message, std::size_t messageLength) = 0;
        virtual void sendPing(const char* reason = nullptr, std::size_t reasonLength = 0) = 0;
        virtual void replyPong(const char* reason = nullptr, std::size_t reasonLength = 0) = 0;
        virtual void close(uint16_t statusCode = 1000, const char* reason = nullptr, std::size_t reasonLength = 0) = 0;

        std::string getLocalAddressAsString() const;
        std::string getRemoteAddressAsString() const;

        /* WSReceiver */
        void onMessageStart(int opCode) override;
        void onFrameReceived(const char* junk, uint64_t junkLen) override;
        void onMessageEnd() override;
        void onPongReceived();
        void onMessageError(uint16_t errnum) override;

        /* Callbacks (API) socketConnection -> WSServerContext */
        void onProtocolConnect() override;
        void onProtocolDisconnect() override;

        /* WSTransmitter */
        void sendFrameData(uint8_t data) override;
        void sendFrameData(uint16_t data) override;
        void sendFrameData(uint32_t data) override;
        void sendFrameData(uint64_t data) override;
        void sendFrameData(const char* frame, uint64_t frameLength) override;

        /* SocketProtocol */
        void receiveFromPeer(const char* junk, std::size_t junkLen) override;
        void onReadError(int errnum) override;
        void onWriteError(int errnum) override;

        void setWSServerContext(WSContextBase* wSServerContext);

    protected:
        web::ws::WSProtocol* wSProtocol;

        bool closeReceived = false;
        bool closeSent = false;

        bool pingReceived = false;
        bool pongReceived = false;
    };

} // namespace web::ws

#endif // WEB_WS_WSCONTEXTBASE_H
