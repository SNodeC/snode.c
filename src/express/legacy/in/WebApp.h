/*
 * SNode.C - a slim toolkit for network communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025
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

#ifndef EXPRESS_LEGACY_IN_WEBAPP_H
#define EXPRESS_LEGACY_IN_WEBAPP_H

#include "express/WebAppT.h"           // IWYU pragma: export
#include "web/http/legacy/in/Server.h" // IWYU pragma: export

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace express::legacy::in {

    using WebApp = WebAppT<web::http::legacy::in::ServerBase<web::http::server::Request, web::http::server::Response>>;

} // namespace express::legacy::in

#endif // EXPRESS_LEGACY_IN_WEBAPP_H
