/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020  Volker Christian <me@vchrist.at>
 * Json Middleware 2020 Marlene Mayr, Anna Moser, Matteo Prock, Eric Thalhammer 
 * Github <MarleneMayr><moseranna><MatteoMatteoMatteo><peregrin-tuk>
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

#include "ClientResponse.h"
#include "EventLoop.h"
#include "legacy/HTTPClient.h"
#include "socket/legacy/SocketClient.h"
#include "socket/tls/SocketClient.h"
#include "tls/HTTPClient.h"

#include <cstring>
#include <easylogging++.h>
#include <iostream>
#include <openssl/x509v3.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#define CERTF "/home/student/nds/snode.c/certs/calisto.home.vchrist.at_-_snode.c_-_client.pem"
#define KEYF "/home/student/nds/snode.c/certs/Volker_Christian_-_Web_-_snode.c_-_client.key.encrypted.pem"
#define KEYFPASS "snode.c"
#define SERVERCAFILE "/home/student/nds/snode.c/certs/Volker_Christian_-_Root_CA.crt"

int main(int argc, char* argv[]) {
    net::EventLoop::init(argc, argv);

    http::legacy::HTTPClient jsonClient(
        [](net::socket::legacy::SocketConnection* socketConnection) -> void {
            VLOG(0) << "-- OnConnect";
            VLOG(0) << "     Server: " + socketConnection->getRemoteAddress().host() + "(" + socketConnection->getRemoteAddress().ip() +
                           "):" + std::to_string(socketConnection->getRemoteAddress().port());
            VLOG(0) << "     Client: " + socketConnection->getLocalAddress().host() + "(" + socketConnection->getLocalAddress().ip() +
                           "):" + std::to_string(socketConnection->getLocalAddress().port());
        },
        [](const http::ClientResponse& clientResponse) -> void {
            VLOG(0) << "-- OnResponse";
            VLOG(0) << "     Status:";
            VLOG(0) << "       " << clientResponse.httpVersion;
            VLOG(0) << "       " << clientResponse.statusCode;
            VLOG(0) << "       " << clientResponse.reason;

            VLOG(0) << "     Headers:";
            for (auto [field, value] : *clientResponse.headers) {
                VLOG(0) << "       " << field + " = " + value;
            }

            VLOG(0) << "     Cookies:";
            for (auto [name, cookie] : *clientResponse.cookies) {
                VLOG(0) << "       " + name + " = " + cookie.getValue();
                for (auto [option, value] : cookie.getOptions()) {
                    VLOG(0) << "         " + option + " = " + value;
                }
            }

            char* body = new char[clientResponse.contentLength + 1];
            memcpy(body, clientResponse.body, clientResponse.contentLength);
            body[clientResponse.contentLength] = 0;

            VLOG(1) << "     Body:\n----------- start body -----------\n" << body << "------------ end body ------------";

            delete[] body;
        },
        []([[maybe_unused]] net::socket::legacy::SocketConnection* socketConnection) -> void {
            VLOG(0) << "-- OnDisconnect";
            VLOG(0) << "     Server: " + socketConnection->getRemoteAddress().host() + "(" + socketConnection->getRemoteAddress().ip() +
                           "):" + std::to_string(socketConnection->getRemoteAddress().port());
            VLOG(0) << "     Client: " + socketConnection->getLocalAddress().host() + "(" + socketConnection->getLocalAddress().ip() +
                           "):" + std::to_string(socketConnection->getLocalAddress().port());
        });


    jsonClient.post({{"host", "localhost"}, {"port", 8080}, {"path", "/index.html"},
                        {"body", "{\"userId\":1,\"schnitzel\":\"good\",\"hungry\":false}"}},
                    [](int err) -> void {
        if (err != 0) {
            PLOG(ERROR) << "OnError: " << err;
        }
    }); // Connection:keep-alive\r\n\r\n"

    net::EventLoop::start();

    return 0;
}
