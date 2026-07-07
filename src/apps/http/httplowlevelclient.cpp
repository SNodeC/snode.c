/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
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

/*
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "SemanticLog.h"
#include "core/SNodeC.h"
#include "core/socket/stream/SocketContext.h"
#include "core/socket/stream/SocketContextFactory.h"
#include "net/in/stream/legacy/SocketClient.h"
#include "net/in/stream/tls/SocketClient.h"
#include "web/http/client/ResponseParser.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#include <cstddef>
#include <openssl/ssl.h>
#include <openssl/x509v3.h>

// IWYU pragma: no_include <openssl/ssl3.h>
// IWYU pragma: no_include <openssl/x509.h>
// IWYU pragma: no_include <openssl/types.h>
// IWYU pragma: no_include <openssl/asn1.h>
// IWYU pragma: no_include <openssl/obj_mac.h>
// IWYU pragma: no_include <openssl/crypto.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace apps::http {

    static web::http::client::ResponseParser* getResponseParser(core::socket::stream::SocketContext* socketContext) {
        web::http::client::ResponseParser* responseParser = new web::http::client::ResponseParser(
            socketContext,
            []() {
                snode::semantic::appLog().debug() << "++   OnStarted";
            },
            []([[maybe_unused]] web::http::client::Response& res) {
                snode::semantic::appLog().debug() << "++   OnParsed";
            },
            [](int status, const std::string& reason) {
                snode::semantic::appLog().debug() << "++   OnError: " + std::to_string(status) + " - " + reason;
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
            snode::semantic::appLog().debug() << "SimpleSocketProtocol connected";
        }
        void onDisconnected() override {
            snode::semantic::appLog().debug() << "SimpleSocketProtocol disconnected";
        }

        bool onSignal([[maybe_unused]] int signum) override {
            return true;
        }

        std::size_t onReceivedFromPeer() override {
            return responseParser->parse();
        }

        void onWriteError(int errnum) override {
            core::socket::stream::SocketContext::onWriteError(errnum);
            shutdownRead();
        }

        void onReadError(int errnum) override {
            core::socket::stream::SocketContext::onReadError(errnum);
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

} // namespace apps::http

namespace tls {

    using SocketClient = net::in::stream::tls::SocketClient<apps::http::SimpleSocketProtocolFactory>;
    using SocketAddress = SocketClient::SocketAddress;
    using SocketConnection = SocketClient::SocketConnection;

    SocketClient getClient() {
        SocketClient tlsClient(
            "tls",
            [](SocketConnection* socketConnection) { // onConnect
                snode::semantic::appLog().debug() << "OnConnect";

                snode::semantic::appLog().debug() << "\tServer: " << socketConnection->getRemoteAddress().toString();
                snode::semantic::appLog().debug() << "\tClient: " << socketConnection->getLocalAddress().toString();

                /* Enable automatic hostname checks */
                // X509_VERIFY_PARAM* param = SSL_get0_param(socketConnection->getSSL());

                // X509_VERIFY_PARAM_set_hostflags(param, X509_CHECK_FLAG_NO_PARTIAL_WILDCARDS);
                // if (!X509_VERIFY_PARAM_set1_host(param, "localhost", sizeof("localhost") - 1)) {
                //   // handle error
                //   socketConnection->close();
                // }
            },
            [](SocketConnection* socketConnection) { // onConnected
                snode::semantic::appLog().debug() << "OnConnected";

                X509* server_cert = SSL_get_peer_certificate(socketConnection->getSSL());
                if (server_cert != nullptr) {
                    const long verifyErr = SSL_get_verify_result(socketConnection->getSSL());

                    snode::semantic::appLog().debug()
                        << "     Server certificate: " + std::string(X509_verify_cert_error_string(verifyErr));

                    char* str = X509_NAME_oneline(X509_get_subject_name(server_cert), nullptr, 0);
                    snode::semantic::appLog().debug() << "        Subject: " + std::string(str);
                    OPENSSL_free(str);

                    str = X509_NAME_oneline(X509_get_issuer_name(server_cert), nullptr, 0);
                    snode::semantic::appLog().debug() << "        Issuer: " + std::string(str);
                    OPENSSL_free(str);

                    // We could do all sorts of certificate verification stuff here before deallocating the certificate.

                    GENERAL_NAMES* subjectAltNames =
                        static_cast<GENERAL_NAMES*>(X509_get_ext_d2i(server_cert, NID_subject_alt_name, nullptr, nullptr));

                    const int32_t altNameCount = sk_GENERAL_NAME_num(subjectAltNames);

                    snode::semantic::appLog().debug() << "\t   Subject alternative name count: " << altNameCount;
                    for (int32_t i = 0; i < altNameCount; ++i) {
                        GENERAL_NAME* generalName = sk_GENERAL_NAME_value(subjectAltNames, i);
                        if (generalName->type == GEN_URI) {
                            const std::string subjectAltName =
                                std::string(reinterpret_cast<const char*>(ASN1_STRING_get0_data(generalName->d.uniformResourceIdentifier)),
                                            static_cast<std::size_t>(ASN1_STRING_length(generalName->d.uniformResourceIdentifier)));
                            snode::semantic::appLog().debug() << "\t      SAN (URI): '" + subjectAltName;
                        } else if (generalName->type == GEN_DNS) {
                            const std::string subjectAltName =
                                std::string(reinterpret_cast<const char*>(ASN1_STRING_get0_data(generalName->d.dNSName)),
                                            static_cast<std::size_t>(ASN1_STRING_length(generalName->d.dNSName)));
                            snode::semantic::appLog().debug() << "\t      SAN (DNS): '" + subjectAltName;
                        } else {
                            snode::semantic::appLog().debug() << "\t      SAN (Type): '" + std::to_string(generalName->type);
                        }
                    }

                    sk_GENERAL_NAME_pop_free(subjectAltNames, GENERAL_NAME_free);

                    X509_free(server_cert);
                } else {
                    snode::semantic::appLog().debug() << "     Server certificate: no certificate";
                }

                socketConnection->sendToPeer("GET /index.html HTTP/1.1\r\nConnection: close\r\n\r\n"); // Connection: close\r\n\r\n");
            },
            [](SocketConnection* socketConnection) { // onDisconnect
                snode::semantic::appLog().debug() << "OnDisconnect";

                snode::semantic::appLog().debug() << "\tServer: " + socketConnection->getRemoteAddress().toString();
                snode::semantic::appLog().debug() << "\tClient: " + socketConnection->getLocalAddress().toString();

            });

        const SocketAddress remoteAddress("localhost", 8088);

        tlsClient.connect(
            remoteAddress,
            [instanceName =
                 tlsClient.getConfig()->getInstanceName()](const SocketAddress& socketAddress,
                                                           const core::socket::State& state) { // example.com:81 simulate connnect timeout
                switch (state) {
                    case core::socket::State::OK:
                        snode::semantic::appLog().info() << instanceName << ": connected to '" << socketAddress.toString() << "'";
                        break;
                    case core::socket::State::DISABLED:
                        snode::semantic::appLog().info() << instanceName << ": disabled";
                        break;
                    case core::socket::State::ERROR:
                        snode::semantic::appLog().error() << instanceName << ": " << socketAddress.toString() << ": " << state.what();
                        break;
                    case core::socket::State::FATAL:
                        snode::semantic::appLog().critical() << instanceName << ": " << socketAddress.toString() << ": " << state.what();
                        break;
                }
            });
        return tlsClient;
    }

} // namespace tls

