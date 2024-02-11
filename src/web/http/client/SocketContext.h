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

#include "web/http/SocketContext.h"
#include "web/http/client/ResponseParser.h"

namespace web::http::client {
    class Request;
    class Response;
} // namespace web::http::client

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>
#include <memory>
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
                      const std::function<void(int, const std::string&)>& onResponseError);

    private:
        std::size_t onReceivedFromPeer() override;

        void requestCompleted() override;

        void onConnected() override;
        void onDisconnected() override;

        [[nodiscard]] bool onSignal(int signum) override;

        void responseParsed();
        void responseError(int status, const std::string& reason);

        std::function<void(std::shared_ptr<Request>&)> onRequestBegin;
        std::function<void(std::shared_ptr<Request>& req, std::shared_ptr<Response>& res)> onResponseReady;
        std::function<void(int, const std::string&)> onResponseError;

        std::shared_ptr<Response> response = nullptr;
        std::shared_ptr<Request> request = nullptr;

        ResponseParser parser;
    };

} // namespace web::http::client

#endif // WEB_HTTP_CLIENT_SOCKETCONTEXT_H
