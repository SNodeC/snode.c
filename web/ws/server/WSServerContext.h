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

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef>
#include <cstdint>
#include <functional>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::ws::server {

    class WSServerContext
        : public net::socket::stream::SocketProtocol
        , public web::ws::WSReceiver
        , public web::ws::WSTransmitter {
    public:
        WSServerContext(const std::function<void(WSServerContext* wSServerContext, int opCode)>& _onMessageStart,
                        const std::function<void(WSServerContext* wSServerContext, const char* junk, std::size_t junkLen)>& _onFrameData,
                        const std::function<void(WSServerContext* wSServerContext)>& _onMessageEnd);

    public:
        void messageStart(uint8_t opCode, const char* message, std::size_t messageLength, uint32_t messageKey = 0) override;
        void sendFrame(const char* message, std::size_t messageLength, uint32_t messageKey = 0) override;
        void messageEnd(const char* message, std::size_t messageLength, uint32_t messageKey = 0) override;

        void message(uint8_t opCode, const char* message, std::size_t messageLength, uint32_t messageKey = 0) override;

        void sendPing(const char* reason = nullptr, std::size_t reasonLength = 0);

    private:
        void replyPong(const char* reason = nullptr, std::size_t reasonLength = 0);

        void close(uint16_t statusCode = 1000, const char* reason = nullptr, std::size_t reasonLength = 0);

        /* SocketProtocol */
        void receiveFromPeer(const char* junk, std::size_t junkLen) override;
        void onReadError(int errnum) override;
        void onWriteError(int errnum) override;

        /* WSReceiver */
        void onMessageStart(int opCode) override;
        void onFrameReceived(const char* junk, uint64_t junkLen) override;
        void onMessageEnd() override;
        void onError(uint16_t errnum) override;

        /* WSTransmitter */
        void onFrameReady(char* frame, uint64_t frameLength) override;

        bool closeReceived = false;
        bool closeSent = false;

        bool pingReceived = false;
        bool pongReceived = false;

        std::function<void(WSServerContext* wSServerContext, int opCode)> _onMessageStart = nullptr;
        std::function<void(WSServerContext* wSServerContext, const char* junk, std::size_t junkLen)> _onFrameData = nullptr;
        std::function<void(WSServerContext* wSServerContext)> _onMessageEnd = nullptr;
    };

} // namespace web::ws::server

#endif // WEB_WS_SERVER_WSSERVERCONTEXT_H
