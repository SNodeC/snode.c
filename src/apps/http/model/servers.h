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

#ifndef APPS_HTTP_MODEL_SERVERS_H
#define APPS_HTTP_MODEL_SERVERS_H

#define QUOTE_INCLUDE(a) STR_INCLUDE(a)
#define STR_INCLUDE(a) #a

// clang-format off
#define WEBAPP_INCLUDE QUOTE_INCLUDE(express/STREAM/NET/WebApp.h)
// clang-format on

#include "SemanticLog.h"
#include WEBAPP_INCLUDE // IWYU pragma: export

#include "express/middleware/VerboseRequest.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#if (STREAM_TYPE == TLS) // tls
#include <cstddef>
#include <openssl/ssl.h>
#include <openssl/x509v3.h>
#endif

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

static express::Router getRouter() {
    return express::middleware::VerboseRequest();
}

#if (STREAM_TYPE == LEGACY) // legacy

namespace apps::http::legacy {

    using WebApp = express::legacy::NET::WebApp;
    using SocketConnection = WebApp::SocketConnection;

    static WebApp getWebApp(const std::string& name) {
        WebApp webApp(name, getRouter());

        return webApp;
    }

} // namespace apps::http::legacy

#endif // (STREAM_TYPE == LEGACY) // legacy

#if (STREAM_TYPE == TLS) // tls

namespace apps::http::tls {

    using WebApp = express::tls::NET::WebApp;
    using SocketConnection = WebApp::SocketConnection;

    static WebApp getWebApp(const std::string& name) {
        WebApp webApp(name, getRouter());

        const std::string& instanceName = webApp.getConfig()->getInstanceName();

        webApp.setOnConnect([instanceName](SocketConnection* socketConnection) { // onConnect
            snode::semantic::appLog().debug() << "OnConnect " << instanceName;

            snode::semantic::appLog().debug() << "  Local: " << socketConnection->getLocalAddress().toString();
            snode::semantic::appLog().debug() << "   Peer: " << socketConnection->getRemoteAddress().toString();

            /* Enable automatic hostname checks */
            // X509_VERIFY_PARAM* param = SSL_get0_param(socketConnection->getSSL());

            // X509_VERIFY_PARAM_set_hostflags(param, X509_CHECK_FLAG_NO_PARTIAL_WILDCARDS);
            // if (!X509_VERIFY_PARAM_set1_host(param, "localhost", sizeof("localhost") - 1)) {
            //   // handle error
            //   socketConnection->close();
            // }
        });

        webApp.setOnConnected([instanceName](SocketConnection* socketConnection) { // onConnected
            snode::semantic::appLog().debug() << "OnConnected " << instanceName;

            auto log = snode::semantic::appLog();
            if (log.enabled(logger::LogLevel::Debug)) {
                X509* server_cert = SSL_get_peer_certificate(socketConnection->getSSL());
                if (server_cert != nullptr) {
                    long verifyErr = SSL_get_verify_result(socketConnection->getSSL());

                    snode::semantic::appLog().debug() << "\tPeer certificate verifyErr = " + std::to_string(verifyErr) + ": " +
                                                             std::string(X509_verify_cert_error_string(verifyErr));

                    char* str = X509_NAME_oneline(X509_get_subject_name(server_cert), nullptr, 0);
                    if (str != nullptr) {
                        snode::semantic::appLog().debug() << "\t   Subject: " << str;
                        OPENSSL_free(str);
                    }

                    str = X509_NAME_oneline(X509_get_issuer_name(server_cert), nullptr, 0);
                    if (str != nullptr) {
                        snode::semantic::appLog().debug() << "\t   Issuer: " << str;
                        OPENSSL_free(str);
                    }

                    // We could do all sorts of certificate verification stuff here before deallocating the certificate.

                    GENERAL_NAMES* subjectAltNames =
                        static_cast<GENERAL_NAMES*>(X509_get_ext_d2i(server_cert, NID_subject_alt_name, nullptr, nullptr));

                    int32_t altNameCount = OPENSSL_sk_num(reinterpret_cast<const OPENSSL_STACK*>(subjectAltNames));

                    snode::semantic::appLog().debug() << "\t   Subject alternative name count: " << altNameCount;
                    for (int32_t i = 0; i < altNameCount; ++i) {
                        GENERAL_NAME* generalName = sk_GENERAL_NAME_value(subjectAltNames, i);

                        if (generalName->type == GEN_URI) {
                            std::string subjectAltName =
                                std::string(reinterpret_cast<const char*>(ASN1_STRING_get0_data(generalName->d.uniformResourceIdentifier)),
                                            static_cast<std::size_t>(ASN1_STRING_length(generalName->d.uniformResourceIdentifier)));
                            snode::semantic::appLog().debug() << "\t      SAN (URI): '" + subjectAltName;
                        } else if (generalName->type == GEN_DNS) {
                            std::string subjectAltName =
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
                    snode::semantic::appLog().warn() << "\tPeer certificate: no certificate";
                }
            }
        });

        webApp.setOnDisconnect([instanceName](SocketConnection* socketConnection) { // onDisconnect
            snode::semantic::appLog().debug() << "OnDisconnect " << instanceName;

            snode::semantic::appLog().debug() << "            Local: " << socketConnection->getLocalAddress().toString(false);
            snode::semantic::appLog().debug() << "             Peer: " << socketConnection->getRemoteAddress().toString(false);

            snode::semantic::appLog().debug() << "     Online Since: " << socketConnection->getOnlineSince();
            snode::semantic::appLog().debug() << "  Online Duration: " << socketConnection->getOnlineDuration();

            snode::semantic::appLog().debug() << "     Total Queued: " << socketConnection->getTotalQueued();
            snode::semantic::appLog().debug() << "       Total Sent: " << socketConnection->getTotalSent();
            snode::semantic::appLog().debug() << "      Write Delta: "
                                              << socketConnection->getTotalQueued() - socketConnection->getTotalSent();
            snode::semantic::appLog().debug() << "       Total Read: " << socketConnection->getTotalRead();
            snode::semantic::appLog().debug() << "  Total Processed: " << socketConnection->getTotalProcessed();
            snode::semantic::appLog().debug() << "       Read Delta: "
                                              << socketConnection->getTotalRead() - socketConnection->getTotalProcessed();
        });

        return webApp;
    }

} // namespace apps::http::tls

#endif // (STREAM_TYPE == TLS) // tls

#endif // APPS_HTTP_MODEL_SERVERS_H
