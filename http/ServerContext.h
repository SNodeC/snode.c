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

#ifndef SERVERCONTEXT_H
#define SERVERCONTEXT_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "Request.h"
#include "RequestParser.h"
#include "Response.h"

namespace net::socket {
    class SocketConnectionBase;
}

namespace http {

    class ServerContext {
    public:
        ServerContext(net::socket::SocketConnectionBase* socketConnection,
                      const std::function<void(Request& req, Response& res)>& onRequestReady,
                      const std::function<void(Request& req, Response& res)>& onRequestCompleted);

        ~ServerContext();

        void receiveRequestData(const char* junk, size_t junkLen);
        void onReadError(int errnum);

        void sendResponseData(const char* buf, size_t len);
        void onWriteError(int errnum);

        void responseCompleted();

        void terminateConnection();

    private:
        net::socket::SocketConnectionBase* socketConnection;

        bool requestInProgress = false;

    public:
        Request request;
        Response response;

    private:
        std::function<void(Request& req, Response& res)> onRequestCompleted;

        RequestParser parser;
    };

} // namespace http

#endif // SERVERCONTEXT_H
