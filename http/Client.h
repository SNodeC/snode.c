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
#include <easylogging++.h>
#include <functional>
#include <map>
#include <netinet/in.h>
#include <string>

#endif // DOXYGEN_SHOULD_SKIP_THIS

#include "ClientContext.h"
#include "socket/InetAddress.h"

namespace http {

    class ServerResponse;

    template <typename SocketClientT>
    class Client {
        using SocketClient = SocketClientT;
        using SocketConnection = typename SocketClient::SocketConnection;

    public:
        Client(const std::function<void(SocketConnection*)>& onConnect, const std::function<void(ServerResponse&)> onResponseReady,
               const std::function<void(SocketConnection*)> onDisconnect, const std::map<std::string, std::any>& options = {{}})
            : onConnect(onConnect)
            , onResponseReady(onResponseReady)
            , onDisconnect(onDisconnect)
            , options(options) {
        }

        void get(const std::map<std::string, std::any>& options, const std::function<void(int err)>& onError,
                 const net::socket::InetAddress& localHost = net::socket::InetAddress()) {
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

            this->request = "GET " + path + " HTTP/1.1\r\nHost: " + host + "\r\n\r\n";

            this->connect(options, onError, localHost);
        }

        void post(const std::map<std::string, std::any>& options, const std::function<void(int err)>& onError,
                  const net::socket::InetAddress& localHost = net::socket::InetAddress()) {
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

            this->request =
                "POST " + path + " HTTP/1.1\r\nHost: " + host + "\r\nContent-Length: " + std::to_string(contentLength) + "\r\n\r\n" + body;

            this->connect(options, onError, localHost);
        }

    protected:
        SocketClient* socketClient() const {
            return new SocketClient(
                [this](SocketConnection* socketConnection) -> void { // onConnect
                    onConnect(socketConnection);

                    socketConnection->template setContext<http::ClientContext*>(new ClientContext(
                        socketConnection,
                        [this](ServerResponse& clientResponse) -> void {
                            onResponseReady(clientResponse);
                        },
                        []([[maybe_unused]] int status, [[maybe_unused]] const std::string& reason) -> void {
                        }));

                    socketConnection->enqueue(request);
                },
                [this](SocketConnection* socketConnection) -> void { // onDisconnect
                    onDisconnect(socketConnection);
                    socketConnection->template getContext<http::ClientContext*>([](http::ClientContext*& httpClientContext) -> void {
                        delete httpClientContext;
                    });
                },
                [](SocketConnection* socketConnection, const char* junk, ssize_t junkSize) -> void { // onRead
                    socketConnection->template getContext<http::ClientContext*>(
                        [junk, junkSize]([[maybe_unused]] http::ClientContext*& clientContext) -> void {
                            clientContext->receiveResponseData(junk, junkSize);
                        });
                },
                []([[maybe_unused]] SocketConnection* socketConnection,
                   [[maybe_unused]] int errnum) -> void { // onReadError
                    PLOG(ERROR) << "Server: " << socketConnection->getRemoteAddress().host();
                },
                []([[maybe_unused]] SocketConnection* socketConnection,
                   [[maybe_unused]] int errnum) -> void { // onWriteError
                    PLOG(ERROR) << "Server: " << socketConnection->getRemoteAddress().host();
                },
                options);
        }

        void connect(const std::map<std::string, std::any>& options, const std::function<void(int err)>& onError,
                     const net::socket::InetAddress& localHost = net::socket::InetAddress()) const {
            errno = 0;

            socketClient()->connect(options, onError, localHost);
        }

        std::function<void(SocketConnection*)> onConnect;
        std::function<void(ServerResponse&)> onResponseReady;
        std::function<void(SocketConnection*)> onDisconnect;

        std::string request;

        std::map<std::string, std::any> options;
    };

} // namespace http

#endif // CLIENT_H
