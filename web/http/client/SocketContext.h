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

#ifndef WEB_HTTP_CLIENT_SOCKETCONTEXT_H
#define WEB_HTTP_CLIENT_SOCKETCONTEXT_H

#include "web/http/SocketContext.h"
#include "web/http/client/ResponseParser.h"

namespace net::socket::stream {
    class SocketConnection;
} // namespace net::socket::stream

namespace web::http::client {
    class Request;
    class Response;
} // namespace web::http::client

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::http::client {

    template <typename RequestT, typename ResponseT>
    class SocketContext : public web::http::SocketContext {
    public:
        using Request = RequestT;
        using Response = ResponseT;

        SocketContext(net::socket::stream::SocketConnection* socketConnection,
                      const std::function<void(RequestT&, Response&)>& onResponse,
                      const std::function<void(int, const std::string&)>& onError);

        ~SocketContext() override = default;

        Request& getRequest();
        Response& getResponse();

    private:
        void onReceiveFromPeer() override;

        void onWriteError(int errnum) override;
        void onReadError(int errnum) override;

        void terminateConnection() override;

        void sendToPeerCompleted() override;

        Request request;
        Response response;

        ResponseParser parser;
    };

} // namespace web::http::client

#endif // WEB_HTTP_CLIENT_SOCKETCONTEXT_H
