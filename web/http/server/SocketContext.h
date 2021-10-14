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

#ifndef WEB_HTTP_SERVER_SOCKETCONTEXT_H
#define WEB_HTTP_SERVER_SOCKETCONTEXT_H

#include "log/Logger.h"
#include "net/socket/stream/SocketContext.h"
#include "web/http/server/RequestParser.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <list>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::socket::stream {
    class SocketConnection;
}

namespace web::http::server {

    class SocketContext : public net::socket::stream::SocketContext {
    public:
        using SocketConnection = net::socket::stream::SocketConnection;

        explicit SocketContext(net::socket::stream::SocketConnection* socketConnection);

        ~SocketContext() override = default;

        SocketContext(const SocketContext&) = delete;
        SocketContext& operator=(const SocketContext&) = delete;

        virtual void sendToPeerCompleted() = 0;

        virtual void terminateConnection() = 0;
    };

    template <typename RequestT, typename ResponseT>
    class SocketContextT : public SocketContext {
    public:
        using SocketConnection = net::socket::stream::SocketConnection;
        using Request = RequestT;
        using Response = ResponseT;

        struct RequestContext {
            RequestContext(SocketContext* serverContext)
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

        SocketContextT(net::socket::stream::SocketConnection* socketConnection,
                       const std::function<void(Request&, Response&)>& onRequestReady);

    private:
        void stop() override;

        void onReceiveFromPeer() override;

        void onReadError(int errnum) override;
        void onWriteError(int errnum) override;

        void sendToPeerCompleted() override;

        void terminateConnection() override;

        std::function<void(Request& req, Response& res)> onRequestReady;

        RequestParser parser;

        std::list<RequestContext> requestContexts;

        bool requestInProgress = false;
        bool connectionTerminated = false;

        void requestParsed();

        void reset();
    };

} // namespace web::http::server

#endif // WEB_HTTP_SERVER_SOCKETCONTEXT_H
