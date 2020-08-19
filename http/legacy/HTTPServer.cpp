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
#include "socket/legacy/SocketConnection.h"
#include "socket/legacy/SocketServer.h"

namespace http {

    namespace legacy {

        HTTPServer::HTTPServer(const std::function<void(Request& req, Response& res)>& onRequest)
            : onRequest(onRequest) {
        }

        void HTTPServer::listen(in_port_t port, const std::function<void(int err)>& onError) {
            errno = 0;

            (new ::legacy::SocketServer(
                 [this](::legacy::SocketConnection* connectedSocket) -> void { // onConnect
                     connectedSocket->setProtocol<HTTPServerContext*>(
                         new HTTPServerContext(connectedSocket, [this](Request& req, Response& res) -> void {
                             onRequest(req, res);
                         }));
                     ;
                 },
                 [](::legacy::SocketConnection* connectedSocket) -> void { // onDisconnect
                     connectedSocket->getProtocol<HTTPServerContext*>([](HTTPServerContext*& protocol) -> void {
                         delete protocol;
                     });
                 },
                 [](::legacy::SocketConnection* connectedSocket, const char* junk, ssize_t junkSize) -> void { // onRead
                     connectedSocket->getProtocol<HTTPServerContext*>([&junk, &junkSize](HTTPServerContext*& protocol) -> void {
                         protocol->receiveRequestData(junk, junkSize);
                     });
                 },
                 [](::legacy::SocketConnection* connectedSocket, int errnum) -> void { // onReadError
                     connectedSocket->getProtocol<HTTPServerContext*>([&errnum](HTTPServerContext*& protocol) -> void {
                         protocol->onReadError(errnum);
                     });
                 },
                 [](::legacy::SocketConnection* connectedSocket, int errnum) -> void { // onWriteError
                     connectedSocket->getProtocol<HTTPServerContext*>([&errnum](HTTPServerContext*& protocol) -> void {
                         protocol->onWriteError(errnum);
                     });
                 }))
                ->listen(port, 5, [&](int err) -> void {
                    if (onError) {
                        onError(err);
                    }
                });
        }

    } // namespace legacy

} // namespace http
