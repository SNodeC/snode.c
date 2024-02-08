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
#include "web/http/server/RequestParser.h"

namespace web::http::server {
    class Request;
    class Response;
} // namespace web::http::server

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>
#include <list>
#include <memory>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::http::server {

    template <typename RequestT, typename ResponseT>
    class SocketContext : public web::http::SocketContext {
    private:
        using Super = web::http::SocketContext;

        using Request = RequestT;
        using Response = ResponseT;

    public:
        SocketContext(core::socket::stream::SocketConnection* socketConnection,
                      const std::function<void(std::shared_ptr<Request> req, std::shared_ptr<Response> res)>& onRequestReady);

    private:
        std::size_t onReceivedFromPeer() override;

        void requestCompleted() override;

        void onConnected() override;
        void onDisconnected() override;

        [[nodiscard]] bool onSignal(int signum) override;

        void requestParsed();
        void requestError(int status, const std::string &reason);

        std::function<void(std::shared_ptr<Request> req, std::shared_ptr<Response> res)> onRequestReady;

        std::shared_ptr<Response> response;
        std::shared_ptr<Request> request;
        RequestParser parser;

        std::list<std::shared_ptr<Request>> requests;

        bool connectionTerminated = false;

        friend Response;
    };

} // namespace web::http::server

#endif // WEB_HTTP_SERVER_SOCKETCONTEXT_H
