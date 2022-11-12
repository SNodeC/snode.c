/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021, 2022 Volker Christian <me@vchrist.at>
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

#ifndef APPS_HTTP_MODEL_CLIENTS_H
#define APPS_HTTP_MODEL_CLIENTS_H

#define QUOTE_INCLUDE(a) STR_INCLUDE(a)
#define STR_INCLUDE(a) #a

// clang-format off
#define CLIENT_INCLUDE QUOTE_INCLUDE(web/http/STREAM/NET/Client.h)
// clang-format on

#include CLIENT_INCLUDE // IWYU pragma: export

#include "web/http/client/Request.h"
#include "web/http/client/Response.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#if (STREAM_TYPE == TLS) // tls
#include <cstddef>
#include <openssl/ssl.h>
#include <openssl/x509v3.h>
#endif

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#if (STREAM_TYPE == LEGACY) // legacy

namespace apps::http::legacy {

    using Request = web::http::client::Request;
    using Response = web::http::client::Response;
    using Client = web::http::legacy::NET::Client<Request, Response>;

    Client getClient() {
        return Client(
            "httpclient",
            [](Client::SocketConnection* socketConnection) -> void { // onConnect
                VLOG(0) << "OnConnect";

                VLOG(0) << "\tLocal: (" + socketConnection->getLocalAddress().address() + ") " +
                               socketConnection->getLocalAddress().toString();
                VLOG(0) << "\tPeer:  (" + socketConnection->getRemoteAddress().address() + ") " +
                               socketConnection->getRemoteAddress().toString();
            },
            []([[maybe_unused]] Client::SocketConnection* socketConnection) -> void {
                VLOG(0) << "-- OnConnected";
            },
            [](Request& request) -> void {
                request.url = "/index.html";
                request.set("Connection", "close");
                request.start();
            },
            []([[maybe_unused]] Request& request, Response& response) -> void {
                VLOG(0) << "-- OnResponse";
                VLOG(0) << "     Status:";
                VLOG(0) << "       " << response.httpVersion << " " << response.statusCode << " " << response.reason;

                VLOG(0) << "     Headers:";
                for (const auto& [field, value] : response.headers) {
                    VLOG(0) << "       " << field + " = " + value;
                }

                VLOG(0) << "     Cookies:";
                for (const auto& [name, cookie] : response.cookies) {
                    VLOG(0) << "       " + name + " = " + cookie.getValue();
                    for (const auto& [option, value] : cookie.getOptions()) {
                        VLOG(0) << "         " + option + " = " + value;
                    }
                }

                response.body.push_back(0); // make it a c-string
                VLOG(0) << "Body:\n----------- start body -----------\n" << response.body.data() << "\n------------ end body ------------";
            },
            [](int status, const std::string& reason) -> void {
                VLOG(0) << "-- OnResponseError";
                VLOG(0) << "     Status: " << status;
                VLOG(0) << "     Reason: " << reason;
            },
            [](Client::SocketConnection* socketConnection) -> void {
                VLOG(0) << "-- OnDisconnect";

                VLOG(0) << "\tLocal: (" + socketConnection->getLocalAddress().address() + ") " +
                               socketConnection->getLocalAddress().toString();
                VLOG(0) << "\tPeer:  (" + socketConnection->getRemoteAddress().address() + ") " +
                               socketConnection->getRemoteAddress().toString();
            });
    }

} // namespace apps::http::legacy

#endif // (STREAM_TYPE == LEGACY) // legacy

#if (STREAM_TYPE == TLS) // tls

namespace apps::http::tls {

    using Request = web::http::client::Request;
    using Response = web::http::client::Response;
    using Client = web::http::tls::NET::Client<Request, Response>;

