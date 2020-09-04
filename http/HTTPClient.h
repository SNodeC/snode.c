/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020  Volker Christian <me@vchrist.at>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef HTTPCLIENT_H
#define HTTPCLIENT_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <any>
#include <easylogging++.h>
#include <functional>
#include <map>
#include <netinet/in.h>
#include <string>

#endif // DOXYGEN_SHOULD_SKIP_THIS

#include "HTTPClientContext.h"
#include "socket/InetAddress.h"

namespace http {

    class ClientResponse;

    template <typename SocketClient>
    class HTTPClient {
    public:
        HTTPClient(const std::function<void(typename SocketClient::SocketConnection*)>& onConnect,
                   const std::function<void(ClientResponse& clientResponse)> onResponseReady,
                   const std::function<void(typename SocketClient::SocketConnection*)> onDisconnect,
                   const std::map<std::string, std::any>& options = {{}})
            : onConnect(onConnect)
            , onResponseReady(onResponseReady)
            , onDisconnect(onDisconnect)
            , options(options) {
        }

        void get(const std::map<std::string, std::any>& options, const std::function<void(int err)>& onError,
                 const net::socket::InetAddress& localHost = net::socket::InetAddress()) {
            std::string path = "";

            for (auto& [name, value] : options) {
                if (name == "path") {
                    path = std::any_cast<const char*>(value);
                }
            }

            this->request = "GET " + path + " HTTP/1.1\r\n\r\n";

            this->connect(options, onError, localHost);
        }

    protected:
        void connect(const std::map<std::string, std::any>& options, const std::function<void(int err)>& onError,
                     const net::socket::InetAddress& localHost = net::socket::InetAddress()) {
            errno = 0;

            (new SocketClient(
                 [*this](typename SocketClient::SocketConnection* socketConnection) -> void { // onConnect
                     this->onConnect(socketConnection);

                     socketConnection->template setProtocol<http::HTTPClientContext*>(new HTTPClientContext(
                         socketConnection,
                         [*this](ClientResponse& clientResponse) -> void {
                             this->onResponseReady(clientResponse);
                         },
                         []([[maybe_unused]] int status, [[maybe_unused]] const std::string& reason) -> void {
                         }));

                     socketConnection->enqueue(request);
                 },
                 [*this](typename SocketClient::SocketConnection* socketConnection) -> void { // onDisconnect
                     this->onDisconnect(socketConnection);
                     socketConnection->template getProtocol<http::HTTPClientContext*>(
                         [](http::HTTPClientContext*& httpClientContext) -> void {
                             delete httpClientContext;
                         });
                 },
                 [](typename SocketClient::SocketConnection* socketConnection, const char* junk, ssize_t junkSize) -> void { // onRead
                     socketConnection->template getProtocol<http::HTTPClientContext*>(
                         [junk, junkSize]([[maybe_unused]] http::HTTPClientContext*& clientContext) -> void {
                             clientContext->receiveResponseData(junk, junkSize);
                         });
                 },
                 [onError]([[maybe_unused]] typename SocketClient::SocketConnection* socketConnection,
                           [[maybe_unused]] int errnum) -> void { // onReadError
                     PLOG(ERROR) << "Server \"" << socketConnection->getRemoteAddress().host() << "\"";
                 },
                 [onError]([[maybe_unused]] typename SocketClient::SocketConnection* socketConnection,
                           [[maybe_unused]] int errnum) -> void { // onWriteError
                     PLOG(ERROR) << "Server: " << socketConnection->getRemoteAddress().host();
                 },
                 this->options))
                ->connect(options, onError, localHost);
        }

        std::function<void(typename SocketClient::SocketConnection*)> onConnect;
        std::function<void(ClientResponse& clientResponse)> onResponseReady;
        std::function<void(typename SocketClient::SocketConnection*)> onDisconnect;

        std::string request;

        std::map<std::string, std::any> options;
    };

} // namespace http

#endif // HTTPCLIENT_H
