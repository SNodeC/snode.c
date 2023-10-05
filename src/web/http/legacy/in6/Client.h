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

#ifndef WEB_HTTP_LEGACY_IN6_CLIENT_H
#define WEB_HTTP_LEGACY_IN6_CLIENT_H

#include "net/in6/stream/legacy/SocketClient.h" // IWYU pragma: export
#include "web/http/client/Client.h"             // IWYU pragma: export

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::http::legacy::in6 {

    template <typename Request, typename Response>
    using Client = web::http::client::Client<net::in6::stream::legacy::SocketClient, Request, Response>;

} // namespace web::http::legacy::in6

#endif // WEB_HTTP_LEGACY_IN6_CLIENT_H
