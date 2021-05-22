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

#ifndef HTTP_SERVER_HTTPSERVERCONTEXT_H
#define HTTP_SERVER_HTTPSERVERCONTEXT_H

#include "web/server/http/Request.h"
#include "web/server/http/RequestParser.h"
#include "web/server/http/Response.h"
#include "net/socket/stream/SocketProtocol.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <list>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::socket::stream {
    class SocketConnectionBase;
}

namespace web::server::http {

    class HTTPServerContextBase : public net::socket::stream::SocketProtocol {
    public:
        using SocketConnection = net::socket::stream::SocketConnectionBase;

        virtual void sendResponseData(const char* buf, std::size_t len) = 0;
        virtual void responseCompleted() = 0;
        virtual void terminateConnection() = 0;
    };

    template <typename RequestT, typename ResponseT>
    class HTTPServerContext : public HTTPServerContextBase {
    public:
        using SocketConnection = net::socket::stream::SocketConnectionBase;
        using Request = RequestT;
        using Response = ResponseT;

        struct RequestContext {
            RequestContext(HTTPServerContextBase* serverContext)
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

        HTTPServerContext(const std::function<void(Request& req, Response& res)>& onRequestReady);

        ~HTTPServerContext() override = default;

        void receiveFromPeer(const char* junk, std::size_t junkLen) override;
        void onReadError(int errnum) override;

        void sendResponseData(const char* junk, std::size_t junkLen) override;
        void onWriteError(int errnum) override;

        void responseCompleted() override;
        void terminateConnection() override;

    private:
        std::function<void(Request& req, Response& res)> onRequestReady;

        RequestParser parser;

        std::list<RequestContext> requestContexts;

        bool requestInProgress = false;
        bool connectionTerminated = false;

        void requestParsed();

        void reset();
    };

} // namespace web::server

#endif // HTTP_SERVER_HTTPSERVERCONTEXT_H
