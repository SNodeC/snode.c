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

#define CERTF "/home/voc/projects/ServerVoc/certs/calisto.home.vchrist.at_-_snode.c_-_client.pem"
#define KEYF "/home/voc/projects/ServerVoc/certs/Volker_Christian_-_Web_-_snode.c_-_client.key.encrypted.pem"
#define KEYFPASS "snode.c"

using namespace net::socket;

tls::SocketClient tlsClient() {
    tls::SocketClient client(
        []([[maybe_unused]] tls::SocketConnection* connectedSocket) -> void { // onConnect
            std::cout << "OnConnect" << std::endl;
            connectedSocket->enqueue("GET /index.html HTTP/1.1\r\n\r\n"); // Connection:keep-alive\r\n\r\n");

            X509* client_cert = SSL_get_peer_certificate(connectedSocket->getSSL());
            if (client_cert != NULL) {
                std::cout << "Server certificate" << std::endl;

                char* str = X509_NAME_oneline(X509_get_subject_name(client_cert), 0, 0);
                std::cout << "\t subject: " << str << std::endl;
                OPENSSL_free(str);

                str = X509_NAME_oneline(X509_get_issuer_name(client_cert), 0, 0);
                std::cout << "\t issuer: " << str << std::endl;
                OPENSSL_free(str);

                // We could do all sorts of certificate verification stuff here before deallocating the certificate.

                X509_free(client_cert);
            } else {
                printf("Client does not have certificate.\n");
            }

            std::cout << connectedSocket->getRemoteAddress().port() << std::endl;
            std::cout << connectedSocket->getLocalAddress().port() << std::endl;
            std::cout << connectedSocket->getRemoteAddress().host() << std::endl;
            std::cout << connectedSocket->getLocalAddress().host() << std::endl;
            std::cout << connectedSocket->getRemoteAddress().ip() << std::endl;
            std::cout << connectedSocket->getLocalAddress().ip() << std::endl;

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
        },
        CERTF, KEYF, KEYFPASS);

    client.connect("calisto.home.vchrist.at", 8088, [](int err) -> void {
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

            std::cout << connectedSocket->getRemoteAddress().port() << std::endl;
            std::cout << connectedSocket->getLocalAddress().port() << std::endl;
            std::cout << connectedSocket->getRemoteAddress().host() << std::endl;
            std::cout << connectedSocket->getLocalAddress().host() << std::endl;
            std::cout << connectedSocket->getRemoteAddress().ip() << std::endl;
            std::cout << connectedSocket->getLocalAddress().ip() << std::endl;
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

    legacyClient.connect("calisto.home.vchrist.at", 8080, [](int err) -> void {
        if (err) {
            std::cout << "Connect: " << strerror(err) << std::endl;
        } else {
            std::cout << "Connected" << std::endl;
        }
    });

    return legacyClient;
}

int main(int argc, char* argv[]) {
    net::EventLoop::init(argc, argv);

    tls::SocketClient sc = tlsClient();
    legacy::SocketClient lc = legacyClient();

    lc.connect("calisto.home.vchrist.at", 8080, [](int err) -> void { // example.com:81 simulate connnect timeout
        if (err) {
            std::cout << "Connect: " << strerror(err) << std::endl;
        } else {
            std::cout << "Connected" << std::endl;
        }
    });

    sc.connect("calisto.home.vchrist.at", 8088, [](int err) -> void {
        if (err) {
            std::cout << "Connect: " << strerror(err) << std::endl;
        } else {
            std::cout << "Connected" << std::endl;
        }
    });

    net::EventLoop::start();

    return 0;
}
