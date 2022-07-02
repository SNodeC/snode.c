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

#include "config.h"                            // for CLIENTCERTF
#include "core/SNodeC.h"                       // for SNodeC
#include "core/socket/SocketContext.h"         // for SocketProtocol
#include "core/socket/SocketContextFactory.h"  // for SocketProtocolF...
#include "log/Logger.h"                        // for Writer, Storage
#include "net/in/stream/legacy/SocketClient.h" // for SocketC...
#include "net/in/stream/tls/SocketClient.h"    // for SocketC...
#include "web/http/client/ResponseParser.h"    // for ResponseParser

#include <openssl/asn1.h>    // for ASN1_STRING_get...
#include <openssl/crypto.h>  // for OPENSSL_free
#include <openssl/obj_mac.h> // for NID_subject_alt...
#include <openssl/opensslv.h>
#include <openssl/ssl.h>    // IWYU pragma: keep
#include <openssl/x509.h>   // for X509_NAME_oneline
#include <openssl/x509v3.h> // for GENERAL_NAME
#include <type_traits>      // for add_const<>::type
#include <utility>          // for tuple_element<>...

// IWYU pragma: no_include <openssl/ssl3.h>

#if OPENSSL_VERSION_NUMBER >= 0x30000000L
#include <openssl/types.h>
#elif OPENSSL_VERSION_NUMBER >= 0x10100000L
#include <openssl/ossl_typ.h>
#endif

namespace web::http {
    class CookieOptions;
}

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

