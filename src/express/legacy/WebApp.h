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

#ifndef EXPRESS_LEGACY_WEBAPP_H
#define EXPRESS_LEGACY_WEBAPP_H

#include "express/WebAppT.h"            // IWYU pragma: export
#include "web/http/legacy/in/Server.h"  // IWYU pragma: export
#include "web/http/legacy/in6/Server.h" // IWYU pragma: export
#include "web/http/legacy/rf/Server.h"  // IWYU pragma: export

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace express::legacy {

    namespace in {

        class WebApp : public WebAppT<web::http::legacy::in::Server<express::Request, express::Response>> {
            using WebAppT<web::http::legacy::in::Server<express::Request, express::Response>>::WebAppT;
        };

    } // namespace in

    namespace in6 {

        class WebApp : public WebAppT<web::http::legacy::in6::Server<express::Request, express::Response>> {
            using WebAppT<web::http::legacy::in6::Server<express::Request, express::Response>>::WebAppT;
        };

    } // namespace in6

    namespace rf {

        class WebApp : public WebAppT<web::http::legacy::rf::Server<express::Request, express::Response>> {
            using WebAppT<web::http::legacy::rf::Server<express::Request, express::Response>>::WebAppT;
        };

    } // namespace rf

} // namespace express::legacy

#endif // EXPRESS_LEGACY_WEBAPP_H
