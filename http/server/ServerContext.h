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

#include <list>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "Request.h"
#include "RequestParser.h"
#include "Response.h"

namespace net::socket::stream {
    class SocketConnectionBase;
}

namespace http::server {

    class ServerContextBase {
    public:
        virtual ~ServerContextBase() = default;

        virtual void receiveRequestData(const char* junk, std::size_t junkLen) = 0;
        virtual void sendResponseData(const char* buf, std::size_t len) = 0;
        virtual void responseCompleted() = 0;
        virtual void terminateConnection() = 0;

        virtual void onWriteError(int errnum) = 0;
        virtual void onReadError(int errnum) = 0;
    };

    template <typename RequestT, typename ResponseT>
    class ServerContext : public ServerContextBase {
    public:
        using SocketConnection = net::socket::stream::SocketConnectionBase;
        using Request = RequestT;
        using Response = ResponseT;

        struct RequestContext {
            RequestContext(ServerContext<Request, Response>* serverContext)
                : response(serverContext)
                , ready(false)
                , status(0) {
            }

            Request request;
            Response response;

            bool ready;

            int status;
            std::string reason;
        };

        ServerContext(SocketConnection* socketConnection, const std::function<void(Request& req, Response& res)>& onRequestReady);

        ~ServerContext() = default;

        void receiveRequestData(const char* junk, std::size_t junkLen) override;
        void onReadError(int errnum);

        void sendResponseData(const char* buf, std::size_t len) override;
        void onWriteError(int errnum) override;

        void responseCompleted() override;
        void terminateConnection() override;

    private:
        SocketConnection* socketConnection;

        std::function<void(Request& req, Response& res)> onRequestReady;

        RequestParser parser;

        std::list<RequestContext> requestContexts;

        bool requestInProgress = false;
        bool connectionTerminated = false;

        void requestParsed();

        void reset();
    };

} // namespace http::server

#endif // SERVERCONTEXT_H
