/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020 Volker Christian <me@vchrist.at>
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

#ifndef CLIENT_H
#define CLIENT_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <any>
#include <functional>
#include <map>
#include <netinet/in.h>
#include <string>

#endif // DOXYGEN_SHOULD_SKIP_THIS

#include "ClientContext.h"
#include "Logger.h"

namespace http {

    template <typename SocketClientT>
    class Client {
    public:
        using SocketClient = SocketClientT;
        using SocketConnection = typename SocketClient::SocketConnection;
        using Socket = typename SocketConnection::Socket;
        using SocketAddress = typename Socket::SocketAddress;

        Client(const std::function<void(SocketConnection*)>& onConnect,
               const std::function<void(ServerRequest&)>& onRequestBegin,
               const std::function<void(ServerResponse&)>& onResponseReady,
               const std::function<void(int, const std::string&)>& onResponseError,
               const std::function<void(SocketConnection*)>& onDisconnect,
               const std::map<std::string, std::any>& options = {{}})
            : onConnect(onConnect)
            , onRequestBegin(onRequestBegin)
            , onResponseReady(onResponseReady)
            , onResponseError(onResponseError)
            , onDisconnect(onDisconnect)
            , options(options) {
        }

        void
        get(const std::map<std::string, std::any>& options,
            const std::function<void(int err)>& onError,
            const typename SocketConnection::Socket::SocketAddress& localHost = typename SocketConnection::Socket::SocketAddress()) const {
            std::string path = "";
            std::string host = "";

            for (auto& [name, value] : options) {
                if (name == "path") {
                    path = std::any_cast<const char*>(value);
                }
                if (name == "host") {
                    host = std::any_cast<const char*>(value);
                }
            }

            std::string request = "GET " + path + " HTTP/1.1\r\nHost: " + host + "\r\n\r\n";

            connect(request, options, onError, localHost);
        }

        void post(const std::map<std::string, std::any>& options,
                  const std::function<void(int err)>& onError,
                  const SocketAddress& localHost = SocketAddress()) const {
            std::string path = "";
            std::string host = "";
            std::string body = "";
            int contentLength = 0;

            for (auto& [name, value] : options) {
                if (name == "path") {
                    path = std::any_cast<const char*>(value);
                }
                if (name == "host") {
                    host = std::any_cast<const char*>(value);
                }
                if (name == "body") {
                    body = std::any_cast<const char*>(value);
                    contentLength = body.length();
                }
            }

            std::string request =
                "POST " + path + " HTTP/1.1\r\nHost: " + host + "\r\nContent-Length: " + std::to_string(contentLength) + "\r\n\r\n" + body;

            connect(request, options, onError, localHost);
        }

        void connect(const std::string& request,
                     const std::map<std::string, std::any>& options,
                     const std::function<void(int err)>& onError,
                     const SocketAddress& localHost = SocketAddress()) const {
            errno = 0;

            SocketClient socketClient(
                [request,
                 onRequestBegin = this->onRequestBegin,
                 onResponseReady = this->onResponseReady,
                 onResponseError = this->onResponseError](SocketConnection* socketConnection) -> void { // onConstruct
                    http::ClientContext* clientContext = new ClientContext(socketConnection, onResponseReady, onResponseError);
                    clientContext->setRequest(request);
                    socketConnection->template setContext<http::ClientContext*>(clientContext);
                    onRequestBegin(clientContext->serverRequest);
                },
                [](SocketConnection* socketConnection) -> void { // onDestruct
                    socketConnection->template getContext<http::ClientContext*>([](http::ClientContext* clientContext) -> void {
                        delete clientContext;
                    });
                },
                [onConnect = this->onConnect](SocketConnection* socketConnection) -> void { // onConnect
                    onConnect(socketConnection);

                    socketConnection->template getContext<http::ClientContext*>(
                        [&socketConnection](http::ClientContext* clientContext) -> void {
                            socketConnection->enqueue(clientContext->getRequest());
                        });
                },
                [onDisconnect = this->onDisconnect](SocketConnection* socketConnection) -> void { // onDisconnect
                    onDisconnect(socketConnection);
                },
                [](SocketConnection* socketConnection, const char* junk, ssize_t junkSize) -> void { // onRead
                    socketConnection->template getContext<http::ClientContext*>(
                        [junk, junkSize](http::ClientContext* clientContext) -> void {
                            clientContext->receiveResponseData(junk, junkSize);
                        });
                },
                [](SocketConnection* socketConnection, int errnum) -> void { // onReadError
                    if (errnum != 0) {
                        PLOG(ERROR) << "Server: " << socketConnection->getRemoteAddress().host() << " (" << errnum << ")";
                    } else {
                        VLOG(0) << "Server: EOF";
                    }
                },
                [](SocketConnection* socketConnection, int errnum) -> void { // onWriteError
                    if (errnum != 0) {
                        PLOG(ERROR) << "Server: " << socketConnection->getRemoteAddress().host() << " (" << errnum << ")";
                    } else {
                        VLOG(0) << "Server: EOF";
                    }
                },
                this->options);

            socketClient.connect(options, onError, localHost);
        }

        std::function<void(SocketConnection*)> onConnect;
        std::function<void(ServerRequest&)> onRequestBegin;
        std::function<void(ServerResponse&)> onResponseReady;
        std::function<void(int, const std::string&)> onResponseError;
        std::function<void(SocketConnection*)> onDisconnect;
        std::map<std::string, std::any> options;
    };

} // namespace http

#endif // CLIENT_H
