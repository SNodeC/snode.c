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

#include "ResponseParser.h"
#include "ServerRequest.h"
#include "ServerResponse.h"

namespace net::socket::stream {
    class SocketConnectionBase;
}

namespace http {

    class ClientContext {
    public:
        using SocketConnection = net::socket::stream::SocketConnectionBase;

        ClientContext(SocketConnection* socketConnection,
                      const std::function<void(ServerResponse&)>& onResponse,
                      const std::function<void(int status, const std::string& reason)>& onError);

        void receiveResponseData(const char* junk, size_t junkLen);

        void setRequest(const std::string& request);
        const std::string& getRequest();

    protected:
        SocketConnection* socketConnection;

        std::string request;

    public:
        ServerRequest serverRequest;
        ServerResponse serverResponse;

    private:
        ResponseParser parser;
    };

} // namespace http

#endif // CLIENTCONTEXT_H
