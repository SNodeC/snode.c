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

#ifndef WEB_HTTP_SERVER_HTTPSERVERCONTEXTFACTORY_H
#define WEB_HTTP_SERVER_HTTPSERVERCONTEXTFACTORY_H

#include "net/socket/stream/SocketConnectionBase.h"
#include "net/socket/stream/SocketProtocolFactory.h"
#include "web/http/server/HTTPServerContext.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::http::server {

    template <typename RequestT, typename ResponseT>
    class HTTPServerContextFactory : public net::socket::stream::SocketProtocolFactory {
    public:
        using Request = RequestT;
        using Response = ResponseT;

        HTTPServerContextFactory() = default;

        HTTPServerContextFactory(const HTTPServerContextFactory&) = delete;
        HTTPServerContextFactory& operator=(const HTTPServerContextFactory&) = delete;

    private:
        net::socket::stream::SocketProtocol* create() const override {
            return new HTTPServerContext<Request, Response>(onRequestReady);
        }

    public:
        void setOnRequestReady(const std::function<void(Request& req, Response& res)>& onRequestReady) {
            this->onRequestReady = onRequestReady;
        }

    private:
        std::function<void(Request& req, Response& res)> onRequestReady;
    };

} // namespace web::http::server

#endif // WEB_HTTP_SERVER_HTTPSERVERCONTEXTFACTORY_H
