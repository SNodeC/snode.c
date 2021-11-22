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

#include "config.h"                        // for SERVERCAFILE
#include "core/SNodeC.h"                   // for SNodeC
#include "log/Logger.h"                    // for Writer, Storage
#include "web/http/client/Request.h"       // for Request, client
#include "web/http/client/Response.h"      // for Response
#include "web/http/client/legacy/Client.h" // for Client6, Client6...
#include "web/http/client/tls/Client.h"    // for Client6, Client6...

#include <cstring>            // for memcpy
#include <openssl/asn1.h>     // for ASN1_STRING_get0...
#include <openssl/crypto.h>   // for OPENSSL_free
#include <openssl/obj_mac.h>  // for NID_subject_alt_...
#include <openssl/ossl_typ.h> // for X509
#include <openssl/ssl3.h>     // for SSL_get_peer_cer...
#include <openssl/x509.h>     // for X509_NAME_oneline
#include <openssl/x509v3.h>   // for GENERAL_NAME
#include <type_traits>        // for add_const<>::type
#include <utility>            // for tuple_element<>:...

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

using namespace web::http::client;

int main(int argc, char* argv[]) {
    core::SNodeC::init(argc, argv);

    {
        legacy::ClientRfComm<Request, Response> legacyClient(
            [](const legacy::ClientRfComm<>::SocketAddress& localAddress,
               const legacy::ClientRfComm<>::SocketAddress& remoteAddress) -> void {
                VLOG(0) << "-- OnConnect";

                VLOG(0) << "\tServer: (" + remoteAddress.address() + ") " + remoteAddress.toString();
                VLOG(0) << "\tClient: (" + localAddress.address() + ") " + localAddress.toString();
            },
            []([[maybe_unused]] legacy::ClientRfComm<>::SocketConnection* socketConnection) -> void {
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
                for (const auto& [field, value] : *response.headers) {
                    VLOG(0) << "       " << field + " = " + value;
                }

                VLOG(0) << "     Cookies:";
                for (const auto& [name, cookie] : *response.cookies) {
                    VLOG(0) << "       " + name + " = " + cookie.getValue();
                    for (const auto& [option, value] : cookie.getOptions()) {
                        VLOG(0) << "         " + option + " = " + value;
                    }
                }

                char* body = new char[response.contentLength + 1];
                memcpy(body, response.body, response.contentLength);
                body[response.contentLength] = 0;

                VLOG(1) << "     Body:\n----------- start body -----------\n" << body << "------------ end body ------------";

                delete[] body;
            },
            [](int status, const std::string& reason) -> void {
                VLOG(0) << "-- OnResponseError";
                VLOG(0) << "     Status: " << status;
                VLOG(0) << "     Reason: " << reason;
            },
            [](legacy::ClientRfComm<>::SocketConnection* socketConnection) -> void {
                VLOG(0) << "-- OnDisconnect";

                VLOG(0) << "\tServer: (" + socketConnection->getRemoteAddress().address() + ") " +
                               socketConnection->getRemoteAddress().toString();
                VLOG(0) << "\tClient: (" + socketConnection->getLocalAddress().address() + ") " +
                               socketConnection->getLocalAddress().toString();
            });

        tls::ClientRfComm<Request, Response> tlsClient(
            [](const tls::ClientRfComm<>::SocketAddress& localAddress, const tls::ClientRfComm<>::SocketAddress& remoteAddress) -> void {
                VLOG(0) << "-- OnConnect";

                VLOG(0) << "\tServer: (" + remoteAddress.address() + ") " + remoteAddress.toString();
                VLOG(0) << "\tClient: (" + localAddress.address() + ") " + localAddress.toString();
            },
            [](tls::ClientRfComm<>::SocketConnection* socketConnection) -> void {
                VLOG(0) << "-- OnConnected";

                X509* server_cert = SSL_get_peer_certificate(socketConnection->getSSL());
                if (server_cert != nullptr) {
                    long verifyErr = SSL_get_verify_result(socketConnection->getSSL());

                    VLOG(0) << "     Server certificate: " + std::string(X509_verify_cert_error_string(verifyErr));

                    char* str = X509_NAME_oneline(X509_get_subject_name(server_cert), 0, 0);
                    VLOG(0) << "        Subject: " + std::string(str);
                    OPENSSL_free(str);

                    str = X509_NAME_oneline(X509_get_issuer_name(server_cert), 0, 0);
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
            },
            [](Request& request) -> void {
                request.url = "/index.html";
                request.set("Connection", "close");
                request.start();
            },
            []([[maybe_unused]] Request& request, const Response& response) -> void {
                VLOG(0) << "-- OnResponse";
                VLOG(0) << "     Status:";
                VLOG(0) << "       " << response.httpVersion << " " << response.statusCode << " " << response.reason;

                VLOG(0) << "     Headers:";
                for (const auto& [field, value] : *response.headers) {
                    VLOG(0) << "       " << field + " = " + value;
                }

                VLOG(0) << "     Cookies:";
                for (const auto& [name, cookie] : *response.cookies) {
                    VLOG(0) << "       " + name + " = " + cookie.getValue();
                    for (const auto& [option, value] : cookie.getOptions()) {
                        VLOG(0) << "         " + option + " = " + value;
                    }
                }

                char* body = new char[response.contentLength + 1];
                memcpy(body, response.body, response.contentLength);
                body[response.contentLength] = 0;

                VLOG(1) << "     Body:\n----------- start body -----------\n" << body << "------------ end body ------------";

                delete[] body;
            },
            [](int status, const std::string& reason) -> void {
                VLOG(0) << "-- OnResponseError";
                VLOG(0) << "     Status: " << status;
                VLOG(0) << "     Reason: " << reason;
            },
            [](tls::ClientRfComm<>::SocketConnection* socketConnection) -> void {
                VLOG(0) << "-- OnDisconnect";

                VLOG(0) << "\tServer: (" + socketConnection->getRemoteAddress().address() + ") " +
                               socketConnection->getRemoteAddress().toString();
                VLOG(0) << "\tClient: (" + socketConnection->getLocalAddress().address() + ") " +
                               socketConnection->getLocalAddress().toString();
            },
            {{"caFile", SERVERCAFILE}, {"Host", "myhost"}});

        // "A4:B1:C1:2C:82:37" titan
        // "44:01:BB:A3:63:32"  mpow

        legacyClient.connect("A4:B1:C1:2C:82:37", 1, "44:01:BB:A3:63:32", [](int err) -> void {
            if (err != 0) {
                PLOG(ERROR) << "OnError: " << err;
            }
        });

        tlsClient.connect("A4:B1:C1:2C:82:37", 2, "44:01:BB:A3:63:32", [](int err) -> void {
            if (err != 0) {
                PLOG(ERROR) << "OnError: " << err;
            }
        });
    }

    return core::SNodeC::start();
}
