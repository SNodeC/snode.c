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
#include "ResponseParser.h"
#include "ServerResponse.h"
#include "socket/legacy/SocketClient.h"
#include "socket/tls/SocketClient.h"

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

static http::ResponseParser* getResponseParser() {
    http::ResponseParser* responseParser = new http::ResponseParser(
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

tls::SocketClient getTlsClient() {
    tls::SocketClient tlsClient(
        [](tls::SocketConnection* socketConnection) -> void { // onStart
            socketConnection->setContext<http::ResponseParser*>(getResponseParser());
        },
        [](tls::SocketConnection* socketConnection) -> void { // onConnect
            VLOG(0) << "OnConnect";
            socketConnection->enqueue("GET /index.html HTTP/1.1\r\n\r\n"); // Connection:keep-alive\r\n\r\n");

            VLOG(0) << "\tClient: " + socketConnection->getLocalAddress().host() + "(" + socketConnection->getLocalAddress().ip() +
                           "):" + std::to_string(socketConnection->getLocalAddress().port());
            VLOG(0) << "\tServer: " + socketConnection->getRemoteAddress().host() + "(" + socketConnection->getRemoteAddress().ip() +
                           "):" + std::to_string(socketConnection->getRemoteAddress().port());

            X509* server_cert = SSL_get_peer_certificate(socketConnection->getSSL());
            if (server_cert != NULL) {
                int verifyErr = SSL_get_verify_result(socketConnection->getSSL());

                VLOG(0) << "\tServer certificate: " + std::string(X509_verify_cert_error_string(verifyErr));

                char* str = X509_NAME_oneline(X509_get_subject_name(server_cert), 0, 0);
                VLOG(0) << "\t   Subject: " + std::string(str);
                OPENSSL_free(str);

                str = X509_NAME_oneline(X509_get_issuer_name(server_cert), 0, 0);
                VLOG(0) << "\t   Issuer: " + std::string(str);
                OPENSSL_free(str);

                // We could do all sorts of certificate verification stuff here before deallocating the certificate.

                GENERAL_NAMES* subjectAltNames =
                    static_cast<GENERAL_NAMES*>(X509_get_ext_d2i(server_cert, NID_subject_alt_name, NULL, NULL));

                int32_t altNameCount = sk_GENERAL_NAME_num(subjectAltNames);
                VLOG(0) << "\t   Subject alternative name count: " << altNameCount;
                for (int32_t i = 0; i < altNameCount; ++i) {
                    GENERAL_NAME* generalName = sk_GENERAL_NAME_value(subjectAltNames, i);
                    if (generalName->type == GEN_URI) {
                        std::string subjectAltName =
                            std::string(reinterpret_cast<const char*>(ASN1_STRING_get0_data(generalName->d.uniformResourceIdentifier)),
                                        ASN1_STRING_length(generalName->d.uniformResourceIdentifier));
                        VLOG(0) << "\t      SAN (URI): '" + subjectAltName;
                    } else if (generalName->type == GEN_DNS) {
                        std::string subjectAltName =
                            std::string(reinterpret_cast<const char*>(ASN1_STRING_get0_data(generalName->d.dNSName)),
                                        ASN1_STRING_length(generalName->d.dNSName));
                        VLOG(0) << "\t      SAN (DNS): '" + subjectAltName;
                    } else {
                        VLOG(0) << "\t      SAN (Type): '" + std::to_string(generalName->type);
                    }
                    //                    sk_GENERAL_NAME_free(generalName);
                }
                sk_GENERAL_NAME_pop_free(subjectAltNames, GENERAL_NAME_free);

                X509_free(server_cert);
            } else {
                VLOG(0) << "\tServer certificate: no certificate";
            }
        },
        [](tls::SocketConnection* socketConnection) -> void { // onDisconnect
            VLOG(0) << "OnDisconnect";
            VLOG(0) << "\tClient: " + socketConnection->getLocalAddress().host() + "(" + socketConnection->getLocalAddress().ip() +
                           "):" + std::to_string(socketConnection->getLocalAddress().port());
            VLOG(0) << "\tServer: " + socketConnection->getRemoteAddress().host() + "(" + socketConnection->getRemoteAddress().ip() +
                           "):" + std::to_string(socketConnection->getRemoteAddress().port());

            socketConnection->getContext<http::ResponseParser*>([](http::ResponseParser*& responseParser) -> void {
                delete responseParser;
            });
        },
        [](tls::SocketConnection* socketConnection, const char* junk, ssize_t junkSize) -> void { // onRead
            VLOG(0) << "OnRead";
            socketConnection->getContext<http::ResponseParser*>([junk, junkSize](http::ResponseParser*& responseParser) -> void {
                responseParser->parse(junk, junkSize);
            });
        },
        []([[maybe_unused]] tls::SocketConnection* socketConnection, int errnum) -> void { // onReadError
            VLOG(0) << "OnReadError: " + std::to_string(errnum);
        },
        []([[maybe_unused]] tls::SocketConnection* socketConnection, int errnum) -> void { // onWriteError
            VLOG(0) << "OnWriteError: " + std::to_string(errnum);
        },
        {{"certChain", CERTF}, {"keyPEM", KEYF}, {"password", KEYFPASS}, {"caFile", SERVERCAFILE}});

    tlsClient.connect({{"host", "localhost"}, {"port", 8088}}, [](int err) -> void {
        if (err) {
            PLOG(ERROR) << "Connect: " + std::to_string(err);
        } else {
            VLOG(0) << "Connected";
        }
    });

    return tlsClient;
}

legacy::SocketClient getLegacyClient() {
    legacy::SocketClient legacyClient(
        [](legacy::SocketConnection* socketConnection) -> void {
            socketConnection->setContext<http::ResponseParser*>(getResponseParser());
        },
        [](legacy::SocketConnection* socketConnection) -> void { // onConnect
            VLOG(0) << "OnConnect";
            socketConnection->enqueue("GET /index.html HTTP/1.1\r\n\r\n"); // Connection:keep-alive\r\n\r\n");

            VLOG(0) << "\tServer: " + socketConnection->getRemoteAddress().host() + "(" + socketConnection->getRemoteAddress().ip() +
                           "):" + std::to_string(socketConnection->getRemoteAddress().port());
            VLOG(0) << "\tClient: " + socketConnection->getLocalAddress().host() + "(" + socketConnection->getLocalAddress().ip() +
                           "):" + std::to_string(socketConnection->getLocalAddress().port());
        },
        [](legacy::SocketConnection* socketConnection) -> void { // onDisconnect
            VLOG(0) << "OnDisconnect";
            VLOG(0) << "\tServer: " + socketConnection->getRemoteAddress().host() + "(" + socketConnection->getRemoteAddress().ip() +
                           "):" + std::to_string(socketConnection->getRemoteAddress().port());
            VLOG(0) << "\tClient: " + socketConnection->getLocalAddress().host() + "(" + socketConnection->getLocalAddress().ip() +
                           "):" + std::to_string(socketConnection->getLocalAddress().port());
            socketConnection->getContext<http::ResponseParser*>([](http::ResponseParser*& responseParser) -> void {
                delete responseParser;
            });
        },
        [](legacy::SocketConnection* socketConnection, const char* junk, ssize_t junkSize) -> void { // onRead
            VLOG(0) << "OnRead";
            socketConnection->getContext<http::ResponseParser*>([junk, junkSize](http::ResponseParser*& responseParser) -> void {
                responseParser->parse(junk, junkSize);
            });
        },
        []([[maybe_unused]] legacy::SocketConnection* socketConnection, int errnum) -> void { // onReadError
            VLOG(0) << "OnReadError: " << errnum;
        },
        []([[maybe_unused]] legacy::SocketConnection* socketConnection, int errnum) -> void { // onWriteError
            VLOG(0) << "OnWriteError: " << errnum;
        },
        {{}});

    legacyClient.connect({{"host", "localhost"}, {"port", 8080}}, [](int err) -> void {
        if (err) {
            PLOG(ERROR) << "Connect: " << std::to_string(err);
        } else {
            VLOG(0) << "Connected";
        }
    });

    return legacyClient;
}

int main(int argc, char* argv[]) {
    net::EventLoop::init(argc, argv);

    {
        legacy::SocketClient legacyClient = getLegacyClient();
        legacyClient.connect({{"host", "localhost"}, {"port", 8080}}, [](int err) -> void { // example.com:81 simulate connnect timeout
            if (err) {
                PLOG(ERROR) << "Connect: " << std::to_string(err);
            } else {
                VLOG(0) << "Connected";
            }
        });

        tls::SocketClient tlsClient = getTlsClient();
        tlsClient.connect({{"host", "localhost"}, {"port", 8088}}, [](int err) -> void {
            if (err) {
                PLOG(ERROR) << "Connect: " << std::to_string(err);
            } else {
                VLOG(0) << "Connected";
            }
        });
    }

    return net::EventLoop::start();
}
