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

#ifndef WEB_HTTP_CLIENT_CLIENT_H
#define WEB_HTTP_CLIENT_CLIENT_H

#include "web/http/client/SocketContextFactory.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <any>        // IWYU pragma: export
#include <functional> // IWYU pragma: export
#include <map>        // IWYU pragma: export
#include <string>     // IWYU pragma: export

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace web::http::client {

    template <template <typename SocketContextFactoryT> typename SocketClientT, typename RequestT, typename ResponseT>
    class Client : public SocketClientT<web::http::client::SocketContextFactory<RequestT, ResponseT>> {
    public:
        using Request = RequestT;
        using Response = ResponseT;

    private:
        using Super = SocketClientT<web::http::client::SocketContextFactory<Request, Response>>; // this makes it an HTTP client;

    public:
        using SocketConnection = typename Super::SocketConnection;
        using SocketAddress = typename Super::SocketAddress;

        Client(const std::string& name,
               const std::function<void(SocketConnection*)>& onConnect,
               const std::function<void(SocketConnection*)>& onConnected,
               const std::function<void(Request&)>& onRequestBegin,
               const std::function<void(Request&, Response&)>& onResponseReady,
               const std::function<void(int, const std::string&)>& onResponseError,
               const std::function<void(SocketConnection*)>& onDisconnect,
               const std::map<std::string, std::any>& options = {{}})
            : Super(
                  name,
                  onConnect,
                  [onConnected, onRequestBegin](SocketConnection* socketConnection) -> void { // onConnected
                      Request& request =
                          dynamic_cast<web::http::client::SocketContext<Request, Response>*>(socketConnection->getSocketContext())
                              ->getRequest();
                      request.setHost(socketConnection->getRemoteAddress().toString());

                      onConnected(socketConnection);
                      onRequestBegin(request);
                  },
                  onDisconnect,
                  options) {
            Super::getSocketContextFactory()->setOnResponseReady(onResponseReady);
            Super::getSocketContextFactory()->setOnResponseError(onResponseError);
        }

        Client(const std::function<void(SocketConnection*)>& onConnect,
               const std::function<void(SocketConnection*)>& onConnected,
               const std::function<void(Request&)>& onRequestBegin,
               const std::function<void(Request&, Response&)>& onResponseReady,
               const std::function<void(int, const std::string&)>& onResponseError,
               const std::function<void(SocketConnection*)>& onDisconnect,
               const std::map<std::string, std::any>& options = {{}})
            : Client("", onConnect, onConnected, onRequestBegin, onResponseReady, onResponseError, onDisconnect, options) {
        }

        Client(const std::string& name,
               const std::function<void(Request&)>& onRequestBegin,
               const std::function<void(Request&, Response&)>& onResponseReady,
               const std::function<void(int, const std::string&)>& onResponseError,
               const std::map<std::string, std::any>& options = {{}})
            : Super(name, options) {
            Super::getSocketContextFactory()->setOnResponseReady(onResponseReady);
            Super::getSocketContextFactory()->setOnResponseError(onResponseError);

            onConnectedOrig = Super::onConnected([this, onRequestBegin](SocketConnection* socketConnection) -> void { // onConnected
                Request& request =
                    dynamic_cast<web::http::client::SocketContext<Request, Response>*>(socketConnection->getSocketContext())->getRequest();
                request.setHost(socketConnection->getRemoteAddress().toString());

                onConnectedOrig(socketConnection);
                onRequestBegin(request);
            });
        }

        Client(const std::function<void(Request&)>& onRequestBegin,
               const std::function<void(Request&, Response&)>& onResponseReady,
               const std::function<void(int, const std::string&)>& onResponseError,
               const std::map<std::string, std::any>& options = {{}})
            : Client("", onRequestBegin, onResponseReady, onResponseError, options) {
        }

        std::function<void(SocketConnection*)> onConnectedOrig;
    };

} // namespace web::http::client

#endif // WEB_HTTP_CLIENT_CLIENT_H
