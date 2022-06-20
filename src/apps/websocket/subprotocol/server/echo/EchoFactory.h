/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021, 2022 Volker Christian <me@vchrist.at>
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

#ifndef WEB_WEBSOCKET_SUBPROTOCOL_ECHO_SERVER_ECHOINTERFACE_H
#define WEB_WEBSOCKET_SUBPROTOCOL_ECHO_SERVER_ECHOINTERFACE_H

#include "web/websocket/SubProtocolFactory.h"

namespace web::websocket::subprotocol::echo::server {
    class Echo;
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <string> // for string

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::websocket::subprotocol::echo::server {

    class EchoFactory : public web::websocket::SubProtocolFactory<Echo> {
    public:
        using web::websocket::SubProtocolFactory<Echo>::SubProtocolFactory;

    private:
        Echo* create() override;
    };

} // namespace web::websocket::subprotocol::echo::server

extern "C" void* echoServerSubProtocolFactory();

#endif // ECHOINTERFACE_H
