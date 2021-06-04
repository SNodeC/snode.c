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

#ifndef WEB_WS_SUBPROTOCOL_SUBPROTOCOLPLUGININTERFACE_H
#define WEB_WS_SUBPROTOCOL_SUBPROTOCOLPLUGININTERFACE_H

#include "web/ws/SubProtocolPluginInterface.h"
#include "web/ws/server/SubProtocol.h"

#include <list>

namespace web::http::server {

    class Request;
    class Response;

} // namespace web::http::server

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::ws::server {

    extern "C" {
        class SubProtocolPluginInterface : public web::ws::SubProtocolPluginInterface {
        public:
            SubProtocolPluginInterface(const char* (*name)(),
                                       web::ws::SubProtocol::Role (*role)(),
                                       web::ws::SubProtocol* (*create)(web::http::server::Request&, web::http::server::Response&),
                                       void (*destroy)(web::ws::SubProtocol*))
                : web::ws::SubProtocolPluginInterface(name, role, create, destroy) {
            }

            std::list<web::ws::server::SubProtocol*>* getClients() {
                return &clients;
            }

            std::list<web::ws::server::SubProtocol*> clients;
        };
    }

} // namespace web::ws::server

#endif // WEB_WS_SUBPROTOCOL_SUBPROTOCOLPLUGININTERFACE_H
