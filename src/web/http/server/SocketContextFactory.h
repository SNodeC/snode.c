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

#ifndef WEB_HTTP_SERVER_SOCKETCONTEXTFACTORY_H
#define WEB_HTTP_SERVER_SOCKETCONTEXTFACTORY_H

#include "web/http/SocketContextFactory.h"
#include "web/http/server/SocketContext.hpp"

namespace core::socket {
    class SocketConnection;
} // namespace core::socket

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::http::server {

    template <typename RequestT, typename ResponseT>
    class SocketContextFactory : public web::http::SocketContextFactory<web::http::server::SocketContext, RequestT, ResponseT> {
    public:
        using Request = RequestT;
        using Response = ResponseT;

        SocketContextFactory() = default;

        ~SocketContextFactory() override = default;

        SocketContextFactory(const SocketContextFactory&) = delete;
        SocketContextFactory& operator=(const SocketContextFactory&) = delete;

        void setOnRequestReady(const std::function<void(Request&, Response&)>& onRequestReady) {
            this->onRequestReady = onRequestReady;
        }

    private:
        core::socket::stream::SocketContext* create(core::socket::stream::SocketConnection* socketConnection) override {
            return new web::http::server::SocketContext<Request, Response>(socketConnection, onRequestReady);
        }

        std::function<void(Request&, Response&)> onRequestReady;
    };

} // namespace web::http::server

#endif // WEB_HTTP_SERVER_SOCKETCONTEXTFACTORY_H
