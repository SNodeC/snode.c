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
#include "socket/tls/SocketClient.h"
#include "socket/tls/SocketConnection.h"

namespace http {

    namespace tls {

        HTTPClient::HTTPClient(const std::function<void(net::socket::tls::SocketConnection*)>& onConnect,
                               const std::function<void(ClientResponse& clientResponse)> onResponseReady,
                               const std::function<void(net::socket::tls::SocketConnection*)> onDisconnect, const std::string& caFile,
                               const std::string& caDir, bool useDefaultCADir)
            : onConnect(onConnect)
            , onResponseReady(onResponseReady)
            , onDisconnect(onDisconnect)
            , caFile(caFile)
            , caDir(caDir)
            , useDefaultCADir(useDefaultCADir) {
        }

        void HTTPClient::connect([[maybe_unused]] const std::string& server, [[maybe_unused]] in_port_t port,
                                 [[maybe_unused]] const std::function<void(int err)>& onError) {
            errno = 0;

            (new net::socket::tls::SocketClient(
                 [this](net::socket::tls::SocketConnection* socketConnection) -> void { // onConnect
                     this->onConnect(socketConnection);
                     socketConnection->setProtocol<http::HTTPClientContext*>(new HTTPClientContext(
                         socketConnection,
                         [this]([[maybe_unused]] ClientResponse& clientResponse) -> void {
                             this->onResponseReady(clientResponse);
                         },
                         []([[maybe_unused]] int status, [[maybe_unused]] const std::string& reason) -> void {
                         }));
                 },
                 [this](net::socket::tls::SocketClient* socketClient,
                        net::socket::tls::SocketConnection* socketConnection) -> void { // onDisconnect
                     this->onDisconnect(socketConnection);
                     delete socketClient;
                     socketConnection->getProtocol<http::HTTPClientContext*>([](http::HTTPClientContext*& httpClientContext) -> void {
                         delete httpClientContext;
                     });
                 },
                 [](net::socket::tls::SocketConnection* socketConnection, const char* junk, ssize_t junkSize) -> void { // onRead
                     socketConnection->getProtocol<http::HTTPClientContext*>(
                         [junk, junkSize]([[maybe_unused]] http::HTTPClientContext*& clientContext) -> void {
                             clientContext->receiveResponseData(junk, junkSize);
                         });
                 },
                 []([[maybe_unused]] net::socket::tls::SocketConnection* socketConnection, int errnum) -> void { // onReadError
                     VLOG(0) << "OnReadError: " << errnum;
                 },
                 []([[maybe_unused]] net::socket::tls::SocketConnection* socketConnection, int errnum) -> void { // onWriteError
                     VLOG(0) << "OnWriteError: " << errnum;
                 },
                 caFile, caDir, useDefaultCADir))
                ->connect(server, port, onError);
        }

    } // namespace tls

} // namespace http
