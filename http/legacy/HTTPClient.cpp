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

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <easylogging++.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "HTTPClient.h"
#include "HTTPClientContext.h"
#include "socket/legacy/SocketClient.h"

namespace http {

    namespace legacy {

        HTTPClient::HTTPClient(const std::function<void(net::socket::legacy::SocketConnection*)>& onConnect,
                               const std::function<void(ClientResponse& clientResponse)> onResponseReady,
                               const std::function<void(net::socket::legacy::SocketConnection*)> onDisconnect,
                               const std::map<std::string, std::any>& options)
            : onConnect(onConnect)
            , onResponseReady(onResponseReady)
            , onDisconnect(onDisconnect)
            , options(options) {
        }

        void HTTPClient::connect(const std::string& server, in_port_t port, const std::function<void(int err)>& onError) {
            errno = 0;

            (new net::socket::legacy::SocketClient(
                 [*this](net::socket::legacy::SocketConnection* socketConnection) -> void { // onConnect
                     this->onConnect(socketConnection);
                     socketConnection->setProtocol<http::HTTPClientContext*>(new HTTPClientContext(
                         socketConnection,
                         [*this]([[maybe_unused]] ClientResponse& clientResponse) -> void {
                             this->onResponseReady(clientResponse);
                         },
                         []([[maybe_unused]] int status, [[maybe_unused]] const std::string& reason) -> void {
                         }));
                     socketConnection->enqueue(request);
                 },
                 [*this](net::socket::legacy::SocketConnection* socketConnection) -> void { // onDisconnect
                     this->onDisconnect(socketConnection);
                     socketConnection->getProtocol<http::HTTPClientContext*>([](http::HTTPClientContext*& httpClientContext) -> void {
                         delete httpClientContext;
                     });
                 },
                 [](net::socket::legacy::SocketConnection* socketConnection, const char* junk, ssize_t junkSize) -> void { // onRead
                     socketConnection->getProtocol<http::HTTPClientContext*>(
                         [junk, junkSize]([[maybe_unused]] http::HTTPClientContext*& clientContext) -> void {
                             clientContext->receiveResponseData(junk, junkSize);
                         });
                 },
                 []([[maybe_unused]] net::socket::legacy::SocketConnection* socketConnection, int errnum) -> void { // onReadError
                     VLOG(0) << "OnReadError: " << errnum;
                 },
                 []([[maybe_unused]] net::socket::legacy::SocketConnection* socketConnection, int errnum) -> void { // onWriteError
                     VLOG(0) << "OnWriteError: " << errnum;
                 },
                 options))
                ->connect(server, port, onError);
        }

        void HTTPClient::get(const std::map<std::string, std::any>& options, const std::function<void(int err)>& onError) {
            for (auto& [name, value] : options) {
                if (name == "host") {
                    this->host = std::any_cast<std::string>(value);
                } else if (name == "port") {
                    this->port = std::any_cast<in_port_t>(value);
                } else if (name == "path") {
                    this->path = std::any_cast<std::string>(value);
                }
            }

            this->request = "GET " + this->path + " HTTP/1.1\r\n\r\n";

            this->connect(host, port, onError);
        }

    } // namespace legacy

} // namespace http
