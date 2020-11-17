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

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "EventLoop.h"
#include "Logger.h"
#include "ResponseParser.h"
#include "ServerResponse.h"
#include "config.h" // just for this example app
#include "socket/bluetooth/rfcomm/Socket.h"
#include "socket/sock_stream/legacy/SocketClient.h"

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

using namespace net::socket::stream;
using namespace net::socket::bluetooth;
using namespace net::socket::bluetooth::address;

static http::ResponseParser* getResponseParser() {
    http::ResponseParser* responseParser = new http::ResponseParser(
        [](void) -> void {
        },
        [](const std::string& httpVersion, const std::string& statusCode, const std::string& reason) -> void {
            VLOG(0) << "++ Response: " << httpVersion << " " << statusCode << " " << reason;
        },
        [](const std::map<std::string, std::string>& headers, const std::map<std::string, http::CookieOptions>& cookies) -> void {
            VLOG(0) << "++   Headers:";
            for (auto [field, value] : headers) {
                VLOG(0) << "++       " << field + " = " + value;
            }

            VLOG(0) << "++   Cookies:";
            for (auto [name, cookie] : cookies) {
                VLOG(0) << "++     " + name + " = " + cookie.getValue();
                for (auto [option, value] : cookie.getOptions()) {
                    VLOG(0) << "++       " + option + " = " + value;
                }
            }
        },
        [](char* content, size_t contentLength) -> void {
            char* strContent = new char[contentLength + 1];
            memcpy(strContent, content, contentLength);
            strContent[contentLength] = 0;
            VLOG(0) << "++   OnContent: " << contentLength << std::endl << strContent;
            delete[] strContent;
        },
        [](http::ResponseParser& parser) -> void {
            VLOG(0) << "++   OnParsed";
            parser.reset();
        },
        [](int status, const std::string& reason) -> void {
            VLOG(0) << "++   OnError: " + std::to_string(status) + " - " + reason;
        });

    return responseParser;
}

legacy::SocketClient<net::socket::bluetooth::rfcomm::Socket> getBtClient() {
    legacy::SocketClient<rfcomm::Socket> btClient(
        [](legacy::SocketClient<rfcomm::Socket>::SocketConnection* socketConnection) -> void { // onConstruct
            socketConnection->setContext<http::ResponseParser*>(getResponseParser());
        },
        []([[maybe_unused]] legacy::SocketClient<rfcomm::Socket>::SocketConnection* socketConnection) -> void { // onDestruct
        },
        [](legacy::SocketClient<rfcomm::Socket>::SocketConnection* socketConnection) -> void { // onConnect
            VLOG(0) << "OnConnect";
            socketConnection->enqueue("GET /index.html HTTP/1.1\r\nConnection: close\r\n\r\n"); // Connection: close\r\n\r\n");

            VLOG(0) << "\tServer: " + socketConnection->getRemoteAddress().address() + "(" +
                           socketConnection->getRemoteAddress().address() +
                           "):" + std::to_string(socketConnection->getRemoteAddress().channel());
            VLOG(0) << "\tClient: " + socketConnection->getLocalAddress().address() + "(" + socketConnection->getLocalAddress().address() +
                           "):" + std::to_string(socketConnection->getLocalAddress().channel());
        },
        [](legacy::SocketClient<rfcomm::Socket>::SocketConnection* socketConnection) -> void { // onDisconnect
            VLOG(0) << "OnDisconnect";
            VLOG(0) << "\tServer: " + socketConnection->getRemoteAddress().address() + "(" +
                           socketConnection->getRemoteAddress().address() +
                           "):" + std::to_string(socketConnection->getRemoteAddress().channel());
            VLOG(0) << "\tClient: " + socketConnection->getLocalAddress().address() + "(" + socketConnection->getLocalAddress().address() +
                           "):" + std::to_string(socketConnection->getLocalAddress().channel());

            socketConnection->getContext<http::ResponseParser*>([](http::ResponseParser*& responseParser) -> void {
                delete responseParser;
            });
        },
        [](legacy::SocketClient<rfcomm::Socket>::SocketConnection* socketConnection,
           const char* junk,
           ssize_t junkSize) -> void { // onRead
            VLOG(0) << "OnRead";

            socketConnection->getContext<http::ResponseParser*>([junk, junkSize](http::ResponseParser*& responseParser) -> void {
                responseParser->parse(junk, junkSize);
            });
        },
        []([[maybe_unused]] legacy::SocketClient<rfcomm::Socket>::SocketConnection* socketConnection,
           int errnum) -> void { // onReadError
            VLOG(0) << "OnReadError: " << errnum;
        },
        []([[maybe_unused]] legacy::SocketClient<rfcomm::Socket>::SocketConnection* socketConnection,
           int errnum) -> void { // onWriteError
            VLOG(0) << "OnWriteError: " << errnum;
        },
        {{}});

    return btClient;
}

int main(int argc, char* argv[]) {
    net::EventLoop::init(argc, argv);

    {
        RfCommAddress remoteAddress("B8:27:EB:6C:98:9E", 1); // kleinebeere
        RfCommAddress bindAddress("5C:C5:D4:B8:3C:AA");      // calisto

        legacy::SocketClient legacyClient = getBtClient();

        legacyClient.connect(
            remoteAddress,
            [](int err) -> void {
                if (err) {
                    PLOG(ERROR) << "Connect: " << std::to_string(err);
                } else {
                    VLOG(0) << "Connected";
                }
            },
            bindAddress);
    }

    return net::EventLoop::start();
}
