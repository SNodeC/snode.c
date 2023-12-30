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
#include "web/http/client/ResponseParser.h"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <openssl/ssl.h> // IWYU pragma: keep
#include <openssl/x509v3.h>
#include <string>
#include <vector>

namespace web::http {
    class CookieOptions;
}

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
        []() -> void {
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

            VLOG(0) << "++   OnContent: " << content.data();
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

class SimpleSocketProtocol : public core::socket::stream::SocketContext {
public:
    explicit SimpleSocketProtocol(core::socket::stream::SocketConnection* socketConnection)
        : core::socket::stream::SocketContext(socketConnection) {
        responseParser = getResponseParser(this);
    }

    ~SimpleSocketProtocol() override;

    void onConnected() override {
        VLOG(0) << "SimpleSocketProtocol connected";
    }
    void onDisconnected() override {
        VLOG(0) << "SimpleSocketProtocol disconnected";
    }

    std::size_t onReceivedFromPeer() override {
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
        SocketClient tlsClient(
            "tls",
            [](SocketConnection* socketConnection) -> void { // onConnect
                VLOG(0) << "OnConnect";

                VLOG(0) << "\tServer: " << socketConnection->getRemoteAddress().toString();
                VLOG(0) << "\tClient: " << socketConnection->getLocalAddress().toString();

                /* Enable automatic hostname checks */
                // X509_VERIFY_PARAM* param = SSL_get0_param(socketConnection->getSSL());

                // X509_VERIFY_PARAM_set_hostflags(param, X509_CHECK_FLAG_NO_PARTIAL_WILDCARDS);
                // if (!X509_VERIFY_PARAM_set1_host(param, "localhost", sizeof("localhost") - 1)) {
                //   // handle error
                //   socketConnection->close();
                // }
            },
            [](SocketConnection* socketConnection) -> void { // onConnected
                VLOG(0) << "OnConnected";

                X509* server_cert = SSL_get_peer_certificate(socketConnection->getSSL());
                if (server_cert != nullptr) {
                    const long verifyErr = SSL_get_verify_result(socketConnection->getSSL());

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
#ifdef __GNUC__
#pragma GCC diagnostic push
#ifdef __has_warning
#if __has_warning("-Wused-but-marked-unused")
#pragma GCC diagnostic ignored "-Wused-but-marked-unused"
#endif
#endif
#endif
                    const int32_t altNameCount = sk_GENERAL_NAME_num(subjectAltNames);
#ifdef __GNUC_
#pragma GCC diagnostic pop
#endif
                    VLOG(0) << "\t   Subject alternative name count: " << altNameCount;
                    for (int32_t i = 0; i < altNameCount; ++i) {
#ifdef __GNUC__
#pragma GCC diagnostic push
#ifdef __has_warning
#if __has_warning("-Wused-but-marked-unused")
#pragma GCC diagnostic ignored "-Wused-but-marked-unused"
#endif
#endif
#endif
                        GENERAL_NAME* generalName = sk_GENERAL_NAME_value(subjectAltNames, i);
#ifdef __GNUC_
#pragma GCC diagnostic pop
#endif
                        if (generalName->type == GEN_URI) {
                            const std::string subjectAltName =
                                std::string(reinterpret_cast<const char*>(ASN1_STRING_get0_data(generalName->d.uniformResourceIdentifier)),
                                            static_cast<std::size_t>(ASN1_STRING_length(generalName->d.uniformResourceIdentifier)));
                            VLOG(0) << "\t      SAN (URI): '" + subjectAltName;
                        } else if (generalName->type == GEN_DNS) {
                            const std::string subjectAltName =
                                std::string(reinterpret_cast<const char*>(ASN1_STRING_get0_data(generalName->d.dNSName)),
                                            static_cast<std::size_t>(ASN1_STRING_length(generalName->d.dNSName)));
                            VLOG(0) << "\t      SAN (DNS): '" + subjectAltName;
                        } else {
                            VLOG(0) << "\t      SAN (Type): '" + std::to_string(generalName->type);
                        }
                    }
#ifdef __GNUC__
#pragma GCC diagnostic push
#ifdef __has_warning
#if __has_warning("-Wused-but-marked-unused")
#pragma GCC diagnostic ignored "-Wused-but-marked-unused"
#endif
#endif
#endif
                    sk_GENERAL_NAME_pop_free(subjectAltNames, GENERAL_NAME_free);
#ifdef __GNUC_
#pragma GCC diagnostic pop
#endif
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

            });

        const SocketAddress remoteAddress("localhost", 8088);

        tlsClient.connect(remoteAddress,
                          [instanceName = tlsClient.getConfig().getInstanceName()](
                              const SocketAddress& socketAddress,
                              const core::socket::State& state) -> void { // example.com:81 simulate connnect timeout
                              switch (state) {
                                  case core::socket::State::OK:
                                      VLOG(1) << instanceName << ": connected to '" << socketAddress.toString() << "'";
                                      break;
                                  case core::socket::State::DISABLED:
                                      VLOG(1) << instanceName << ": disabled";
                                      break;
                                  case core::socket::State::ERROR:
                                      LOG(ERROR) << instanceName << ": " << socketAddress.toString() << ": " << state.what();
                                      break;
                                  case core::socket::State::FATAL:
                                      LOG(FATAL) << instanceName << ": " << socketAddress.toString() << ": " << state.what();
                                      break;
                              }
                          });
        return tlsClient;
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

                VLOG(0) << "\tServer: " << socketConnection->getRemoteAddress().toString();
                VLOG(0) << "\tClient: " << socketConnection->getLocalAddress().toString();
            },
            [](SocketConnection* socketConnection) -> void { // onConnected
                VLOG(0) << "OnConnected";

                socketConnection->sendToPeer("GET /index.html HTTP/1.1\r\nConnection: close\r\n\r\n"); // Connection: close\r\n\r\n");
            },
            [](SocketConnection* socketConnection) -> void { // onDisconnect
                VLOG(0) << "OnDisconnect";

                VLOG(0) << "\tServer: " << socketConnection->getRemoteAddress().toString();
                VLOG(0) << "\tClient: " << socketConnection->getLocalAddress().toString();
            });

        const SocketAddress remoteAddress("localhost", 8080);

        legacyClient.connect(remoteAddress,
                             [instanceName = legacyClient.getConfig().getInstanceName()](
                                 const tls::SocketAddress& socketAddress,
                                 const core::socket::State& state) -> void { // example.com:81 simulate connnect timeout
                                 switch (state) {
                                     case core::socket::State::OK:
                                         VLOG(1) << instanceName << ": connected to '" << socketAddress.toString() << "'";
                                         break;
                                     case core::socket::State::DISABLED:
                                         VLOG(1) << instanceName << ": disabled";
                                         break;
                                     case core::socket::State::ERROR:
                                         LOG(ERROR) << instanceName << ": " << socketAddress.toString() << ": " << state.what();
                                         break;
                                     case core::socket::State::FATAL:
                                         LOG(FATAL) << instanceName << ": " << socketAddress.toString() << ": " << state.what();
                                         break;
                                 }
                             });

        return legacyClient;
    }

} // namespace legacy

