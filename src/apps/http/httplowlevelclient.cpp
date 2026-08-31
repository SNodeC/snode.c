#include <SemanticLog.h>
/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021, 2022, 2023 Volker Christian <me@vchrist.at>
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

#include "core/SNodeC.h"
#include "core/socket/stream/SocketContext.h"
#include "core/socket/stream/SocketContextFactory.h"
#include "log/Logger.h"
#include "net/in/stream/legacy/SocketClient.h"
#include "net/in/stream/tls/SocketClient.h"
#include "web/http/CookieOptions.h"
#include "web/http/client/ResponseParser.h"

#include <map>
#include <openssl/ssl.h> // IWYU pragma: keep
#include <openssl/x509v3.h>
#include <string>
#include <utility>

// IWYU pragma: no_include <openssl/ssl3.h>
// IWYU pragma: no_include <bits/utility.h>
// IWYU pragma: no_include <openssl/ssl3.h>
// IWYU pragma: no_include <openssl/x509.h>
// IWYU pragma: no_include <openssl/types.h>
// IWYU pragma: no_include <openssl/asn1.h>
// IWYU pragma: no_include <openssl/obj_mac.h>
// IWYU pragma: no_include <openssl/crypto.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

static web::http::client::ResponseParser* getResponseParser(core::socket::stream::SocketContext* socketContext) {
    web::http::client::ResponseParser* responseParser = new web::http::client::ResponseParser(
        socketContext,
        [](void) -> void {
        },
        [](const std::string& httpVersion, const std::string& statusCode, const std::string& reason) -> void {
            snode::semantic::appLog().trace() << "++ Response: " << httpVersion << " " << statusCode << " " << reason;
        },
        [](std::map<std::string, std::string>& headers, std::map<std::string, web::http::CookieOptions>& cookies) -> void {
            snode::semantic::appLog().trace() << "++   Headers:";
            for (auto& [field, value] : headers) {
                snode::semantic::appLog().trace() << "++       " << field + " = " + value;
            }

            snode::semantic::appLog().trace() << "++   Cookies:";
            for (auto& [name, cookie] : cookies) {
                snode::semantic::appLog().trace() << "++     " + name + " = " + cookie.getValue();
                for (auto& [option, value] : cookie.getOptions()) {
                    snode::semantic::appLog().trace() << "++       " + option + " = " + value;
                }
            }
        },
        [](std::vector<uint8_t> content) -> void {
            content.push_back(0);

            snode::semantic::appLog().trace() << "++   OnContent: "; // << content.data();
        },
        [](web::http::client::ResponseParser& parser) -> void {
            snode::semantic::appLog().trace() << "++   OnParsed";
            parser.reset();
        },
        [](int status, const std::string& reason) -> void {
            snode::semantic::appLog().trace() << "++   OnError: " + std::to_string(status) + " - " + reason;
        });

    return responseParser;
}

class SimpleSocketProtocol : public core::socket::stream::SocketContext {
public:
    explicit SimpleSocketProtocol(core::socket::stream::SocketConnection* socketConnection)
        : core::socket::stream::SocketContext(socketConnection) {
        responseParser = getResponseParser(this);
    }

    ~SimpleSocketProtocol() override;

    void onConnected() override {
        snode::semantic::appLog().trace() << "SimpleSocketProtocol connected";
    }
    void onDisconnected() override {
        snode::semantic::appLog().trace() << "SimpleSocketProtocol disconnected";
    }

    std::size_t onReceivedFromPeer() override {
        return responseParser->parse();
    }

    void onWriteError(int errnum) override {
        snode::semantic::appLog().trace() << "OnWriteError: " << errnum;
        shutdownRead();
    }

    void onReadError(int errnum) override {
        snode::semantic::appLog().trace() << "OnReadError: " << errnum;
        shutdownWrite();
    }

private:
    web::http::client::ResponseParser* responseParser;
};

SimpleSocketProtocol::~SimpleSocketProtocol() {
    delete responseParser;
}

class SimpleSocketProtocolFactory : public core::socket::stream::SocketContextFactory {
public:
    ~SimpleSocketProtocolFactory() override;

private:
    core::socket::stream::SocketContext* create(core::socket::stream::SocketConnection* socketConnection) override {
        return new SimpleSocketProtocol(socketConnection);
    }
};

