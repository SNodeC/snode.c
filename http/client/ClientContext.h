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

#ifndef CLIENTCONTEXT_H
#define CLIENTCONTEXT_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef>
#include <functional>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "Request.h"
#include "Response.h"
#include "ResponseParser.h"

namespace net::socket::stream {
    class SocketConnectionBase;
} // namespace net::socket::stream

namespace http::client {

    class ClientContext {
    public:
        using SocketConnection = net::socket::stream::SocketConnectionBase;

        ClientContext(SocketConnection* socketConnection,
                      const std::function<void(Response&)>& onResponse,
                      const std::function<void(int status, const std::string& reason)>& onError);

        void receiveResponseData(const char* junk, std::size_t junkLen);
        void sendRequestData(const char* buf, std::size_t len);

        Request& getRequest();

        void terminateConnection();

        void requestCompleted();

    protected:
        SocketConnection* socketConnection;

        Request request;
        Response response;

    private:
        ResponseParser parser;
    };

} // namespace http::client

#endif // CLIENTCONTEXT_H