int main(int argc, char* argv[]) {
    core::SNodeC::init(argc, argv);

    {
        const legacy::SocketAddress legacyRemoteAddress("localhost", 8080);

        const legacy::SocketClient legacyClient = legacy::getLegacyClient();

        legacyClient.connect(legacyRemoteAddress,
                             [instanceName = legacyClient.getConfig().getInstanceName()](
                                 const tls::SocketAddress& socketAddress,
                                 const core::socket::State& state) -> void { // example.com:81 simulate connnect timeout
                                 switch (state) {
                                     case core::socket::State::OK:
                                         VLOG(1) << instanceName << ": connected to '" << socketAddress.toString() << "'";
                                         break;
                                     case core::socket::State::DISABLED:
                                         VLOG(1) << instanceName << ": disabled";
                                         break;
                                     case core::socket::State::ERROR:
                                         LOG(ERROR) << instanceName << ": " << socketAddress.toString() << ": " << state.what();
                                         break;
                                     case core::socket::State::FATAL:
                                         LOG(FATAL) << instanceName << ": " << socketAddress.toString() << ": " << state.what();
                                         break;
                                 }
                             });

        const tls::SocketAddress tlsRemoteAddress = tls::SocketAddress("localhost", 8088);

        const tls::SocketClient tlsClient = tls::getClient();

        tlsClient.connect(tlsRemoteAddress,
                          [instanceName = tlsClient.getConfig().getInstanceName()](
                              const tls::SocketAddress& socketAddress,
                              const core::socket::State& state) -> void { // example.com:81 simulate connnect timeout
                              switch (state) {
                                  case core::socket::State::OK:
                                      VLOG(1) << instanceName << ": connected to '" << socketAddress.toString() << "'";
                                      break;
                                  case core::socket::State::DISABLED:
                                      VLOG(1) << instanceName << ": disabled";
                                      break;
                                  case core::socket::State::ERROR:
                                      LOG(ERROR) << instanceName << ": " << socketAddress.toString() << ": " << state.what();
                                      break;
                                  case core::socket::State::FATAL:
                                      LOG(FATAL) << instanceName << ": " << socketAddress.toString() << ": " << state.what();
                                      break;
                              }
                          });
    }

    return core::SNodeC::start();
}
