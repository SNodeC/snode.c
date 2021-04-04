/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020 Volker Christian <me@vchrist.at>
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

#ifndef HTTP_CLIENT_CLIENTCONTEXT_H
#define HTTP_CLIENT_CLIENTCONTEXT_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef>
#include <functional>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "http/client/Request.h"
#include "http/client/Response.h"
#include "http/client/ResponseParser.h"

namespace net::socket::stream {
    class SocketConnectionBase;
} // namespace net::socket::stream

namespace http::client {

    class ClientContextBase {
    public:
        virtual ~ClientContextBase() = default;

        virtual void receiveResponseData(const char* junk, std::size_t junkLen) = 0;
        virtual void sendRequestData(const char* buf, std::size_t len) = 0;

        virtual Request& getRequest() = 0;

        virtual void terminateConnection() = 0;
        virtual void requestCompleted() = 0;
    };

    template <typename RequestT, typename ResponseT>
    class ClientContext : public ClientContextBase {
    public:
        using SocketConnection = net::socket::stream::SocketConnectionBase;
        using Request = RequestT;
        using Response = ResponseT;

        ClientContext(SocketConnection* socketConnection,
                      const std::function<void(Response&)>& onResponse,
                      const std::function<void(int status, const std::string& reason)>& onError);

        ~ClientContext() override = default;

        void receiveResponseData(const char* junk, std::size_t junkLen) override;
        void sendRequestData(const char* junk, std::size_t junkLen) override;

        Request& getRequest() override;

        void terminateConnection() override;

        void requestCompleted() override;

    protected:
        SocketConnection* socketConnection;

        Request request;
        Response response;

    private:
        ResponseParser parser;
    };

} // namespace http::client

#endif // HTTP_CLIENT_CLIENTCONTEXT_H
