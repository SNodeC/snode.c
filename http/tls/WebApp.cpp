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

#include "WebApp.h"

#include "HTTPServerContext.h"
#include "socket/tls/SocketServer.h"

namespace tls {

    WebApp::WebApp(const std::string& rootDir, const std::string& cert, const std::string& key, const std::string& password)
        : ::WebApp(rootDir)
        , cert(cert)
        , key(key)
        , password(password) {
    }

    void WebApp::listen(int port, const std::function<void(int err)>& onError) {
        errno = 0;

        (new tls::SocketServer(
             [this](tls::SocketConnection* connectedSocket) -> void { // onConnect
                 connectedSocket->setProtocol<HTTPServerContext*>(new HTTPServerContext(*this, connectedSocket));
             },
             [](tls::SocketConnection* connectedSocket) -> void { // onDisconnect
                 connectedSocket->getProtocol<HTTPServerContext*>([](HTTPServerContext*& protocol) -> void {
                     delete protocol;
                 });
             },
             [](tls::SocketConnection* connectedSocket, const char* junk, ssize_t junkSize) -> void { // onRead
                 connectedSocket->getProtocol<HTTPServerContext*>([&junk, &junkSize](HTTPServerContext*& protocol) -> void {
                     protocol->receiveRequestData(junk, junkSize);
                 });
             },
             [](tls::SocketConnection* connectedSocket, int errnum) -> void { // onReadError
                 connectedSocket->getProtocol<HTTPServerContext*>([&errnum](HTTPServerContext*& protocol) -> void {
                     protocol->onReadError(errnum);
                 });
             },
             [](tls::SocketConnection* connectedSocket, int errnum) -> void { // onWriteError
                 connectedSocket->getProtocol<HTTPServerContext*>([&errnum](HTTPServerContext*& protocol) -> void {
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
