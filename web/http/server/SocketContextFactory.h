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

#include "log/Logger.h"

#ifndef WEB_HTTP_SERVER_SOCKETCONTEXTFACTORY_H
#define WEB_HTTP_SERVER_SOCKETCONTEXTFACTORY_H

#include "net/socket/stream/SocketConnection.h"
#include "net/socket/stream/SocketContextFactory.h"
#include "web/http/server/SocketContext.h"
#include "web/http/server/SocketContextUpgradeFactorySelector.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::http::server {

    template <typename RequestT, typename ResponseT>
    class SocketContextFactory : public net::socket::stream::SocketContextFactory {
    public:
        using Request = RequestT;
        using Response = ResponseT;

        SocketContextFactory() {
            if (useCount == 0) {
                SocketContextUpgradeFactorySelector::instance()->loadSocketContexts();
            }
            useCount++;
        }

        ~SocketContextFactory() {
            useCount--;
            if (useCount == 0) {
                SocketContextUpgradeFactorySelector::instance()->unloadSocketContexts();
            }
        }

        SocketContextFactory(const SocketContextFactory&) = delete;
        SocketContextFactory& operator=(const SocketContextFactory&) = delete;

    private:
        net::socket::stream::SocketContext* create(net::socket::stream::SocketConnection* socketConnection) const override {
            return new SocketContext<Request, Response>(socketConnection, onRequestReady);
        }

    public:
        void setOnRequestReady(const std::function<void(Request&, Response&)>& onRequestReady) {
            this->onRequestReady = onRequestReady;
        }

    private:
        std::function<void(Request&, Response&)> onRequestReady;

        static int useCount;
    };

    template <typename RequestT, typename ResponseT>
    int SocketContextFactory<RequestT, ResponseT>::useCount = 0;

} // namespace web::http::server

#endif // WEB_HTTP_SERVER_SOCKETCONTEXTFACTORY_H
