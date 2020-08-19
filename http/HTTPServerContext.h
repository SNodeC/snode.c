/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020  Volker Christian <me@vchrist.at>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef HTTPSERVERCONTEXT_H
#define HTTPSERVERCONTEXT_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "HTTPRequestParser.h"
#include "Request.h"
#include "Response.h"

namespace net::socket {
    class SocketConnectionBase;
}

namespace http {

    class HTTPServerContext {
    public:
        HTTPServerContext(net::socket::SocketConnectionBase* connectedSocket,
                          const std::function<void(Request& req, Response& res)>& onRequestReady,
                          const std::function<void(Request& req, Response& res)>& onResponseFinished);

        void receiveRequestData(const char* junk, size_t junkLen);
        void onReadError(int errnum);

        void sendResponseData(const char* buf, size_t len);
        void onWriteError(int errnum);

        void requestReady();
        void responseCompleted();

        void terminateConnection();

    private:
        net::socket::SocketConnectionBase* connectedSocket;

        bool requestInProgress = false;

    public:
        std::function<void(Request& req, Response& res)> onRequestReady;
        std::function<void(Request& req, Response& res)> onResponseFinished;

        Request request;
        Response response;

    private:
        HTTPRequestParser parser;
    };

} // namespace http

#endif // HTTPSERVERCONTEXT_H
