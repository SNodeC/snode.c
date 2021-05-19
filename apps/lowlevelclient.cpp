/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021 Volker Christian <me@vchrist.at>
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

#include "config.h" // just for this example app
#include "http/client/Response.h"
#include "http/client/ResponseParser.h"
#include "log/Logger.h"
#include "net/SNodeC.h"
#include "net/socket/ip/tcp/ipv4/Socket.h"
#include "net/socket/stream/SocketProtocol.h"
#include "net/socket/stream/SocketProtocolFactory.h"
#include "net/socket/stream/legacy/SocketClient.h"
#include "net/socket/stream/tls/SocketClient.h"

#include <openssl/x509v3.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

using namespace net::socket::ip;
using namespace net::socket::ip::address::ipv4;
using namespace net::socket::stream;

static http::client::ResponseParser* getResponseParser() {
    http::client::ResponseParser* responseParser = new http::client::ResponseParser(
        [](void) -> void {
        },
        [](const std::string& httpVersion, const std::string& statusCode, const std::string& reason) -> void {
            VLOG(0) << "++ Response: " << httpVersion << " " << statusCode << " " << reason;
        },
        [](const std::map<std::string, std::string>& headers, const std::map<std::string, http::CookieOptions>& cookies) -> void {
            VLOG(0) << "++   Headers:";
            for (const auto& [field, value] : headers) {
                VLOG(0) << "++       " << field + " = " + value;
            }

            VLOG(0) << "++   Cookies:";
            for (const auto& [name, cookie] : cookies) {
                VLOG(0) << "++     " + name + " = " + cookie.getValue();
                for (const auto& [option, value] : cookie.getOptions()) {
                    VLOG(0) << "++       " + option + " = " + value;
                }
            }
        },
        [](char* content, std::size_t contentLength) -> void {
            char* strContent = new char[contentLength + 1];
            memcpy(strContent, content, contentLength);
            strContent[contentLength] = 0;
            VLOG(0) << "++   OnContent: " << contentLength << std::endl << strContent;
            delete[] strContent;
        },
        [](http::client::ResponseParser& parser) -> void {
            VLOG(0) << "++   OnParsed";
            parser.reset();
        },
        [](int status, const std::string& reason) -> void {
            VLOG(0) << "++   OnError: " + std::to_string(status) + " - " + reason;
        });

    return responseParser;
}

class SimpleSocketProtocol : public SocketProtocol {
public:
    SimpleSocketProtocol() {
        responseParser = getResponseParser();
    }

    ~SimpleSocketProtocol() override {
        delete responseParser;
    }

    void receiveData(const char* junk, std::size_t junkLen) override {
        responseParser->parse(junk, junkLen);
    }

    void onWriteError([[maybe_unused]] int errnum) override {
        VLOG(0) << "OnWriteError: " << errnum;
    }

    void onReadError([[maybe_unused]] int errnum) override {
        VLOG(0) << "OnReadError: " << errnum;
    }

private:
    http::client::ResponseParser* responseParser;
};

class SimpleSocketProtocolFactory : public SocketProtocolFactory {
public:
    SimpleSocketProtocolFactory() = default;

    SocketProtocol* create() const override {
        return new SimpleSocketProtocol();
    }
};

class SocketConnectionBase;

