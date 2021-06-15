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

#include "SocketContext.h"

#include "log/Logger.h"
#include "web/ws/client/SubProtocol.h"
#include "web/ws/client/SubProtocolSelector.h"
#include "web/ws/ws_utils.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#define CLOSE_SOCKET_TIMEOUT 10

namespace web::ws::client {

    SocketContext::SocketContext(net::socket::stream::SocketConnectionBase* socketConnection, web::ws::client::SubProtocol* subProtocol)
        : web::ws::SocketContext(socketConnection, subProtocol, Transmitter::Role::CLIENT) {
    }

    SocketContext::~SocketContext() {
        web::ws::client::SubProtocolSelector::instance()->destroy(subProtocol);
    }

} // namespace web::ws::client