SimpleSocketProtocolFactory::~SimpleSocketProtocolFactory() {
}

namespace tls {

    using SocketClient = net::in::stream::tls::SocketClient<SimpleSocketProtocolFactory>;
    using SocketAddress = SocketClient::SocketAddress;
    using SocketConnection = SocketClient::SocketConnection;

    SocketClient getClient() {
        SocketClient client(
            "tls",
            [](SocketConnection* socketConnection) -> void { // onConnect
                snode::semantic::appLog().trace() << "OnConnect";

                snode::semantic::appLog().trace() << "\tServer: (" + socketConnection->getRemoteAddress().address() + ") " +
                               socketConnection->getRemoteAddress().toString();
                snode::semantic::appLog().trace() << "\tClient: (" + socketConnection->getLocalAddress().address() + ") " +
                               socketConnection->getLocalAddress().toString();

                /* Enable automatic hostname checks */
                // X509_VERIFY_PARAM* param = SSL_get0_param(socketConnection->getSSL());

                // X509_VERIFY_PARAM_set_hostflags(param, X509_CHECK_FLAG_NO_PARTIAL_WILDCARDS);
                // if (!X509_VERIFY_PARAM_set1_host(param, "localhost", sizeof("localhost") - 1)) {
                //   // handle error
                //   socketConnection->close();
                // }
            },
            [](SocketConnection* socketConnection) -> void { // onConnected
                snode::semantic::appLog().trace() << "OnConnected";

                X509* server_cert = SSL_get_peer_certificate(socketConnection->getSSL());
                if (server_cert != nullptr) {
                    long verifyErr = SSL_get_verify_result(socketConnection->getSSL());

                    snode::semantic::appLog().trace() << "     Server certificate: " + std::string(X509_verify_cert_error_string(verifyErr));

                    char* str = X509_NAME_oneline(X509_get_subject_name(server_cert), nullptr, 0);
                    snode::semantic::appLog().trace() << "        Subject: " + std::string(str);
                    OPENSSL_free(str);

                    str = X509_NAME_oneline(X509_get_issuer_name(server_cert), nullptr, 0);
                    snode::semantic::appLog().trace() << "        Issuer: " + std::string(str);
                    OPENSSL_free(str);

                    // We could do all sorts of certificate verification stuff here before deallocating the certificate.

                    GENERAL_NAMES* subjectAltNames =
                        static_cast<GENERAL_NAMES*>(X509_get_ext_d2i(server_cert, NID_subject_alt_name, nullptr, nullptr));
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wused-but-marked-unused"
#endif
                    int32_t altNameCount = sk_GENERAL_NAME_num(subjectAltNames);
#ifdef __clang__
#pragma clang diagnostic pop
#endif
                    snode::semantic::appLog().trace() << "        Subject alternative name count: " << altNameCount;
                    for (int32_t i = 0; i < altNameCount; ++i) {
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wused-but-marked-unused"
#endif
                        GENERAL_NAME* generalName = sk_GENERAL_NAME_value(subjectAltNames, i);
#ifdef __clang__
#pragma clang diagnostic pop
#endif
                        if (generalName->type == GEN_URI) {
                            std::string subjectAltName =
                                std::string(reinterpret_cast<const char*>(ASN1_STRING_get0_data(generalName->d.uniformResourceIdentifier)),
                                            static_cast<std::size_t>(ASN1_STRING_length(generalName->d.uniformResourceIdentifier)));
                            snode::semantic::appLog().trace() << "           SAN (URI): '" + subjectAltName;
                        } else if (generalName->type == GEN_DNS) {
                            std::string subjectAltName =
                                std::string(reinterpret_cast<const char*>(ASN1_STRING_get0_data(generalName->d.dNSName)),
                                            static_cast<std::size_t>(ASN1_STRING_length(generalName->d.dNSName)));
                            snode::semantic::appLog().trace() << "           SAN (DNS): '" + subjectAltName;
                        } else {
                            snode::semantic::appLog().trace() << "           SAN (Type): '" + std::to_string(generalName->type);
                        }
                    }
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wused-but-marked-unused"
#endif
                    sk_GENERAL_NAME_pop_free(subjectAltNames, GENERAL_NAME_free);
#ifdef __clang__
#pragma clang diagnostic pop
#endif
                    X509_free(server_cert);
                } else {
                    snode::semantic::appLog().trace() << "     Server certificate: no certificate";
                }

                socketConnection->sendToPeer("GET /index.html HTTP/1.1\r\nConnection: close\r\n\r\n"); // Connection: close\r\n\r\n");
            },
            [](SocketConnection* socketConnection) -> void { // onDisconnect
                snode::semantic::appLog().trace() << "OnDisconnect";

                snode::semantic::appLog().trace() << "\tServer: " + socketConnection->getRemoteAddress().toString();
                snode::semantic::appLog().trace() << "\tClient: " + socketConnection->getLocalAddress().toString();

            });

        SocketAddress remoteAddress("localhost", 8088);

        client.connect(remoteAddress, [](const SocketAddress& socketAddress, int err) -> void {
            if (err) {
                snode::semantic::sysError(snode::semantic::appLog(), logger::LogLevel::Error, errno) << "Connect: " + std::to_string(err);
            } else {
                snode::semantic::appLog().trace() << "Connecting to " << socketAddress.toString();
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
                snode::semantic::appLog().trace() << "OnConnect";

                snode::semantic::appLog().trace() << "\tServer: (" + socketConnection->getRemoteAddress().address() + ") " +
                               socketConnection->getRemoteAddress().toString();
                snode::semantic::appLog().trace() << "\tClient: (" + socketConnection->getLocalAddress().address() + ") " +
                               socketConnection->getLocalAddress().toString();
            },
            [](SocketConnection* socketConnection) -> void { // onConnected
                snode::semantic::appLog().trace() << "OnConnected";

                socketConnection->sendToPeer("GET /index.html HTTP/1.1\r\nConnection: close\r\n\r\n"); // Connection: close\r\n\r\n");
            },
            [](SocketConnection* socketConnection) -> void { // onDisconnect
                snode::semantic::appLog().trace() << "OnDisconnect";

                snode::semantic::appLog().trace() << "\tServer: (" + socketConnection->getRemoteAddress().address() + ") " +
                               socketConnection->getRemoteAddress().toString();
                snode::semantic::appLog().trace() << "\tClient: (" + socketConnection->getLocalAddress().address() + ") " +
                               socketConnection->getLocalAddress().toString();
            });

        SocketAddress remoteAddress("localhost", 8080);

        legacyClient.connect(remoteAddress, [](const SocketAddress& socketAddress, int err) -> void {
            if (err) {
                snode::semantic::sysError(snode::semantic::appLog(), logger::LogLevel::Error, errno) << "Connect: " << std::to_string(err);
            } else {
                snode::semantic::appLog().trace() << "Connecting to " << socketAddress.toString();
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
                             [](const tls::SocketAddress& socketAddress, int errnum) -> void { // example.com:81 simulate connnect timeout
                                 if (errnum < 0) {
                                     snode::semantic::sysError(snode::semantic::appLog(), logger::LogLevel::Error, errno) << "OnError";
                                 } else if (errnum > 0) {
                                     snode::semantic::sysError(snode::semantic::appLog(), logger::LogLevel::Error, errno) << "OnError: " << socketAddress.toString();
                                 } else {
                                     snode::semantic::appLog().trace() << "snode.c connecting to " << socketAddress.toString();
                                 }
                             });

        tls::SocketAddress tlsRemoteAddress = tls::SocketAddress("localhost", 8088);

        tls::SocketClient tlsClient = tls::getClient();

        tlsClient.connect(tlsRemoteAddress, [](const tls::SocketAddress& socketAddress, int errnum) -> void {
            if (errnum < 0) {
                snode::semantic::sysError(snode::semantic::appLog(), logger::LogLevel::Error, errno) << "OnError";
            } else if (errnum > 0) {
                snode::semantic::sysError(snode::semantic::appLog(), logger::LogLevel::Error, errno) << "OnError: " << socketAddress.toString();
            } else {
                snode::semantic::appLog().trace() << "snode.c connecting to " << socketAddress.toString();
            }
        });
    }

    return core::SNodeC::start();
}