tls::SocketClient<tcp::ipv4::Socket> getTlsClient() {
    tls::SocketClient<tcp::ipv4::Socket> tlsClient(
        new SimpleSocketProtocolFactory(), // SharedFactory
        [](const tls::SocketClient<tcp::ipv4::Socket>::SocketAddress& localAddress,
           const tls::SocketClient<tcp::ipv4::Socket>::SocketAddress& remoteAddress) -> void { // OnConnect
            VLOG(0) << "OnConnect";

            VLOG(0) << "\tServer: " + remoteAddress.toString();
            VLOG(0) << "\tClient: " + localAddress.toString();
        },
        [](tls::SocketClient<tcp::ipv4::Socket>::SocketConnection* socketConnection) -> void { // onConnected
            VLOG(0) << "OnConnected";

            //            socketConnection->setContext<http::client::ResponseParser*>(getResponseParser());

            socketConnection->enqueue("GET /index.html HTTP/1.1\r\nConnection: close\r\n\r\n"); // Connection: close\r\n\r\n");

            X509* server_cert = SSL_get_peer_certificate(socketConnection->getSSL());
            if (server_cert != NULL) {
                long verifyErr = SSL_get_verify_result(socketConnection->getSSL());

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
        [](tls::SocketClient<tcp::ipv4::Socket>::SocketConnection* socketConnection) -> void { // onDisconnect
            VLOG(0) << "OnDisconnect";

            VLOG(0) << "\tServer: " + socketConnection->getRemoteAddress().toString();
            VLOG(0) << "\tClient: " + socketConnection->getLocalAddress().toString();

        },
        [](tls::SocketClient<tcp::ipv4::Socket>::SocketConnection* socketConnection,
           const char* junk,
           std::size_t junkLen) -> void { // onRead
            VLOG(0) << "OnRead";
            static_cast<SimpleSocketProtocol*>(socketConnection->getSocketProtocol())->receiveData(junk, junkLen);

        },
        []([[maybe_unused]] tls::SocketConnection<tcp::ipv4::Socket>* socketConnection,
           int errnum) -> void { // onReadError
            static_cast<SimpleSocketProtocol*>(socketConnection->getSocketProtocol())->onReadError(errnum);
        },
        []([[maybe_unused]] tls::SocketConnection<tcp::ipv4::Socket>* socketConnection,
           int errnum) -> void { // onWriteError
            static_cast<SimpleSocketProtocol*>(socketConnection->getSocketProtocol())->onWriteError(errnum);
        },
        {{"certChain", CLIENTCERTF}, {"keyPEM", CLIENTKEYF}, {"password", KEYFPASS}, {"caFile", SERVERCAFILE}});

    InetAddress remoteAddress("localhost", 8088);

    tlsClient.connect(remoteAddress, [](int err) -> void {
        if (err) {
            PLOG(ERROR) << "Connect: " + std::to_string(err);
        } else {
            VLOG(0) << "Connected";
        }
    });

    return tlsClient;
}

legacy::SocketClient<tcp::ipv4::Socket> getLegacyClient() {
    legacy::SocketClient<tcp::ipv4::Socket> legacyClient(
        new SimpleSocketProtocolFactory(), // SharedFactory
        [](const legacy::SocketClient<tcp::ipv4::Socket>::SocketAddress& localAddress,
           const legacy::SocketClient<tcp::ipv4::Socket>::SocketAddress& remoteAddress) -> void { // OnConnect
            VLOG(0) << "OnConnect";

            VLOG(0) << "\tServer: " + remoteAddress.toString();
            VLOG(0) << "\tClient: " + localAddress.toString();
        },
        [](legacy::SocketClient<tcp::ipv4::Socket>::SocketConnection* socketConnection) -> void { // onConnected
            VLOG(0) << "OnConnected";

            socketConnection->enqueue("GET /index.html HTTP/1.1\r\nConnection: close\r\n\r\n"); // Connection: close\r\n\r\n");
        },
        [](legacy::SocketClient<tcp::ipv4::Socket>::SocketConnection* socketConnection) -> void { // onDisconnect
            VLOG(0) << "OnDisconnect";

            VLOG(0) << "\tServer: " + socketConnection->getRemoteAddress().toString();
            VLOG(0) << "\tClient: " + socketConnection->getLocalAddress().toString();

        },
        [](legacy::SocketClient<tcp::ipv4::Socket>::SocketConnection* socketConnection,
           const char* junk,
           std::size_t junkLen) -> void { // onRead
            VLOG(0) << "OnRead";
            static_cast<SimpleSocketProtocol*>(socketConnection->getSocketProtocol())->receiveData(junk, junkLen);
        },
        []([[maybe_unused]] legacy::SocketClient<tcp::ipv4::Socket>::SocketConnection* socketConnection,
           int errnum) -> void { // onReadError
            static_cast<SimpleSocketProtocol*>(socketConnection->getSocketProtocol())->onReadError(errnum);
        },
        []([[maybe_unused]] legacy::SocketClient<tcp::ipv4::Socket>::SocketConnection* socketConnection,
           int errnum) -> void { // onWriteError
            static_cast<SimpleSocketProtocol*>(socketConnection->getSocketProtocol())->onWriteError(errnum);
        },
        {{}});

    InetAddress remoteAddress("localhost", 8080);

    legacyClient.connect(remoteAddress, [](int err) -> void {
        if (err) {
            PLOG(ERROR) << "Connect: " << std::to_string(err);
        } else {
            VLOG(0) << "Connected";
        }
    });

    return legacyClient;
}

int main(int argc, char* argv[]) {
    net::SNodeC::init(argc, argv);

    {
        InetAddress remoteAddress("localhost", 8080);

        legacy::SocketClient<tcp::ipv4::Socket> legacyClient = getLegacyClient();

        legacyClient.connect(remoteAddress, [](int err) -> void { // example.com:81 simulate connnect timeout
            if (err) {
                PLOG(ERROR) << "Connect: " << std::to_string(err);
            } else {
                VLOG(0) << "Connected";
            }
        });

        remoteAddress = InetAddress("localhost", 8088);

        tls::SocketClient<tcp::ipv4::Socket> tlsClient = getTlsClient();

        tlsClient.connect(remoteAddress, [](int err) -> void {
            if (err) {
                PLOG(ERROR) << "Connect: " << std::to_string(err);
            } else {
                VLOG(0) << "Connected";
            }
        });
    }

    return net::SNodeC::start();
}
