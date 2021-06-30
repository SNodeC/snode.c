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

#ifndef WEB_HTTP_CLIENT_SOCKETCONTEXTFACTORY_H
#define WEB_HTTP_CLIENT_SOCKETCONTEXTFACTORY_H

#include "net/socket/stream/SocketConnection.h"
#include "net/socket/stream/SocketContextFactory.h"
#include "web/http/client/SocketContext.h"
#include "web/http/client/SocketContextUpgradeFactorySelector.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::http::client {

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

    private:
        net::socket::stream::SocketContext* create(net::socket::stream::SocketConnection* socketConnection) const override {
            return new SocketContextT<Request, Response>(socketConnection, onResponse, onRequestError);
        }

    public:
        void setOnResponse(const std::function<void(Request&, Response&)>& onResponse) {
            this->onResponse = onResponse;
        }

        void setOnRequestError(const std::function<void(int, const std::string&)> onRequestError) {
            this->onRequestError = onRequestError;
        }

    private:
        std::function<void(Request&, Response&)> onResponse;
        std::function<void(int, const std::string&)> onRequestError;

        static int useCount;
    };

    template <typename RequestT, typename ResponseT>
    int SocketContextFactory<RequestT, ResponseT>::useCount = 0;

} // namespace web::http::client

#endif // WEB_HTTP_CLIENT_SOCKETCONTEXTFACTORY_H
