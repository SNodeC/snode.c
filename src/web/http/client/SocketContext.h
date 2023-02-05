/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021, 2022, 2023 Volker Christian <me@vchrist.at>
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

#ifndef WEB_HTTP_CLIENT_SOCKETCONTEXT_H
#define WEB_HTTP_CLIENT_SOCKETCONTEXT_H

#include "web/http/SocketContext.h"
#include "web/http/client/ResponseParser.h"

namespace core::socket {
    class SocketConnection;
} // namespace core::socket

namespace web::http::client {
    class Request;
    class Response;
} // namespace web::http::client

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::http::client {

    using SocketContextSuper = web::http::SocketContext;

    template <typename RequestT, typename ResponseT>
    class SocketContext : public SocketContextSuper {
    private:
        using Super = SocketContextSuper;

        using Request = RequestT;
        using Response = ResponseT;

    public:
        SocketContext(core::socket::SocketConnection* socketConnection,
                      const std::function<void(Request&)>& onRequestBegin,
                      const std::function<void(Request&, Response&)>& onResponse,
                      const std::function<void(int, const std::string&)>& onError);

    protected:
        ~SocketContext() override = default;

    public:
        Request& getRequest();
        Response& getResponse();

    private:
        std::size_t onReceiveFromPeer() override;

        void sendToPeerCompleted() override;

        void onConnected() override;
        void onDisconnected() override;

        std::function<void(Request&)> onRequestBegin;

        Request request;
        Response response;

        ResponseParser parser;
    };

} // namespace web::http::client

#endif // WEB_HTTP_CLIENT_SOCKETCONTEXT_H
