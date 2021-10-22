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

#ifndef WEB_WS_SERVER_SOCKETCONTEXTFACTORY_H
#define WEB_WS_SERVER_SOCKETCONTEXTFACTORY_H

#include "web/http/client/SocketContextUpgradeFactory.h"
#include "web/websocket/client/SocketContext.h"

namespace net::socket::stream {
    class SocketConnection;
} // namespace net::socket::stream

namespace web::http::client {
    class Request;
} // namespace web::http::client

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef> // for size_t
#include <string>  // for string

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::websocket::client {

    class SocketContextUpgradeFactory : public web::http::client::SocketContextUpgradeFactory {
    public:
        SocketContextUpgradeFactory() = default;

        SocketContextUpgradeFactory(const SocketContextUpgradeFactory&) = delete;

        SocketContextUpgradeFactory& operator=(const SocketContextUpgradeFactory&) = delete;

        void deleted(SocketContext* socketContext);

        static void link();

    private:
        void prepare(web::http::client::Request& request) override;

        std::string name() override;

        SocketContext* create(net::socket::stream::SocketConnection* socketConnection) override;

        std::size_t refCount = 0;
    };

    extern "C" web::http::client::SocketContextUpgradeFactory* websocketClientContextUpgradeFactory();

} // namespace web::websocket::client

#endif // WEB_WS_SERVER_SOCKETCONTEXTFACTORY_H
