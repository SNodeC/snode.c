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

#ifndef WEB_WS_SERVER_WSSERVERCONTEXT_HPP
#define WEB_WS_SERVER_WSSERVERCONTEXT_HPP

#include "log/Logger.h"
#include "net/socket/stream/SocketConnectionBase.h"
#include "web/ws/WSProtocol.h"
#include "web/ws/server/WSServerContext.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstring>  // for memcpy
#include <endian.h> // for htobe16
#include <ostream>  // for endl

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#define CLOSE_SOCKET_TIMEOUT 10

namespace web::ws::server {

    template <typename WSProtocol>
    WSServerContext<WSProtocol>::WSServerContext()
        : WSContextBase(new WSProtocol()) {
        WSContextBase::setWSServerContext(this);
    }

    template <typename WSProtocol>
    void WSServerContext<WSProtocol>::sendMessageStart(uint8_t opCode, const char* message, std::size_t messageLength) {
        if (!closeSent) {
            WSTransmitter::sendMessageStart(opCode, message, messageLength, MASKED);
        }
    }

    template <typename WSProtocol>
    void WSServerContext<WSProtocol>::sendMessageFrame(const char* message, std::size_t messageLength) {
        if (!closeSent) {
            WSTransmitter::sendMessageFrame(message, messageLength, MASKED);
        }
    }

    template <typename WSProtocol>
    void WSServerContext<WSProtocol>::sendMessageEnd(const char* message, std::size_t messageLength) {
        if (!closeSent) {
            WSTransmitter::sendMessageEnd(message, messageLength, MASKED);
        }
    }

    template <typename WSProtocol>
    void WSServerContext<WSProtocol>::sendMessage(uint8_t opCode, const char* message, std::size_t messageLength) {
        if (!closeSent) {
            WSTransmitter::sendMessage(opCode, message, messageLength, MASKED);
        }
    }

} // namespace web::ws::server

#endif // WEB_WS_SERVER_WSSERVERCONTEXT_HPP
