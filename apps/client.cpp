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

#include "ClientResponse.h"
#include "EventLoop.h"
#include "HTTPResponseParser.h"
#include "legacy/HTTPClient.h"
#include "socket/legacy/SocketClient.h"
#include "socket/tls/SocketClient.h"
#include "tls/HTTPClient.h"

#include <cstring>
#include <easylogging++.h>
#include <iostream>
#include <openssl/x509v3.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#define CERTF "/home/voc/projects/ServerVoc/certs/calisto.home.vchrist.at_-_snode.c_-_client.pem"
#define KEYF "/home/voc/projects/ServerVoc/certs/Volker_Christian_-_Web_-_snode.c_-_client.key.encrypted.pem"
#define KEYFPASS "snode.c"
#define SERVERCAFILE "/home/voc/projects/ServerVoc/certs/Volker_Christian_-_Root_CA.crt"

using namespace net::socket;

int main(int argc, char* argv[]) {
    net::EventLoop::init(argc, argv);

    http::legacy::HTTPClient client(
        [](net::socket::legacy::SocketConnection* socketConnection) -> void {
            VLOG(0) << "-- OnConnect";
            socketConnection->enqueue("GET /index.html HTTP/1.1\r\n\r\n"); // Connection:keep-alive\r\n\r\n");

            VLOG(0) << "\tServer: " + socketConnection->getRemoteAddress().host() + "(" + socketConnection->getRemoteAddress().ip() +
                           "):" + std::to_string(socketConnection->getRemoteAddress().port());
            VLOG(0) << "\tClient: " + socketConnection->getLocalAddress().host() + "(" + socketConnection->getLocalAddress().ip() +
                           "):" + std::to_string(socketConnection->getLocalAddress().port());
        },
        [](const http::ClientResponse& clientResponse) -> void {
            VLOG(0) << "-- OnResponse";
            VLOG(0) << "-- " << clientResponse.httpVersion;
            VLOG(0) << "-- " << clientResponse.statusCode;
            VLOG(0) << "-- " << clientResponse.reason;
        },
        []([[maybe_unused]] net::socket::legacy::SocketConnection* socketConnection) -> void {
            VLOG(0) << "-- OnDisconnect";
            VLOG(0) << "\tServer: " + socketConnection->getRemoteAddress().host() + "(" + socketConnection->getRemoteAddress().ip() +
                           "):" + std::to_string(socketConnection->getRemoteAddress().port());
            VLOG(0) << "\tClient: " + socketConnection->getLocalAddress().host() + "(" + socketConnection->getLocalAddress().ip() +
                           "):" + std::to_string(socketConnection->getLocalAddress().port());
        });

    client.connect("localhost", 8080, [](int err) -> void {
        if (err != 0) {
            PLOG(ERROR) << "OnError: " << err;
        }
    });

    client.connect("localhost", 8080, [](int err) -> void {
        if (err != 0) {
            PLOG(ERROR) << "OnError: " << err;
        }
    });

    http::tls::HTTPClient tlsClient(
        [](net::socket::tls::SocketConnection* socketConnection) -> void {
            VLOG(0) << "-- OnConnect";
            socketConnection->enqueue("GET /index.html HTTP/1.1\r\n\r\n"); // Connection:keep-alive\r\n\r\n");

            VLOG(0) << "\tServer: " + socketConnection->getRemoteAddress().host() + "(" + socketConnection->getRemoteAddress().ip() +
                           "):" + std::to_string(socketConnection->getRemoteAddress().port());
            VLOG(0) << "\tClient: " + socketConnection->getLocalAddress().host() + "(" + socketConnection->getLocalAddress().ip() +
                           "):" + std::to_string(socketConnection->getLocalAddress().port());
        },
        [](const http::ClientResponse& clientResponse) -> void {
            VLOG(0) << "-- OnResponse";
            VLOG(0) << "-- " << clientResponse.httpVersion;
            VLOG(0) << "-- " << clientResponse.statusCode;
            VLOG(0) << "-- " << clientResponse.reason;
        },
        []([[maybe_unused]] net::socket::tls::SocketConnection* socketConnection) -> void {
            VLOG(0) << "-- OnDisconnect";
            VLOG(0) << "\tServer: " + socketConnection->getRemoteAddress().host() + "(" + socketConnection->getRemoteAddress().ip() +
                           "):" + std::to_string(socketConnection->getRemoteAddress().port());
            VLOG(0) << "\tClient: " + socketConnection->getLocalAddress().host() + "(" + socketConnection->getLocalAddress().ip() +
                           "):" + std::to_string(socketConnection->getLocalAddress().port());
        },
        SERVERCAFILE);

    tlsClient.connect("localhost", 8088, [](int err) -> void {
        if (err != 0) {
            PLOG(ERROR) << "OnError: " << err;
        }
    });

    tlsClient.connect("localhost", 8088, [](int err) -> void {
        if (err != 0) {
            PLOG(ERROR) << "OnError: " << err;
        }
    });

    net::EventLoop::start();

    return 0;
}
