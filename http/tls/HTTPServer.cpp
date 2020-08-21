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

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "HTTPServer.h"

#include "HTTPServerContext.h"
#include "Request.h"
#include "Response.h"
#include "socket/tls/SocketConnection.h"
#include "socket/tls/SocketServer.h"

namespace http {

    namespace tls {

        HTTPServer::HTTPServer(const std::function<void(net::socket::tls::SocketConnection*)>& onConnect,
                               const std::function<void(Request& req, Response& res)>& onRequestReady,
                               const std::function<void(Request& req, Response& res)>& onResponseCompleted,
                               const std::function<void(net::socket::tls::SocketConnection*)>& onDisconnect, const std::string& cert,
                               const std::string& key, const std::string& password)
            : onConnect(onConnect)
            , onRequestReady(onRequestReady)
            , onResponseCompleted(onResponseCompleted)
            , onDisconnect(onDisconnect)
            , cert(cert)
            , key(key)
            , password(password) {
        }

        void HTTPServer::listen(in_port_t port, const std::function<void(int err)>& onError) {
            errno = 0;

            (new net::socket::tls::SocketServer(
                 [this](net::socket::tls::SocketConnection* socketConnection) -> void { // onConnect
                     onConnect(socketConnection);
                     socketConnection->setProtocol<HTTPServerContext*>(new HTTPServerContext(
                         socketConnection,
                         [this](Request& req, Response& res) -> void {
                             onRequestReady(req, res);
                         },
                         [this](Request& req, Response& res) -> void {
                             onResponseCompleted(req, res);
                         }));
                 },
                 [this](net::socket::tls::SocketConnection* socketConnection) -> void { // onDisconnect
                     onDisconnect(socketConnection);
                     socketConnection->getProtocol<HTTPServerContext*>([](HTTPServerContext*& protocol) -> void {
                         delete protocol;
                     });
                 },
                 [](net::socket::tls::SocketConnection* socketConnection, const char* junk, ssize_t junkSize) -> void { // onRead
                     socketConnection->getProtocol<HTTPServerContext*>([&junk, &junkSize](HTTPServerContext*& protocol) -> void {
                         protocol->receiveRequestData(junk, junkSize);
                     });
                 },
                 [](net::socket::tls::SocketConnection* socketConnection, int errnum) -> void { // onReadError
                     socketConnection->getProtocol<HTTPServerContext*>([&errnum](HTTPServerContext*& protocol) -> void {
                         protocol->onReadError(errnum);
                     });
                 },
                 [](net::socket::tls::SocketConnection* socketConnection, int errnum) -> void { // onWriteError
                     socketConnection->getProtocol<HTTPServerContext*>([&errnum](HTTPServerContext*& protocol) -> void {
                         protocol->onWriteError(errnum);
                     });
                 },
                 cert, key, password))
                ->listen(port, 5, [&](int err) -> void {
                    if (onError) {
                        onError(err);
                    }
                });
        }

    } // namespace tls

} // namespace http
