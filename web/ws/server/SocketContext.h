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

#ifndef WEB_WS_SERVER_SOCKTECONTEXT_H
#define WEB_WS_SERVER_SOCKTECONTEXT_H

#include "web/http/server/Request.h"
#include "web/http/server/Response.h"
#include "web/ws/SocketContext.h"
#include "web/ws/server/SubProtocol.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::ws::server {

    class SocketContext : public web::ws::SocketContext {
    protected:
        SocketContext(net::socket::stream::SocketConnectionBase* socketConnection,
                      web::ws::server::SubProtocol* subProtocol,
                      web::ws::SubProtocol::Role role);

        ~SocketContext() override;

    public:
        static SocketContext* create(net::socket::stream::SocketConnectionBase* socketConnection,
                                     web::http::server::Request& req,
                                     web::http::server::Response& res);

        static void destroy(SocketContext*);
    };

} // namespace web::ws::server

#endif // WEB_WS_SERVER_SOCKTECONTEXT_H
