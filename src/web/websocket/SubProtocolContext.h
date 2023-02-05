/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021, 2022, 2023 Volker Christian <me@vchrist.at>
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

#ifndef WEB_WEBSOCKET_SOCKETCONTEXTUPGRADEBASE_H
#define WEB_WEBSOCKET_SOCKETCONTEXTUPGRADEBASE_H

#include "web/websocket/Receiver.h"
#include "web/websocket/Transmitter.h"

namespace core::socket::stream {
    class SocketConnection;
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef>
#include <cstdint>

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace web::websocket {

    class SubProtocolContext
        : protected Receiver
        , protected Transmitter {
    public:
        SubProtocolContext(bool role);
        SubProtocolContext(const SubProtocolContext&) = delete;

        SubProtocolContext& operator=(const SubProtocolContext&) = delete;

    public:
        virtual void sendMessage(uint8_t opCode, const char* message, std::size_t messageLength) const = 0;
        virtual void sendMessageStart(uint8_t opCode, const char* message, std::size_t messageLength) const = 0;

        virtual void sendMessageFrame(const char* message, std::size_t messageLength) const = 0;
        virtual void sendMessageEnd(const char* message, std::size_t messageLength) const = 0;
        virtual void sendPing(const char* reason = nullptr, std::size_t reasonLength = 0) const = 0;
        virtual void sendPong(const char* reason = nullptr, std::size_t reasonLength = 0) const = 0;
        virtual void sendClose(uint16_t statusCode = 1000, const char* reason = nullptr, std::size_t reasonLength = 0) = 0;

        virtual core::socket::stream::SocketConnection* getSocketConnection() = 0;

        enum OpCode { CONTINUATION = 0x00, TEXT = 0x01, BINARY = 0x02, CLOSE = 0x08, PING = 0x09, PONG = 0x0A };

    private:
        virtual void sendClose(const char* message, std::size_t messageLength) = 0;
    };

} // namespace web::websocket

#endif // WEB_WEBSOCKET_SOCKETCONTEXTUPGRADEBASE_H