namespace legacy {

    using SocketClient = net::in::stream::legacy::SocketClient<apps::http::SimpleSocketProtocolFactory>;
    using SocketAddress = SocketClient::SocketAddress;
    using SocketConnection = SocketClient::SocketConnection;

    SocketClient getLegacyClient() {
        SocketClient legacyClient(
            "legacy",
            [](SocketConnection* socketConnection) { // OnConnect
                snode::semantic::appLog().debug() << "OnConnect";

                snode::semantic::appLog().debug() << "\tServer: " << socketConnection->getRemoteAddress().toString();
                snode::semantic::appLog().debug() << "\tClient: " << socketConnection->getLocalAddress().toString();
            },
            [](SocketConnection* socketConnection) { // onConnected
                snode::semantic::appLog().debug() << "OnConnected";

                socketConnection->sendToPeer("GET /index.html HTTP/1.1\r\nConnection: close\r\n\r\n"); // Connection: close\r\n\r\n");
            },
            [](SocketConnection* socketConnection) { // onDisconnect
                snode::semantic::appLog().debug() << "OnDisconnect";

                snode::semantic::appLog().debug() << "\tServer: " << socketConnection->getRemoteAddress().toString();
                snode::semantic::appLog().debug() << "\tClient: " << socketConnection->getLocalAddress().toString();
            });

        SocketAddress remoteAddress("localhost", 8080);

        remoteAddress.init();

        snode::semantic::appLog().debug() << "###############': " << remoteAddress.getCanonName();
        snode::semantic::appLog().debug() << "###############': " << remoteAddress.toString();

        legacyClient.connect(
            remoteAddress,
            [instanceName = legacyClient.getConfig()->getInstanceName()](
                const tls::SocketAddress& socketAddress,
                const core::socket::State& state) { // example.com:81 simulate connnect timeout
                switch (state) {
                    case core::socket::State::OK:
                        snode::semantic::appLog().info() << instanceName << ": connected to '" << socketAddress.toString() << "'";
                        break;
                    case core::socket::State::DISABLED:
                        snode::semantic::appLog().info() << instanceName << ": disabled";
                        break;
                    case core::socket::State::ERROR:
                        snode::semantic::appLog().error() << instanceName << ": " << socketAddress.toString() << ": " << state.what();
                        break;
                    case core::socket::State::FATAL:
                        snode::semantic::appLog().critical() << instanceName << ": " << socketAddress.toString() << ": " << state.what();
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

        legacyClient.connect(
            legacyRemoteAddress,
            [instanceName = legacyClient.getConfig()->getInstanceName()](
                const tls::SocketAddress& socketAddress,
                const core::socket::State& state) { // example.com:81 simulate connnect timeout
                switch (state) {
                    case core::socket::State::OK:
                        snode::semantic::appLog().info() << instanceName << ": connected to '" << socketAddress.toString() << "'";
                        break;
                    case core::socket::State::DISABLED:
                        snode::semantic::appLog().info() << instanceName << ": disabled";
                        break;
                    case core::socket::State::ERROR:
                        snode::semantic::appLog().error() << instanceName << ": " << socketAddress.toString() << ": " << state.what();
                        break;
                    case core::socket::State::FATAL:
                        snode::semantic::appLog().critical() << instanceName << ": " << socketAddress.toString() << ": " << state.what();
                        break;
                }
            });

        const tls::SocketAddress tlsRemoteAddress = tls::SocketAddress("localhost", 8088);

        const tls::SocketClient tlsClient = tls::getClient();

        tlsClient.connect(
            tlsRemoteAddress,
            [instanceName =
                 tlsClient.getConfig()->getInstanceName()](const tls::SocketAddress& socketAddress,
                                                           const core::socket::State& state) { // example.com:81 simulate connnect timeout
                switch (state) {
                    case core::socket::State::OK:
                        snode::semantic::appLog().info() << instanceName << ": connected to '" << socketAddress.toString() << "'";
                        break;
                    case core::socket::State::DISABLED:
                        snode::semantic::appLog().info() << instanceName << ": disabled";
                        break;
                    case core::socket::State::ERROR:
                        snode::semantic::appLog().error() << instanceName << ": " << socketAddress.toString() << ": " << state.what();
                        break;
                    case core::socket::State::FATAL:
                        snode::semantic::appLog().critical() << instanceName << ": " << socketAddress.toString() << ": " << state.what();
                        break;
                }
            });
    }

    return core::SNodeC::start();
}