    Client getClient() {
        return Client(
            "httpclient",
            [](Client::SocketConnection* socketConnection) -> void { // onConnect
                VLOG(0) << "OnConnect";

                VLOG(0) << "\tLocal: (" + socketConnection->getLocalAddress().address() + ") " +
                               socketConnection->getLocalAddress().toString();
                VLOG(0) << "\tPeer:  (" + socketConnection->getRemoteAddress().address() + ") " +
                               socketConnection->getRemoteAddress().toString();
            },
            [](Client::SocketConnection* socketConnection) -> void {
                VLOG(0) << "OnConnected";

                VLOG(0) << "\tSSL/TLS-Version: " << SSL_get_version(socketConnection->getSSL());
                VLOG(0) << "\tSSL/TLS-Version: " << SSL_version(socketConnection->getSSL());
                VLOG(0) << "\tSSL/TLS-Client-Version: " << SSL_client_version(socketConnection->getSSL());

                X509* server_cert = SSL_get_peer_certificate(socketConnection->getSSL());
                if (server_cert != nullptr) {
                    long verifyErr = SSL_get_verify_result(socketConnection->getSSL());

                    VLOG(0) << "\tPeer certificate: " + std::string(X509_verify_cert_error_string(verifyErr));

                    char* str = X509_NAME_oneline(X509_get_subject_name(server_cert), nullptr, 0);
                    VLOG(0) << "\t   Subject: " + std::string(str);
                    OPENSSL_free(str);

                    str = X509_NAME_oneline(X509_get_issuer_name(server_cert), nullptr, 0);
                    VLOG(0) << "\t   Issuer: " + std::string(str);
                    OPENSSL_free(str);

                    // We could do all sorts of certificate verification stuff here before deallocating the certificate.

                    GENERAL_NAMES* subjectAltNames =
                        static_cast<GENERAL_NAMES*>(X509_get_ext_d2i(server_cert, NID_subject_alt_name, nullptr, nullptr));

                    int32_t altNameCount = sk_GENERAL_NAME_num(subjectAltNames);
                    VLOG(0) << "\t   Subject alternative name count: " << altNameCount;
                    for (int32_t i = 0; i < altNameCount; ++i) {
                        GENERAL_NAME* generalName = sk_GENERAL_NAME_value(subjectAltNames, i);
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
                    sk_GENERAL_NAME_pop_free(subjectAltNames, GENERAL_NAME_free);

                    X509_free(server_cert);
                } else {
                    VLOG(0) << "\tPeer certificate: no certificate";
                }
            },
            [](Request& request) -> void {
                request.url = "/index.html";
                request.set("Connection", "close");
                request.start();
            },
            []([[maybe_unused]] Request& request, Response& response) -> void {
                VLOG(0) << "-- OnResponse";
                VLOG(0) << "     Status:";
                VLOG(0) << "       " << response.httpVersion << " " << response.statusCode << " " << response.reason;

                VLOG(0) << "     Headers:";
                for (const auto& [field, value] : response.headers) {
                    VLOG(0) << "       " << field + " = " + value;
                }

                VLOG(0) << "     Cookies:";
                for (const auto& [name, cookie] : response.cookies) {
                    VLOG(0) << "       " + name + " = " + cookie.getValue();
                    for (const auto& [option, value] : cookie.getOptions()) {
                        VLOG(0) << "         " + option + " = " + value;
                    }
                }

                response.body.push_back(0); // make it a c-string
                VLOG(0) << "Body:\n----------- start body -----------\n" << response.body.data() << "\n------------ end body ------------";
            },
            [](int status, const std::string& reason) -> void {
                VLOG(0) << "-- OnResponseError";
                VLOG(0) << "     Status: " << status;
                VLOG(0) << "     Reason: " << reason;
            },
            [](Client::SocketConnection* socketConnection) -> void {
                VLOG(0) << "OnDisconnect";

                VLOG(0) << "\tLocal: (" + socketConnection->getLocalAddress().address() + ") " +
                               socketConnection->getLocalAddress().toString();
                VLOG(0) << "\tPeer:  (" + socketConnection->getRemoteAddress().address() + ") " +
                               socketConnection->getRemoteAddress().toString();
            });
    }

} // namespace apps::http::tls

#endif // (STREAM_TYPE == TLS) // tls

#endif // APPS_HTTP_MODEL_CLIENTS_H
