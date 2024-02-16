/*
 * SNode.C - a slim toolkit for network communication
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

#ifndef WEB_HTTP_CLIENT_SOCKETCONTEXT_H
#define WEB_HTTP_CLIENT_SOCKETCONTEXT_H

#include "web/http/SocketContext.h"         // IWYU pragma: export
#include "web/http/client/ResponseParser.h" // IWYU pragma: export

namespace web::http::client {
    class Request;
    class Response;
} // namespace web::http::client

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>
#include <memory> // IWYU pragma: export
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::http::client {

    template <typename RequestT, typename ResponseT>
    class SocketContext : public web::http::SocketContext {
    private:
        using Super = web::http::SocketContext;

        using Request = RequestT;
        using Response = ResponseT;

    public:
        SocketContext(core::socket::stream::SocketConnection* socketConnection,
                      const std::function<void(std::shared_ptr<Request>&)>& onRequestBegin,
                      const std::function<void(std::shared_ptr<Request>&, std::shared_ptr<Response>&)>& onResponseReady,
                      const std::function<void(int, const std::string&)>& onResponseParseError,
                      const std::function<void(const std::shared_ptr<Request>&)>& onRequestEnd);

    private:
        void sendToPeerCompleted(bool success) override;
        void responseParsed();
        void responseError(int status, const std::string& reason);
        void requestCompleted() override;

        std::function<void(std::shared_ptr<Request>&)> onRequestBegin;
        std::function<void(std::shared_ptr<Request>& req, std::shared_ptr<Response>& res)> onResponseReady;
        std::function<void(int, const std::string&)> onResponseParseError;
        std::function<void(const std::shared_ptr<Request>&)> onRequestEnd;

        void onConnected() override;
        std::size_t onReceivedFromPeer() override;
        void onDisconnected() override;
        [[nodiscard]] bool onSignal(int signum) override;

        std::shared_ptr<Request> request = nullptr;
        std::shared_ptr<Response> response = nullptr;

        ResponseParser parser;

        friend class web::http::client::Request;
    };

} // namespace web::http::client

#endif // WEB_HTTP_CLIENT_SOCKETCONTEXT_H
