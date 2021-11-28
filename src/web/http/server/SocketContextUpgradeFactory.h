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

#include "core/socket/SocketContext.h"            // IWYU pragma: export
#include "web/http/SocketContextUpgradeFactory.h" // IWYU pragma: export

namespace web::http::server {
    class Request;
    class Response;
} // namespace web::http::server

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <string> // for string

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::http::server {

    class SocketContextUpgradeFactory : public web::http::SocketContextUpgradeFactory<Request, Response> {
    public:
        using Resquest = web::http::server::Request;
        using Reponse = web::http::server::Response;

    protected:
        SocketContextUpgradeFactory();

    public:
        void checkRefCount() final;

    protected:
        static void link(const std::string& upgradeContextName, SocketContextUpgradeFactory* (*linkedPlugin)());
    };

} // namespace web::http::server

#endif // WEB_HTTP_SERVER_SOCKETCONTEXTUPGRADEFACTORY_H
