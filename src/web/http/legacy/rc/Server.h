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

#ifndef WEB_HTTP_LEGACY_RC_SERVER_H
#define WEB_HTTP_LEGACY_RC_SERVER_H

#include "net/rc/stream/legacy/SocketServer.h" // IWYU pragma: export
#include "web/http/server/Response.h"          // IWYU pragma: export
#include "web/http/server/Server.h"            // IWYU pragma: export

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::http::legacy::rc {

    template <typename Request, typename Response>
    using ServerBase = web::http::server::Server<net::rc::stream::legacy::SocketServer, Request, Response>;

    using Server = ServerBase<web::http::server::Request, web::http::server::Response>;

} // namespace web::http::legacy::rc

#endif // WEB_HTTP_LEGACY_RC_SERVER_H
