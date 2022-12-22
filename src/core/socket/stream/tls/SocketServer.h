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

#ifndef CORE_SOCKET_STREAM_TLS_SOCKETSERVER_H
#define CORE_SOCKET_STREAM_TLS_SOCKETSERVER_H

#include "core/socket/stream/SocketServer.h"       // IWYU pragma: export
#include "core/socket/stream/tls/SocketAcceptor.h" // IWYU pragma: export

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#include <openssl/ssl.h>
#include <openssl/x509v3.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::socket::stream::tls {

    template <typename SocketServerT, typename SocketContextFactoryT>
    class SocketServer
        : public core::socket::stream::SocketServer<SocketServerT, core::socket::stream::tls::SocketAcceptor, SocketContextFactoryT> {
    private:
        using Super = core::socket::stream::SocketServer<SocketServerT, core::socket::stream::tls::SocketAcceptor, SocketContextFactoryT>;
        using Super::Super;

    public:
        using SocketConnection = typename Super::SocketConnection;
        using SocketAddress = typename Super::SocketAddress;

        SocketServer(const std::string& name,
                     const std::function<void(SocketConnection*)>& onConnect,
                     const std::function<void(SocketConnection*)>& onConnected,
                     const std::function<void(SocketConnection*)>& onDisconnect,
                     const std::map<std::string, std::any>& options = {{}})
            : Super(name, onConnect, onConnected, onDisconnect, options)
            , sniSslCtxs(new std::map<std::string, SSL_CTX*>, [](std::map<std::string, SSL_CTX*>* sniSslCtxs) {
                for (const auto& [domain, sniSslCtx] : *sniSslCtxs) {
                    ssl_ctx_free(sniSslCtx);
                }
                delete sniSslCtxs;
            }) {
            Super::_options.insert({{"SNI_SSL_CTXS", sniSslCtxs}});
        }

        SocketServer(const std::function<void(SocketConnection*)>& onConnect,
                     const std::function<void(SocketConnection*)>& onConnected,
                     const std::function<void(SocketConnection*)>& onDisconnect,
                     const std::map<std::string, std::any>& options = {{}})
            : SocketServer("", onConnect, onConnected, onDisconnect, options) {
        }

        explicit SocketServer(const std::string& name, const std::map<std::string, std::any>& options = {{}})
            : SocketServer(
                  name,
                  [name](SocketConnection* socketConnection) -> void { // onConnect
                      VLOG(0) << "OnConnect - " << name;

                      VLOG(0) << "\tLocal: (" + socketConnection->getLocalAddress().address() + ") " +
                                     socketConnection->getLocalAddress().toString();
                      VLOG(0) << "\tPeer:  (" + socketConnection->getRemoteAddress().address() + ") " +
                                     socketConnection->getRemoteAddress().toString();
                  },
                  [name](SocketConnection* socketConnection) -> void { // onConnected
                      VLOG(0) << "OnConnected - " << name;

                      X509* client_cert = SSL_get_peer_certificate(socketConnection->getSSL());
                      if (client_cert != nullptr) {
                          long verifyErr = SSL_get_verify_result(socketConnection->getSSL());

                          VLOG(0) << "\tPeer certificate: " + std::string(X509_verify_cert_error_string(verifyErr));

                          char* str = X509_NAME_oneline(X509_get_subject_name(client_cert), nullptr, 0);
                          VLOG(0) << "\t   Subject: " + std::string(str);
                          OPENSSL_free(str);

                          str = X509_NAME_oneline(X509_get_issuer_name(client_cert), nullptr, 0);
                          VLOG(0) << "\t   Issuer: " + std::string(str);
                          OPENSSL_free(str);

                          // We could do all sorts of certificate verification stuff here before deallocating the certificate.

                          GENERAL_NAMES* subjectAltNames =
                              static_cast<GENERAL_NAMES*>(X509_get_ext_d2i(client_cert, NID_subject_alt_name, nullptr, nullptr));

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

                          X509_free(client_cert);
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
                  },
                  options) {
        }

        explicit SocketServer(const std::map<std::string, std::any>& options = {{}})
            : SocketServer("", options) {
        }

    public:
        void addSniCert(const std::string& domain, const std::map<std::string, std::any>& sniCert) {
            SSL_CTX* sslCtx = ssl_ctx_new(sniCert, true);

            if (sslCtx != nullptr) {
                sniSslCtxs->insert_or_assign(domain, sslCtx);
                VLOG(2) << "SSL_CTX for domain '" << domain << "' installed";
            } else {
                VLOG(2) << "Can not create SSL_CTX for SNI '" << domain << "'";
            }
        }

        void addSniCerts(const std::map<std::string, std::map<std::string, std::any>>& sniCerts) {
            for (const auto& [domain, cert] : sniCerts) {
                addSniCert(domain, cert);
            }
        }

        std::shared_ptr<std::map<std::string, SSL_CTX*>> sniSslCtxs;
    };

} // namespace core::socket::stream::tls

#endif // CORE_SOCKET_STREAM_TLS_SOCKETSERVER_H
