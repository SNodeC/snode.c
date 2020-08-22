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
#include <easylogging++.h>
#include <iostream>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#define CERTF "/home/voc/projects/ServerVoc/certs/calisto.home.vchrist.at_-_snode.c_-_client.pem"
#define KEYF "/home/voc/projects/ServerVoc/certs/Volker_Christian_-_Web_-_snode.c_-_client.key.encrypted.pem"
#define KEYFPASS "snode.c"
#define SERVERCAFILE "/home/voc/projects/ServerVoc/certs/Volker_Christian_-_Root_CA.crt"

using namespace net::socket;

tls::SocketClient tlsClient() {
    tls::SocketClient client(
        []([[maybe_unused]] tls::SocketConnection* socketConnection) -> void { // onConnect
            VLOG(0) << "OnConnect";
            socketConnection->enqueue("GET /index.html HTTP/1.1\r\n\r\n"); // Connection:keep-alive\r\n\r\n");

            VLOG(0) << "\tServer: " + socketConnection->getRemoteAddress().host() + "(" + socketConnection->getRemoteAddress().ip() +
                           "):" + std::to_string(socketConnection->getRemoteAddress().port());
            VLOG(0) << "\tClient: " + socketConnection->getLocalAddress().host() + "(" + socketConnection->getLocalAddress().ip() +
                           "):" + std::to_string(socketConnection->getLocalAddress().port());

            X509* server_cert = SSL_get_peer_certificate(socketConnection->getSSL());
            if (server_cert != NULL) {
                VLOG(0) << "Server certificate";

                char* str = X509_NAME_oneline(X509_get_subject_name(server_cert), 0, 0);
                VLOG(0) << "\t subject: " + std::string(str);
                OPENSSL_free(str);

                str = X509_NAME_oneline(X509_get_issuer_name(server_cert), 0, 0);
                VLOG(0) << "\t issuer: " + std::string(str);
                OPENSSL_free(str);

                // We could do all sorts of certificate verification stuff here before deallocating the certificate.

                X509_free(server_cert);

                int verifyErr = SSL_get_verify_result(socketConnection->getSSL());
                VLOG(0) << "Certificate verify result: " + std::string(X509_verify_cert_error_string(verifyErr));
            } else {
                printf("Client does not have certificate.\n");
            }
        },
        []([[maybe_unused]] tls::SocketConnection* socketConnection) -> void { // onDisconnect
            VLOG(0) << "OnDisconnect";
            VLOG(0) << "\tServer: " + socketConnection->getRemoteAddress().host() + "(" + socketConnection->getRemoteAddress().ip() +
                           "):" + std::to_string(socketConnection->getRemoteAddress().port());
            VLOG(0) << "\tClient: " + socketConnection->getLocalAddress().host() + "(" + socketConnection->getLocalAddress().ip() +
                           "):" + std::to_string(socketConnection->getLocalAddress().port());
        },
        []([[maybe_unused]] tls::SocketConnection* socketConnection, const char* junk, ssize_t junkSize) -> void { // onRead
            VLOG(0) << "OnRead";
            char* buf = new char[junkSize + 1];
            memcpy(buf, junk, junkSize);
            buf[junkSize] = 0;
            VLOG(0) << "------------ begin data";
            VLOG(0) << buf;
            VLOG(0) << "------------ end data";
            delete[] buf;
        },
        []([[maybe_unused]] tls::SocketConnection* socketConnection, int errnum) -> void { // onReadError
            VLOG(0) << "OnReadError: " + std::to_string(errnum);
        },
        []([[maybe_unused]] tls::SocketConnection* socketConnection, int errnum) -> void { // onWriteError
            VLOG(0) << "OnWriteError: " + std::to_string(errnum);
        },
        CERTF, KEYF, KEYFPASS, SERVERCAFILE);

    client.connect("calisto.home.vchrist.at", 8088, [](int err) -> void {
        if (err) {
            VLOG(0) << "Connect: " + std::string(strerror(err));
        } else {
            VLOG(0) << "Connected";
        }
    });

    return client;
}

legacy::SocketClient legacyClient() {
    legacy::SocketClient legacyClient(
        []([[maybe_unused]] legacy::SocketConnection* socketConnection) -> void { // onConnect
            VLOG(0) << "OnConnect";
            socketConnection->enqueue("GET /index.html HTTP/1.1\r\n\r\n"); // Connection:keep-alive\r\n\r\n");

            VLOG(0) << "\tServer: " + socketConnection->getRemoteAddress().host() + "(" + socketConnection->getRemoteAddress().ip() +
                           "):" + std::to_string(socketConnection->getRemoteAddress().port());
            VLOG(0) << "\tClient: " + socketConnection->getLocalAddress().host() + "(" + socketConnection->getLocalAddress().ip() +
                           "):" + std::to_string(socketConnection->getLocalAddress().port());
        },
        []([[maybe_unused]] legacy::SocketConnection* socketConnection) -> void { // onDisconnect
            VLOG(0) << "OnDisconnect";
            VLOG(0) << "\tServer: " + socketConnection->getRemoteAddress().host() + "(" + socketConnection->getRemoteAddress().ip() +
                           "):" + std::to_string(socketConnection->getRemoteAddress().port());
            VLOG(0) << "\tClient: " + socketConnection->getLocalAddress().host() + "(" + socketConnection->getLocalAddress().ip() +
                           "):" + std::to_string(socketConnection->getLocalAddress().port());
        },
        []([[maybe_unused]] legacy::SocketConnection* socketConnection, const char* junk, ssize_t junkSize) -> void { // onRead
            VLOG(0) << "OnRead";
            char* buf = new char[junkSize + 1];
            memcpy(buf, junk, junkSize);
            buf[junkSize] = 0;
            VLOG(0) << "------------ begin data";
            VLOG(0) << buf;
            VLOG(0) << "------------ end data";
            delete[] buf;
        },
        []([[maybe_unused]] legacy::SocketConnection* socketConnection, int errnum) -> void { // onReadError
            VLOG(0) << "OnReadError: " << errnum;
        },
        []([[maybe_unused]] legacy::SocketConnection* socketConnection, int errnum) -> void { // onWriteError
            VLOG(0) << "OnWriteError: " << errnum;
        });

    legacyClient.connect("calisto.home.vchrist.at", 8080, [](int err) -> void {
        if (err) {
            VLOG(0) << "Connect: " << strerror(err);
        } else {
            VLOG(0) << "Connected";
        }
    });

    return legacyClient;
}

int main(int argc, char* argv[]) {
    net::EventLoop::init(argc, argv);

    legacy::SocketClient lc = legacyClient();
    lc.connect("calisto.home.vchrist.at", 8080, [](int err) -> void { // example.com:81 simulate connnect timeout
        if (err) {
            VLOG(0) << "Connect: " << strerror(err);
        } else {
            VLOG(0) << "Connected";
        }
    });

    tls::SocketClient sc = tlsClient();
    sc.connect("calisto.home.vchrist.at", 8088, [](int err) -> void {
        if (err) {
            VLOG(0) << "Connect: " << strerror(err);
        } else {
            VLOG(0) << "Connected";
        }
    });

    net::EventLoop::start();

    return 0;
}
