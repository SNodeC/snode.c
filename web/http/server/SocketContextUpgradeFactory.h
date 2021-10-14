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

#ifndef WEB_HTTP_SERVER_SOCKETCONTEXTUPGRADEFACTORY_H
#define WEB_HTTP_SERVER_SOCKETCONTEXTUPGRADEFACTORY_H

#include "net/socket/stream/SocketContextFactory.h"

namespace web::http::server {
    class Request;
    class Response;
} // namespace web::http::server

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::http::server {

    // Should do ref-counting on create and destroy! If ref-count equals 0 no one needs this factory anymore and it should be destroyed and
    // dynamically loaded libraries should be dlclosed. Destroy should be done from an upgraded SocketContext!
    // Thus, SocketContextUpgradeFactory needs the handle to its library to delayed-dlclose it when necessary.
    // A concrete class lives for example in libsnodec-websocket-server and libsnodec-websocket-client

    class SocketContextUpgradeFactory : public net::socket::stream::SocketContextFactory {
    protected:
        SocketContextUpgradeFactory() = default;
        ~SocketContextUpgradeFactory() override = default;

    public:
        enum class Role { CLIENT, SERVER };

        virtual void prepare(Request& request, Response& response);

        virtual std::string name() = 0;
        virtual Role role() = 0;

        virtual void destroy() = 0;

    protected:
        Request* request;
        Response* response;
    };

} // namespace web::http::server

#endif // WEB_HTTP_SERVER_SOCKETCONTEXTUPGRADEFACTORY_H
