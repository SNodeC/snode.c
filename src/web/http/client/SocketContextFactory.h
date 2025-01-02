/*
 * SNode.C - a slim toolkit for network communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025
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

#ifndef WEB_HTTP_CLIENT_SOCKETCONTEXTFACTORY_H
#define WEB_HTTP_CLIENT_SOCKETCONTEXTFACTORY_H

#include "web/http/SocketContextFactory.h"
#include "web/http/client/ConfigHTTP.h"
#include "web/http/client/SocketContext.h" // IWYU pragma: export

namespace core::socket::stream {
    class SocketConnection;
} // namespace core::socket::stream

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::http::client {

    template <typename RequestT, typename ResponseT>
    class SocketContextFactory : public web::http::SocketContextFactory<web::http::client::SocketContext, RequestT, ResponseT> {
    public:
        using Request = RequestT;
        using Response = ResponseT;

        SocketContextFactory(const std::function<void(const std::shared_ptr<Request>&)>& onRequestBegin,
                             const std::function<void(const std::shared_ptr<Request>&)>& onRequestEnd,
                             const std::function<net::config::ConfigInstance&()>& getConfigInstance)
            : onRequestBegin(onRequestBegin)
            , onRequestEnd(onRequestEnd)
            , configHttp(web::http::client::ConfigHTTP(getConfigInstance())) {
        }

        void setPipelinedRequests(bool pipelinedRequests) {
            configHttp.setPipelinedRequests(pipelinedRequests);
        }

    private:
        core::socket::stream::SocketContext* create(core::socket::stream::SocketConnection* socketConnection) override {
            return new web::http::client::SocketContext(socketConnection, onRequestBegin, onRequestEnd, configHttp.getPipelinedRequests());
        }

        std::function<void(const std::shared_ptr<Request>&)> onRequestBegin;
        std::function<void(const std::shared_ptr<Request>&)> onRequestEnd;

        ConfigHTTP configHttp;
    };

} // namespace web::http::client

#endif // WEB_HTTP_CLIENT_SOCKETCONTEXTFACTORY_H
