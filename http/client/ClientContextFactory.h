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

#ifndef HTTPSERVERCONTEXTFACTORY_H
#define HTTPSERVERCONTEXTFACTORY_H

#include "http/client/ClientContext.h"
#include "net/socket/stream/SocketConnectionBase.h"
#include "net/socket/stream/SocketProtocolFactory.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace http::client {

    template <typename RequestT, typename ResponseT>
    class ClientContextFactory : public net::socket::stream::SocketProtocolFactory {
    public:
        using Request = RequestT;
        using Response = ResponseT;

        ClientContextFactory(const std::function<void(Response&)>& onResponse, const std::function<void(int, const std::string&)>& onRequestError)
            : onResponse(onResponse)
        , onRequestError(onRequestError){
        }

        net::socket::stream::SocketProtocol* create() const override {
            return new ClientContext<Request, Response>(onResponse, onRequestError);
        }

    protected:
        std::function<void(Response&)> onResponse;
        std::function<void(int, const std::string&)> onRequestError;
    };

} // namespace http::client

#endif // CLIENTCONTEXTFACTORY_H
