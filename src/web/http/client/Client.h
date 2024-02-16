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

#ifndef WEB_HTTP_CLIENT_CLIENT_H
#define WEB_HTTP_CLIENT_CLIENT_H

#include "web/http/client/SocketContextFactory.h" // IWYU pragma: export

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>
#include <map>
#include <string>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace web::http::client {

    template <template <typename SocketContextFactoryT, typename... Args> typename SocketClientT, typename RequestT, typename ResponseT>
    class Client
        : public SocketClientT<web::http::client::SocketContextFactory<RequestT, ResponseT>,
                               std::function<void(const std::shared_ptr<RequestT>&)>,
                               std::function<void(const std::shared_ptr<RequestT>&, const std::shared_ptr<ResponseT>&)>,
                               std::function<void(int, const std::string&)>> {
    public:
        using Request = RequestT;
        using Response = ResponseT;

    private:
        using Super = SocketClientT<web::http::client::SocketContextFactory<Request, Response>,
                                    std::function<void(const std::shared_ptr<Request>&)>,
                                    std::function<void(const std::shared_ptr<Request>&, const std::shared_ptr<Response>&)>,
                                    std::function<void(int, const std::string&)>>; // this makes it an HTTP client;

    public:
        using SocketConnection = typename Super::SocketConnection;
        using SocketAddress = typename Super::SocketAddress;

        Client(const std::string& name,
               const std::function<void(SocketConnection*)>& onConnect,
               const std::function<void(SocketConnection*)>& onConnected,
               const std::function<void(SocketConnection*)>& onDisconnect,
               std::function<void(const std::shared_ptr<Request>&)>&& onRequestBegin,
               std::function<void(const std::shared_ptr<Request>&, const std::shared_ptr<Response>&)>&& onResponseReady,
               std::function<void(int, const std::string&)>&& onResponseError)
            : Super(name,
                    onConnect,
                    onConnected,
                    onDisconnect,
                    std::move(onRequestBegin),
                    std::move(onResponseReady),
                    std::move(onResponseError)) {
        }

        Client(const std::function<void(SocketConnection*)>& onConnect,
               const std::function<void(SocketConnection*)>& onConnected,
               const std::function<void(SocketConnection*)>& onDisconnect,
               std::function<void(const std::shared_ptr<Request>&)>&& onRequestBegin,
               std::function<void(const std::shared_ptr<Request>&, const std::shared_ptr<Response>&)>&& onResponseReady,
               std::function<void(int, const std::string&)>&& onResponseError)
            : Client("",
                     onConnect,
                     onConnected,
                     onDisconnect,
                     std::move(onRequestBegin),
                     std::move(onResponseReady),
                     std::move(onResponseError)) {
        }

        Client(const std::string& name,
               std::function<void(const std::shared_ptr<Request>&)>&& onRequestBegin,
               std::function<void(const std::shared_ptr<Request>&, const std::shared_ptr<Response>&)>&& onResponseReady,
               std::function<void(int, const std::string&)>&& onResponseError)
            : Super(name, std::move(onRequestBegin), std::move(onResponseReady), std::move(onResponseError)) {
        }

        Client(std::function<void(const std::shared_ptr<Request>&)>&& onRequestBegin,
               std::function<void(const std::shared_ptr<Request>&, const std::shared_ptr<Response>&)>&& onResponseReady,
               std::function<void(int, const std::string&)>&& onResponseError)
            : Client("", std::move(onRequestBegin), std::move(onResponseReady), std::move(onResponseError)) {
        }
    };

} // namespace web::http::client

#endif // WEB_HTTP_CLIENT_CLIENT_H
