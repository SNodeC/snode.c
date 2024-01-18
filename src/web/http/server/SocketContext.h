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

#include "web/http/SocketContext.h" // IWYU pragma: export
#include "web/http/server/RequestContextBase.h"
#include "web/http/server/RequestParser.h"

namespace web::http::server {
    class Request;
    class Response;
} // namespace web::http::server

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>
#include <list>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::http::server {

    template <typename RequestT, typename ResponseT>
    class SocketContext : public web::http::SocketContext {
    private:
        using Super = web::http::SocketContext;

        using Request = RequestT;
        using Response = ResponseT;

        class RequestContext : public RequestContextBase {
        public:
            explicit RequestContext(SocketContext* serverContext)
                : RequestContextBase(serverContext)
                , response(this)
                , ready(false)
                , status(0) {
            }

            Request request;
            Response response;

            bool ready;

            int status;
            std::string reason;
        };

    public:
        SocketContext(core::socket::stream::SocketConnection* socketConnection,
                      const std::function<void(Request&, Response&)>& onRequestReady);

    protected:
        ~SocketContext() override;

    private:
        std::size_t onReceivedFromPeer() override;

        void sendToPeerCompleted() override;

        void onConnected() override;
        void onDisconnected() override;

        void switchSocketContext(core::socket::stream::SocketContext* socketContext) override;

        [[nodiscard]] bool onSignal(int signum) override;

        void requestParsed();

        std::function<void(Request& req, Response& res)> onRequestReady;

        RequestParser parser;

        std::list<RequestContext*> requestContexts;
        RequestContext* currentRequestContext = nullptr;

        bool requestInProgress = false;
        bool connectionTerminated = false;

        friend class RequestContextBase;
    };

} // namespace web::http::server

#endif // WEB_HTTP_SERVER_SOCKETCONTEXT_H
