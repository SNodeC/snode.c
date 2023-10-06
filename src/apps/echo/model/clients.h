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

#ifndef APPS_ECHO_MODEL_CLIENT_H
#define APPS_ECHO_MODEL_CLIENT_H

#include "log/Logger.h"

#define QUOTE_INCLUDE(a) STR(a)
#define STR(a) #a

// clang-format off
#define SOCKETCLIENT_INCLUDE QUOTE_INCLUDE(net/NET/stream/STREAM/SocketClient.h)
// clang-format on

#include SOCKETCLIENT_INCLUDE // IWYU pragma: export

#include "EchoSocketContext.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <string>

#if (STREAM_TYPE == TLS) // tls
#include <cstddef>
#include <openssl/ssl.h>
#include <openssl/x509v3.h>
#endif

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#if (STREAM_TYPE == LEGACY) // legacy

namespace apps::echo::model::legacy {

    using EchoSocketClient = net::NET::stream::legacy::SocketClient<EchoClientSocketContextFactory>;

    EchoSocketClient getClient() {
        return EchoSocketClient("echoclient");
    }

} // namespace apps::echo::model::legacy

#elif (STREAM_TYPE == TLS) // tls

namespace apps::echo::model::tls {

    using EchoSocketClient = net::NET::stream::tls::SocketClient<EchoClientSocketContextFactory>;
    using SocketConnection = EchoSocketClient::SocketConnection;

    EchoSocketClient getClient() {
        EchoSocketClient client("echoclient");

        client.setOnConnect([&client](SocketConnection* socketConnection) -> void { // onConnect
            VLOG(0) << "OnConnect " << client.getConfig().getInstanceName();

            VLOG(0) << "\tLocal: (" + socketConnection->getLocalAddress().address() + ") " + socketConnection->getLocalAddress().toString();
            VLOG(0) << "\tPeer:  (" + socketConnection->getRemoteAddress().address() + ") " +
                           socketConnection->getRemoteAddress().toString();

            /* Enable automatic hostname checks */
            // X509_VERIFY_PARAM* param = SSL_get0_param(socketConnection->getSSL());

            // X509_VERIFY_PARAM_set_hostflags(param, X509_CHECK_FLAG_NO_PARTIAL_WILDCARDS);
            // if (!X509_VERIFY_PARAM_set1_host(param, "localhost", sizeof("localhost") - 1)) {
            //   // handle error
            //   socketConnection->close();
            // }
        });

        client.setOnConnected([&client](SocketConnection* socketConnection) -> void { // onConnected
            VLOG(0) << "OnConnected " << client.getConfig().getInstanceName();

            X509* server_cert = SSL_get_peer_certificate(socketConnection->getSSL());
            if (server_cert != nullptr) {
                long verifyErr = SSL_get_verify_result(socketConnection->getSSL());

                VLOG(0) << "\tPeer certificate verifyErr = " + std::to_string(verifyErr) + ": " +
                               std::string(X509_verify_cert_error_string(verifyErr));

                char* str = X509_NAME_oneline(X509_get_subject_name(server_cert), nullptr, 0);
                VLOG(0) << "\t   Subject: " + std::string(str);
                OPENSSL_free(str);

                str = X509_NAME_oneline(X509_get_issuer_name(server_cert), nullptr, 0);
                VLOG(0) << "\t   Issuer: " + std::string(str);
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
                VLOG(0) << "\t   Subject alternative name count: " << altNameCount;
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
                        VLOG(0) << "\t      SAN (URI): '" + subjectAltName;
                    } else if (generalName->type == GEN_DNS) {
                        std::string subjectAltName =
                            std::string(reinterpret_cast<const char*>(ASN1_STRING_get0_data(generalName->d.dNSName)),
                                        static_cast<std::size_t>(ASN1_STRING_length(generalName->d.dNSName)));
                        VLOG(0) << "\t      SAN (DNS): '" + subjectAltName;
                    } else {
                        VLOG(0) << "\t      SAN (Type): '" + std::to_string(generalName->type);
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
                VLOG(0) << "\tPeer certificate: no certificate";
            }
        });

        client.setOnDisconnect([&client](SocketConnection* socketConnection) -> void { // onDisconnect
            VLOG(0) << "OnDisconnect " << client.getConfig().getInstanceName();

            VLOG(0) << "\tLocal: (" + socketConnection->getLocalAddress().address() + ") " + socketConnection->getLocalAddress().toString();
            VLOG(0) << "\tPeer:  (" + socketConnection->getRemoteAddress().address() + ") " +
                           socketConnection->getRemoteAddress().toString();
        });

        return client;
    }

} // namespace apps::echo::model::tls

#endif

#endif // APPS_ECHO_MODEL_CLIENT_H
