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

#ifndef WSTRANSCEIVER_H
#define WSTRANSCEIVER_H

#include "http/WSReceiver.h"
#include "http/WSTransmitter.h"
#include "net/socket/stream/SocketProtocol.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstdint>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace http::websocket {

    class WSServerContext
        : public net::socket::stream::SocketProtocol
        , public WSReceiver
        , public WSTransmitter {
    private:
        void receiveData(const char* junk, std::size_t junklen) override;
        void onReadError(int errnum) override;
        void onWriteError(int errnum) override;

        void onMessageStart(int opCode) override;
        void onMessageData(char* junk, uint64_t junkLen) override;
        void onMessageEnd() override;
        void onError(uint16_t errnum) override;

        void onFrameReady(char* frame, uint64_t frameLength) override;

        void close(uint16_t statusCode = 0, const char* reason = nullptr, std::size_t reasonLength = 0);
        void ping(const char* reason = nullptr, std::size_t reasonLength = 0);
        void pong(const char* reason = nullptr, std::size_t reasonLength = 0);

        bool closeReceived = false;
        bool closeSent = false;

        bool pingReceived = false;
        bool pongReceived = false;
    };

} // namespace http::websocket

#endif // WSTRANSCEIVER_H
