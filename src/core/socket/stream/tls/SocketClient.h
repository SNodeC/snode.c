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

#ifndef CORE_SOCKET_STREAM_TLS_SOCKETCLIENT_H
#define CORE_SOCKET_STREAM_TLS_SOCKETCLIENT_H

#include "core/socket/stream/SocketClient.h"        // IWYU pragma: export
#include "core/socket/stream/tls/SocketConnector.h" // IWYU pragma: export

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "core/socket/stream/tls/ssl_version.h" // IWYU pragma: export
#include "log/Logger.h"

#include <openssl/ssl.h>    // IWYU pragma: export
#include <openssl/x509v3.h> // IWYU pragma: export

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::socket::stream::tls {

    template <typename SocketClientT, typename SocketContextFactoryT>
    class SocketClient
        : public core::socket::stream::SocketClient<SocketClientT, core::socket::stream::tls::SocketConnector, SocketContextFactoryT> {
    private:
        using Super = core::socket::stream::SocketClient<SocketClientT, core::socket::stream::tls::SocketConnector, SocketContextFactoryT>;

    public:
        using SocketConnection = typename Super::SocketConnection;
        using SocketAddress = typename Super::SocketAddress;

        using Super::Super;

        explicit SocketClient(const std::string& name)
            : SocketClient(
                  name,
                  [name](SocketConnection* socketConnection) -> void { // onConnect
                      VLOG(0) << "OnConnect - " << name;

                      VLOG(0) << "\tLocal: (" + socketConnection->getLocalAddress().address() + ") " +
                                     socketConnection->getLocalAddress().toString();
                      VLOG(0) << "\tPeer:  (" + socketConnection->getRemoteAddress().address() + ") " +
                                     socketConnection->getRemoteAddress().toString();

                      /* Enable automatic hostname checks */
                      // X509_VERIFY_PARAM* param = SSL_get0_param(socketConnection->getSSL());

                      // X509_VERIFY_PARAM_set_hostflags(param, X509_CHECK_FLAG_NO_PARTIAL_WILDCARDS);
                      // if (!X509_VERIFY_PARAM_set1_host(param, "localhost", sizeof("localhost") - 1)) {
                      //   // handle error
                      //   socketConnection->close();
                      // }
                  },
                  [name](SocketConnection* socketConnection) -> void { // onConnected
                      VLOG(0) << "OnConnected - " << name;

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
                                  std::string subjectAltName = std::string(
                                      reinterpret_cast<const char*>(ASN1_STRING_get0_data(generalName->d.uniformResourceIdentifier)),
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
                  [name](SocketConnection* socketConnection) -> void { // onDisconnect
                      VLOG(0) << "OnDisconnect - " << name;

                      VLOG(0) << "\tLocal: (" + socketConnection->getLocalAddress().address() + ") " +
                                     socketConnection->getLocalAddress().toString();
                      VLOG(0) << "\tPeer:  (" + socketConnection->getRemoteAddress().address() + ") " +
                                     socketConnection->getRemoteAddress().toString();
                  }) {
        }

        explicit SocketClient()
            : SocketClient("") {
        }

        void setSni(const std::string& sniName) {
            Super::config->setSni(sniName);
        }
    };

} // namespace core::socket::stream::tls

#endif // CORE_SOCKET_STREAM_TLS_SOCKETCLIENT_H
