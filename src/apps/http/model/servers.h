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

#ifndef APPS_HTTP_MODEL_SERVERS_H
#define APPS_HTTP_MODEL_SERVERS_H

#define QUOTE_INCLUDE(a) STR_INCLUDE(a)
#define STR_INCLUDE(a) #a

// clang-format off
#define WEBAPP_INCLUDE QUOTE_INCLUDE(express/STREAM/NET/WebApp.h)
// clang-format on

#include WEBAPP_INCLUDE // IWYU pragma: export

#include "express/Router.h"
#include "express/middleware/StaticMiddleware.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#if (STREAM_TYPE == TLS) // tls
#include <cstddef>
#include <openssl/ssl.h>
#include <openssl/x509v3.h>
#endif

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

express::Router getRouter(const std::string& rootPath) {
    express::Router router;

    router.use(express::middleware::StaticMiddleware(rootPath));

    return router;
}

#if (STREAM_TYPE == LEGACY) // legacy

namespace apps::http::legacy {

    using WebApp = express::legacy::NET::WebApp;

    WebApp getWebApp(const std::string& name, const std::string& rootPath) {
        WebApp webApp(name, getRouter(rootPath));

        return webApp;
    }

} // namespace apps::http::legacy

#endif // (STREAM_TYPE == LEGACY) // legacy

#if (STREAM_TYPE == TLS) // tls

namespace apps::http::tls {

    using WebApp = express::tls::NET::WebApp;
    using SocketConnection = WebApp::SocketConnection;

    WebApp getWebApp(const std::string& name, const std::string& rootPath) {
        WebApp webApp(name, getRouter(rootPath));

        webApp.setOnConnect([&webApp](SocketConnection* socketConnection) -> void { // onConnect
            LOG(INFO) << "OnConnect " << webApp.getConfig().getInstanceName();

            LOG(INFO) << "\tLocal: (" + socketConnection->getLocalAddress().getAddress() + ") " +
                             socketConnection->getLocalAddress().toString();
            LOG(INFO) << "\tPeer:  (" + socketConnection->getRemoteAddress().getAddress() + ") " +
                             socketConnection->getRemoteAddress().toString();

            /* Enable automatic hostname checks */
            // X509_VERIFY_PARAM* param = SSL_get0_param(socketConnection->getSSL());

            // X509_VERIFY_PARAM_set_hostflags(param, X509_CHECK_FLAG_NO_PARTIAL_WILDCARDS);
            // if (!X509_VERIFY_PARAM_set1_host(param, "localhost", sizeof("localhost") - 1)) {
            //   // handle error
            //   socketConnection->close();
            // }
        });

        webApp.setOnConnected([&webApp](SocketConnection* socketConnection) -> void { // onConnected
            LOG(INFO) << "OnConnected " << webApp.getConfig().getInstanceName();

            X509* server_cert = SSL_get_peer_certificate(socketConnection->getSSL());
            if (server_cert != nullptr) {
                long verifyErr = SSL_get_verify_result(socketConnection->getSSL());

                LOG(INFO) << "\tPeer certificate verifyErr = " + std::to_string(verifyErr) + ": " +
                                 std::string(X509_verify_cert_error_string(verifyErr));

                char* str = X509_NAME_oneline(X509_get_subject_name(server_cert), nullptr, 0);
                LOG(INFO) << "\t   Subject: " + std::string(str);
                OPENSSL_free(str);

                str = X509_NAME_oneline(X509_get_issuer_name(server_cert), nullptr, 0);
                LOG(INFO) << "\t   Issuer: " + std::string(str);
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
                LOG(INFO) << "\t   Subject alternative name count: " << altNameCount;
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
                        LOG(INFO) << "\t      SAN (URI): '" + subjectAltName;
                    } else if (generalName->type == GEN_DNS) {
                        std::string subjectAltName =
                            std::string(reinterpret_cast<const char*>(ASN1_STRING_get0_data(generalName->d.dNSName)),
                                        static_cast<std::size_t>(ASN1_STRING_length(generalName->d.dNSName)));
                        LOG(INFO) << "\t      SAN (DNS): '" + subjectAltName;
                    } else {
                        LOG(INFO) << "\t      SAN (Type): '" + std::to_string(generalName->type);
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
                LOG(INFO) << "\tPeer certificate: no certificate";
            }
        });

        webApp.setOnDisconnect([&webApp](SocketConnection* socketConnection) -> void { // onDisconnect
            LOG(INFO) << "OnDisconnect " << webApp.getConfig().getInstanceName();

            LOG(INFO) << "\tLocal: (" + socketConnection->getLocalAddress().getAddress() + ") " +
                             socketConnection->getLocalAddress().toString();
            LOG(INFO) << "\tPeer:  (" + socketConnection->getRemoteAddress().getAddress() + ") " +
                             socketConnection->getRemoteAddress().toString();
        });

        return webApp;
    }

} // namespace apps::http::tls

#endif // (STREAM_TYPE == TLS) // tls

#endif // APPS_HTTP_MODEL_SERVERS_H
