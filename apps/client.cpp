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

#include "EventLoop.h"
#include "socket/legacy/SocketClient.h"
#include "socket/tls/SocketClient.h"

#include <cstring>
#include <iostream>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

tls::SocketClient tlsClient() {
    tls::SocketClient client(
        []([[maybe_unused]] tls::SocketConnection* connectedSocket) -> void { // onConnect
            std::cout << "OnConnect" << std::endl;
            connectedSocket->enqueue("GET /index.html HTTP/1.1\r\n\r\n"); // Connection:keep-alive\r\n\r\n");
        },
        []([[maybe_unused]] tls::SocketConnection* connectedSocket) -> void { // onDisconnect
            std::cout << "OnDisconnect" << std::endl;
        },
        []([[maybe_unused]] tls::SocketConnection* connectedSocket, const char* junk, ssize_t junkSize) -> void { // onRead
            std::cout << "OnRead" << std::endl;
            char* buf = new char[junkSize + 1];
            memcpy(buf, junk, junkSize);
            buf[junkSize] = 0;
            std::cout << "------------ begin data" << std::endl;
            std::cout << buf;
            std::cout << "------------ end data" << std::endl;
            delete[] buf;
        },
        []([[maybe_unused]] tls::SocketConnection* connectedSocket, int errnum) -> void { // onReadError
            std::cout << "OnReadError: " << errnum << std::endl;
        },
        []([[maybe_unused]] tls::SocketConnection* connectedSocket, int errnum) -> void { // onWriteError
            std::cout << "OnWriteError: " << errnum << std::endl;
        });

    client.connect("localhost", 8088, [](int err) -> void {
        if (err) {
            std::cout << "Connect: " << strerror(err) << std::endl;
        } else {
            std::cout << "Connected" << std::endl;
        }
    });

    return client;
}

legacy::SocketClient legacyClient() {
    legacy::SocketClient legacyClient(
        []([[maybe_unused]] legacy::SocketConnection* connectedSocket) -> void { // onConnect
            std::cout << "OnConnect" << std::endl;
            connectedSocket->enqueue("GET /index.html HTTP/1.1\r\n\r\n"); // Connection:keep-alive\r\n\r\n");
        },
        []([[maybe_unused]] legacy::SocketConnection* connectedSocket) -> void { // onDisconnect
            std::cout << "OnDisconnect" << std::endl;
        },
        []([[maybe_unused]] legacy::SocketConnection* connectedSocket, const char* junk, ssize_t junkSize) -> void { // onRead
            std::cout << "OnRead" << std::endl;
            char* buf = new char[junkSize + 1];
            memcpy(buf, junk, junkSize);
            buf[junkSize] = 0;
            std::cout << "------------ begin data" << std::endl;
            std::cout << buf;
            std::cout << "------------ end data" << std::endl;
            delete[] buf;
        },
        []([[maybe_unused]] legacy::SocketConnection* connectedSocket, int errnum) -> void { // onReadError
            std::cout << "OnReadError: " << errnum << std::endl;
        },
        []([[maybe_unused]] legacy::SocketConnection* connectedSocket, int errnum) -> void { // onWriteError
            std::cout << "OnWriteError: " << errnum << std::endl;
        });

    legacyClient.connect("localhost", 8080, [](int err) -> void {
        if (err) {
            std::cout << "Connect: " << strerror(err) << std::endl;
        } else {
            std::cout << "Connected" << std::endl;
        }
    });

    return legacyClient;
}

int main(int argc, char* argv[]) {
    EventLoop::init(argc, argv);

    tls::SocketClient sc = tlsClient();
    legacy::SocketClient lc = legacyClient();

    lc.connect("localhost", 8080, [](int err) -> void {
        if (err) {
            std::cout << "Connect: " << strerror(err) << std::endl;
        } else {
            std::cout << "Connected" << std::endl;
        }
    });

    sc.connect("localhost", 8088, [](int err) -> void {
        if (err) {
            std::cout << "Connect: " << strerror(err) << std::endl;
        } else {
            std::cout << "Connected" << std::endl;
        }
    });

    EventLoop::start();

    return 0;
}
