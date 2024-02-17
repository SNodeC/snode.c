/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024
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

#include "web/http/SocketContext.h"        // IWYU pragma: export
#include "web/http/server/RequestParser.h" // IWYU pragma: export

namespace web::http::server {
    class Request;
    class Response;
} // namespace web::http::server

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>
#include <list>
#include <memory> // IWYU pragma: export
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
                      const std::function<void(std::shared_ptr<Request>& req, std::shared_ptr<Response>& res)>& onRequestReady);

    private:
        void requestParsed();
        void requestError(int status, const std::string& reason);
        void sendToPeerStarted() override;
        void sendToPeerCompleted(bool success) override;
        void requestCompleted() override;

        std::function<void(std::shared_ptr<Request>& req, std::shared_ptr<Response>& res)> onRequestReady;

        void onConnected() override;
        std::size_t onReceivedFromPeer() override;
        void onDisconnected() override;
        [[nodiscard]] bool onSignal(int signum) override;

        std::shared_ptr<Request> request = nullptr;
        std::shared_ptr<Response> response = nullptr;
        std::list<std::shared_ptr<Request>> requests;

        RequestParser parser;

        friend Response;
    };

} // namespace web::http::server

#endif // WEB_HTTP_SERVER_SOCKETCONTEXT_H
