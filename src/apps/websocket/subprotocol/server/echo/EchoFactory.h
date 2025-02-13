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

#ifndef APPS_WEBSOCKET_SUBPROTOCOL_ECHO_SERVER_ECHOINTERFACE_H
#define APPS_WEBSOCKET_SUBPROTOCOL_ECHO_SERVER_ECHOINTERFACE_H

#include "web/websocket/SubProtocolFactory.h"

namespace apps::websocket::subprotocol::echo::server {
    class Echo; // IWYU pragma: keep
}

// IWYU pragma: no_include "apps/websocket/subprotocol/server/echo/Echo.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace apps::websocket::subprotocol::echo::server {

    class EchoFactory : public web::websocket::SubProtocolFactory<Echo> {
    public:
        using web::websocket::SubProtocolFactory<Echo>::SubProtocolFactory;

    private:
        Echo* create(web::websocket::SubProtocolContext* subProtocolContext) override;
    };

} // namespace apps::websocket::subprotocol::echo::server

extern "C" apps::websocket::subprotocol::echo::server::EchoFactory* echoServerSubProtocolFactory();

#endif // APPS_WEBSOCKET_SUBPROTOCOL_ECHO_SERVER_ECHOINTERFACE_H
