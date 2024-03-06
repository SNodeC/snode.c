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

#include "core/socket/stream/SocketContext.h" // IWYU pragma: export
#include "web/http/server/RequestParser.h"    // IWYU pragma: export

namespace web::http::server {
    class Response;
} // namespace web::http::server

namespace core::socket::stream {
    class SocketConnection;
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef>
#include <functional>
#include <list>
#include <memory> // IWYU pragma: export
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::http::server {

    class SocketContext : public core::socket::stream::SocketContext {
    private:
        using Super = core::socket::stream::SocketContext;

        using Request = web::http::server::Request;
        using Response = web::http::server::Response;

    public:
        SocketContext(core::socket::stream::SocketConnection* socketConnection,
                      const std::function<void(const std::shared_ptr<Request>&, const std::shared_ptr<Response>&)>& onRequestReady);

    private:
        void requestStarted();
        void requestParsed();
        void requestParseError(int status, const std::string& reason);
        void responseStarted();
        void responseCompleted(bool success);
        void requestCompleted();

        std::function<void(const std::shared_ptr<Request>&, const std::shared_ptr<Response>&)> onRequestReady;

        void onConnected() override;
        std::size_t onReceivedFromPeer() override;
        void onDisconnected() override;
        bool onSignal(int signum) override;
        void onWriteError(int errnum) override;

        std::shared_ptr<Request> currentRequest = nullptr;
        std::shared_ptr<Response> masterResponse = nullptr;
        std::list<std::shared_ptr<Request>> requests;

        RequestParser parser;

        bool httpClose = false;

        friend Response;
    };

} // namespace web::http::server

#endif // WEB_HTTP_SERVER_SOCKETCONTEXT_H
