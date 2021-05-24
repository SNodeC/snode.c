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

#ifndef WEB_WS_SERVER_WSSERVERCONTEXT_H
#define WEB_WS_SERVER_WSSERVERCONTEXT_H

#include "net/socket/stream/SocketProtocol.h"
#include "web/ws/WSReceiver.h"
#include "web/ws/WSTransmitter.h"
#include "web/ws/server/WSServerContextBase.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef>
#include <cstdint>
#include <functional>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::ws::server {

    template <typename WSServerProtocolT>
    class WSServerContext
        : public net::socket::stream::SocketProtocol
        , public web::ws::server::WSServerContextBase
        , public web::ws::WSReceiver
        , public web::ws::WSTransmitter {
        using WSServerProtocol = WSServerProtocolT;

    public:
        WSServerContext();

        ~WSServerContext();

    public:
        void messageStart(uint8_t opCode, char* message, std::size_t messageLength, uint32_t messageKey = 0) override;
        void sendFrame(char* message, std::size_t messageLength, uint32_t messageKey = 0) override;
        void messageEnd(char* message, std::size_t messageLength, uint32_t messageKey = 0) override;
        void message(uint8_t opCode, char* message, std::size_t messageLength, uint32_t messageKey = 0) override;
        void sendPing(char* reason = nullptr, std::size_t reasonLength = 0) override;
        void replyPong(char* reason = nullptr, std::size_t reasonLength = 0);
        void close(uint16_t statusCode = 1000, const char* reason = nullptr, std::size_t reasonLength = 0) override;

        virtual std::string getLocalAddressAsString() const override;
        virtual std::string getRemoteAddressAsString() const override;

        WSServerProtocol* getWSServerProtocol();

    private:
        /* SocketProtocol */
        void receiveFromPeer(const char* junk, std::size_t junkLen) override;
        void onReadError(int errnum) override;
        void onWriteError(int errnum) override;

        /* WSReceiver */
        void onMessageStart(int opCode) override;
        void onFrameReceived(const char* junk, uint64_t junkLen) override;
        void onMessageEnd() override;
        void onMessageError(uint16_t errnum) override;

        /* WSTransmitter */
        void sendFrameData(uint8_t data) override;
        void sendFrameData(uint16_t data) override;
        void sendFrameData(uint32_t data) override;
        void sendFrameData(uint64_t data) override;
        void sendFrameData(char* frame, uint64_t frameLength) override;

        void onFrameReady(char* frame, uint64_t frameLength) override;

        WSServerProtocol* wSServerProtocol;

        bool closeReceived = false;
        bool closeSent = false;

        bool pingReceived = false;
        bool pongReceived = false;
    };

} // namespace web::ws::server

#endif // WEB_WS_SERVER_WSSERVERCONTEXT_H
