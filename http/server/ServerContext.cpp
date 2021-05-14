/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020 Volker Christian <me@vchrist.at>
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

#include "ServerContext.h"

#include "http/server/HTTPServerContext.h"
#include "net/socket/stream/SocketConnectionBase.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <iostream>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace http::server {

    void ServerContext::upgrade(ServerContext* newServerContext) {
        socketConnection->template setContext<ServerContext*>(newServerContext);

        newServerContext->setSocketConnection(socketConnection);

        markedForDelete = true;
    }

    void ServerContext::take(const char* junk, std::size_t junkLen) {
        receiveData(junk, junkLen);

        if (markedForDelete) {
            delete this;
        }
    }

} // namespace http::server