static web::http::client::ResponseParser* getResponseParser(core::socket::SocketContext* socketContext) {
    web::http::client::ResponseParser* responseParser = new web::http::client::ResponseParser(
        socketContext,
        [](void) -> void {
        },
        [](const std::string& httpVersion, const std::string& statusCode, const std::string& reason) -> void {
            VLOG(0) << "++ Response: " << httpVersion << " " << statusCode << " " << reason;
        },
        [](const std::map<std::string, std::string>& headers, const std::map<std::string, web::http::CookieOptions>& cookies) -> void {
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
        [](std::vector<uint8_t> content) -> void {
            content.push_back(0);

            VLOG(0) << "++   OnContent: "; // << content.data();
        },
        [](web::http::client::ResponseParser& parser) -> void {
            VLOG(0) << "++   OnParsed";
            parser.reset();
        },
        [](int status, const std::string& reason) -> void {
            VLOG(0) << "++   OnError: " + std::to_string(status) + " - " + reason;
        });

    return responseParser;
}

class SimpleSocketProtocol : public core::socket::SocketContext {
public:
    explicit SimpleSocketProtocol(core::socket::SocketConnection* socketConnection)
        : core::socket::SocketContext(socketConnection) {
        responseParser = getResponseParser(this);
    }

    ~SimpleSocketProtocol() override {
        delete responseParser;
    }

    void onConnected() override {
        VLOG(0) << "SimpleSocketProtocol connected";
    }
    void onDisconnected() override {
        VLOG(0) << "SimpleSocketProtocol disconnected";
    }

    std::size_t onReceiveFromPeer() override {
        return responseParser->parse();
    }

    void onWriteError(int errnum) override {
        VLOG(0) << "OnWriteError: " << errnum;
        shutdownRead();
    }

    void onReadError(int errnum) override {
        VLOG(0) << "OnReadError: " << errnum;
        shutdownWrite();
    }

private:
    web::http::client::ResponseParser* responseParser;
};

class SimpleSocketProtocolFactory : public core::socket::SocketContextFactory {
private:
    core::socket::SocketContext* create(core::socket::SocketConnection* socketConnection) override {
        return new SimpleSocketProtocol(socketConnection);
    }
};

namespace tls {

    using SocketClient = net::in::stream::tls::SocketClient<SimpleSocketProtocolFactory>;
    using SocketAddress = SocketClient::SocketAddress;
    using SocketConnection = SocketClient::SocketConnection;

    SocketClient getClient() {
        SocketClient client(
            "tls",
            [](SocketConnection* socketConnection) -> void { // onConnect
                VLOG(0) << "OnConnect";

                VLOG(0) << "\tServer: (" + socketConnection->getRemoteAddress().address() + ") " +
                               socketConnection->getRemoteAddress().toString();
                VLOG(0) << "\tClient: (" + socketConnection->getLocalAddress().address() + ") " +
                               socketConnection->getLocalAddress().toString();
            },
            [](SocketConnection* socketConnection) -> void { // onConnected
                VLOG(0) << "OnConnected";

                X509* server_cert = SSL_get_peer_certificate(socketConnection->getSSL());
                if (server_cert != nullptr) {
                    long verifyErr = SSL_get_verify_result(socketConnection->getSSL());

                    VLOG(0) << "     Server certificate: " + std::string(X509_verify_cert_error_string(verifyErr));

                    char* str = X509_NAME_oneline(X509_get_subject_name(server_cert), nullptr, 0);
                    VLOG(0) << "        Subject: " + std::string(str);
                    OPENSSL_free(str);

                    str = X509_NAME_oneline(X509_get_issuer_name(server_cert), nullptr, 0);
                    VLOG(0) << "        Issuer: " + std::string(str);
                    OPENSSL_free(str);

                    // We could do all sorts of certificate verification stuff here before deallocating the certificate.

                    GENERAL_NAMES* subjectAltNames =
                        static_cast<GENERAL_NAMES*>(X509_get_ext_d2i(server_cert, NID_subject_alt_name, nullptr, nullptr));

                    int32_t altNameCount = sk_GENERAL_NAME_num(subjectAltNames);
                    VLOG(0) << "        Subject alternative name count: " << altNameCount;
                    for (int32_t i = 0; i < altNameCount; ++i) {
                        GENERAL_NAME* generalName = sk_GENERAL_NAME_value(subjectAltNames, i);
                        if (generalName->type == GEN_URI) {
                            std::string subjectAltName =
                                std::string(reinterpret_cast<const char*>(ASN1_STRING_get0_data(generalName->d.uniformResourceIdentifier)),
                                            static_cast<std::size_t>(ASN1_STRING_length(generalName->d.uniformResourceIdentifier)));
                            VLOG(0) << "           SAN (URI): '" + subjectAltName;
                        } else if (generalName->type == GEN_DNS) {
                            std::string subjectAltName =
                                std::string(reinterpret_cast<const char*>(ASN1_STRING_get0_data(generalName->d.dNSName)),
                                            static_cast<std::size_t>(ASN1_STRING_length(generalName->d.dNSName)));
                            VLOG(0) << "           SAN (DNS): '" + subjectAltName;
                        } else {
                            VLOG(0) << "           SAN (Type): '" + std::to_string(generalName->type);
                        }
                    }
                    sk_GENERAL_NAME_pop_free(subjectAltNames, GENERAL_NAME_free);

                    X509_free(server_cert);
                } else {
                    VLOG(0) << "     Server certificate: no certificate";
                }

                socketConnection->sendToPeer("GET /index.html HTTP/1.1\r\nConnection: close\r\n\r\n"); // Connection: close\r\n\r\n");
            },
            [](SocketConnection* socketConnection) -> void { // onDisconnect
                VLOG(0) << "OnDisconnect";

                VLOG(0) << "\tServer: " + socketConnection->getRemoteAddress().toString();
                VLOG(0) << "\tClient: " + socketConnection->getLocalAddress().toString();

            },
            {{"certChain", CLIENTCERTF}, {"keyPEM", CLIENTKEYF}, {"password", KEYFPASS}, {"caFile", SERVERCAFILE}});

        SocketAddress remoteAddress("localhost", 8088);

        client.connect(remoteAddress, [](const SocketAddress& socketAddress, int err) -> void {
            if (err) {
                PLOG(ERROR) << "Connect: " + std::to_string(err);
            } else {
                VLOG(0) << "Connecting to " << socketAddress.toString();
            }
        });

        return client;
    }

} // namespace tls

namespace legacy {

    using SocketClient = net::in::stream::legacy::SocketClient<SimpleSocketProtocolFactory>;
    using SocketAddress = SocketClient::SocketAddress;
    using SocketConnection = SocketClient::SocketConnection;

    SocketClient getLegacyClient() {
        SocketClient legacyClient(
            "legacy",
            [](SocketConnection* socketConnection) -> void { // OnConnect
                VLOG(0) << "OnConnect";

                VLOG(0) << "\tServer: (" + socketConnection->getRemoteAddress().address() + ") " +
                               socketConnection->getRemoteAddress().toString();
                VLOG(0) << "\tClient: (" + socketConnection->getLocalAddress().address() + ") " +
                               socketConnection->getLocalAddress().toString();
            },
            [](SocketConnection* socketConnection) -> void { // onConnected
                VLOG(0) << "OnConnected";

                socketConnection->sendToPeer("GET /index.html HTTP/1.1\r\nConnection: close\r\n\r\n"); // Connection: close\r\n\r\n");
            },
            [](SocketConnection* socketConnection) -> void { // onDisconnect
                VLOG(0) << "OnDisconnect";

                VLOG(0) << "\tServer: (" + socketConnection->getRemoteAddress().address() + ") " +
                               socketConnection->getRemoteAddress().toString();
                VLOG(0) << "\tClient: (" + socketConnection->getLocalAddress().address() + ") " +
                               socketConnection->getLocalAddress().toString();
            },
            {{}});

        SocketAddress remoteAddress("localhost", 8080);

        legacyClient.connect(remoteAddress, [](const SocketAddress& socketAddress, int err) -> void {
            if (err) {
                PLOG(ERROR) << "Connect: " << std::to_string(err);
            } else {
                VLOG(0) << "Connecting to " << socketAddress.toString();
            }
        });

        return legacyClient;
    }

} // namespace legacy

int main(int argc, char* argv[]) {
    core::SNodeC::init(argc, argv);

    {
        legacy::SocketAddress legacyRemoteAddress("localhost", 8080);

        legacy::SocketClient legacyClient = legacy::getLegacyClient();

        legacyClient.connect(legacyRemoteAddress,
                             [](const tls::SocketAddress& socketAddress, int err) -> void { // example.com:81 simulate connnect timeout
                                 if (err) {
                                     PLOG(ERROR) << "Connect: " << std::to_string(err);
                                 } else {
                                     VLOG(0) << "Connecting to " << socketAddress.toString();
                                 }
                             });

        tls::SocketAddress tlsRemoteAddress = tls::SocketAddress("localhost", 8088);

        tls::SocketClient tlsClient = tls::getClient();

        tlsClient.connect(tlsRemoteAddress, [](const tls::SocketAddress& socketAddress, int err) -> void {
            if (err) {
                PLOG(ERROR) << "Connect: " << std::to_string(err);
            } else {
                VLOG(0) << "Connecting to " << socketAddress.toString();
            }
        });
    }

    return core::SNodeC::start();
}
