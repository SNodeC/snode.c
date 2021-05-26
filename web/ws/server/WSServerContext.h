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
#include "web/ws/WSContext.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef>
#include <cstdint>
#include <functional>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#define MASKED false // Server
// #define SHALL_MASKED true // Client

namespace web::ws::server {

    template <typename WSProtocolT>
    class WSServerContext : public web::ws::WSContext {
        using WSProtocol = WSProtocolT;

    public:
        WSServerContext();

    private:
        void sendMessageStart(uint8_t opCode, const char* message, std::size_t messageLength) override;
        void sendMessageFrame(const char* message, std::size_t messageLength) override;
        void sendMessageEnd(const char* message, std::size_t messageLength) override;
        void sendMessage(uint8_t opCode, const char* message, std::size_t messageLength) override;
    };

} // namespace web::ws::server

#endif // WEB_WS_SERVER_WSSERVERCONTEXT_H
