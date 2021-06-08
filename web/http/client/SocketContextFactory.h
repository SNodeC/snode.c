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

#include "net/socket/stream/SocketConnectionBase.h"
#include "net/socket/stream/SocketContextFactory.h"
#include "web/http/client/SocketContext.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::http::client {

    template <typename RequestT, typename ResponseT>
    class SocketContextFactory : public net::socket::stream::SocketContextFactory {
    public:
        using Request = RequestT;
        using Response = ResponseT;

        SocketContextFactory() = default;

    private:
        net::socket::stream::SocketContext* create(net::socket::stream::SocketConnectionBase* socketConnection) const override {
            return new SocketContext<Request, Response>(socketConnection, onResponse, onRequestError);
        }

        std::string name() override {
            return "http";
        }

        std::string type() override {
            return "client";
        }

    public:
        void setOnResponse(const std::function<void(Response&)>& onResponse) {
            this->onResponse = onResponse;
        }

        void setOnRequestError(const std::function<void(int, const std::string&)> onRequestError) {
            this->onRequestError = onRequestError;
        }

    protected:
        std::function<void(Response&)> onResponse;
        std::function<void(int, const std::string&)> onRequestError;
    };

} // namespace web::http::client

#endif // WEB_HTTP_CLIENT_SOCKETCONTEXTFACTORY_H
