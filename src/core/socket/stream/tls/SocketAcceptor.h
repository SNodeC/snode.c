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

#ifndef NET_SOCKET_STREAM_TLS_SOCKETACCEPTOR_H
#define NET_SOCKET_STREAM_TLS_SOCKETACCEPTOR_H

#include "core/socket/stream/SocketAcceptor.h"
#include "core/socket/stream/tls/SocketConnection.h" // IWYU pragma: export
#include "core/socket/stream/tls/ssl_utils.h"
#include "log/Logger.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef>
#include <map>
#include <memory>
#include <openssl/x509v3.h>
#include <set>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::socket::stream::tls {

    template <typename SocketT>
    class SocketAcceptor : public core::socket::stream::SocketAcceptor<tls::SocketConnection<SocketT>> {
    private:
        using Socket = SocketT;

    public:
        using SocketConnection = tls::SocketConnection<Socket>;
        using SocketAddress = typename Socket::SocketAddress;

        SocketAcceptor(const std::shared_ptr<SocketContextFactory>& socketContextFactory,
                       const std::function<void(const SocketAddress&, const SocketAddress&)>& onConnect,
                       const std::function<void(SocketConnection*)>& onConnected,
                       const std::function<void(SocketConnection*)>& onDisconnect,
                       const std::map<std::string, std::any>& options)
            : core::socket::stream::SocketAcceptor<SocketConnection>(
                  socketContextFactory,
                  onConnect,
                  [onConnected, this](SocketConnection* socketConnection) -> void {
                      SSL* ssl = socketConnection->startSSL(this->masterSslCtx);

                      if (ssl != nullptr) {
                          SSL_CTX_set_tlsext_servername_arg(this->masterSslCtx, this);

                          SSL_set_accept_state(ssl);

                          socketConnection->doSSLHandshake(
                              [&onConnected, socketConnection](void) -> void { // onSuccess
                                  LOG(INFO) << "SSL/TLS initial handshake success";
                                  socketConnection->SocketConnection::SocketReader::resume();
                                  onConnected(socketConnection);
                              },
                              [](void) -> void { // onTimeout
                                  LOG(WARNING) << "SSL/TLS initial handshake timed out";
                              },
                              [](int sslErr) -> void { // onError
                                  ssl_log("SSL/TLS initial handshake failed", sslErr);
                              });
                      } else {
                          socketConnection->SocketConnection::SocketReader::disable();
                          socketConnection->SocketConnection::SocketWriter::disable();
                          ssl_log_error("SSL/TLS initialization failed");
                      }
                  },
                  [onDisconnect](SocketConnection* socketConnection) -> void { // onDisconnect
                      socketConnection->stopSSL();
                      onDisconnect(socketConnection);
                  },
                  options) {
            masterSslCtx = ssl_ctx_new(options, true);

            if (masterSslCtx != nullptr) {
                SSL_CTX_set_tlsext_servername_callback(masterSslCtx, serverNameCallback);
                addMasterCert(masterSslCtx);
            }

            sniSslCtxs = std::any_cast<std::shared_ptr<std::map<std::string, SSL_CTX*>>>(options.find("SNI_SSL_CTXS")->second);
        }

        ~SocketAcceptor() override {
            ssl_ctx_free(masterSslCtx);
        }

        void listen(const SocketAddress& localAddress, int backlog, const std::function<void(int)>& onError) {
            if (masterSslCtx == nullptr) {
                errno = EINVAL;
                onError(errno);
                core::socket::stream::SocketAcceptor<SocketConnection>::destruct();
            } else {
                core::socket::stream::SocketAcceptor<SocketConnection>::listen(localAddress, backlog, onError);
            }
        }

    private:
        void addMasterCert(SSL_CTX* sSlCtx) {
            if (sSlCtx != nullptr) {
                X509* x509 = SSL_CTX_get0_certificate(sSlCtx);
                if (x509 != nullptr) {
                    GENERAL_NAMES* subjectAltNames =
                        static_cast<GENERAL_NAMES*>(X509_get_ext_d2i(x509, NID_subject_alt_name, nullptr, nullptr));

                    int32_t altNameCount = sk_GENERAL_NAME_num(subjectAltNames);

                    for (int32_t i = 0; i < altNameCount; ++i) {
                        GENERAL_NAME* generalName = sk_GENERAL_NAME_value(subjectAltNames, i);
                        if (generalName->type == GEN_DNS) {
                            std::string subjectAltName =
                                std::string(reinterpret_cast<const char*>(ASN1_STRING_get0_data(generalName->d.dNSName)),
                                            static_cast<std::size_t>(ASN1_STRING_length(generalName->d.dNSName)));
                            VLOG(2) << "SSL_CTX for domain '" << subjectAltName << "' installed";
                            masterSslCtxDomains.insert(subjectAltName);
                        }
                    }
                    sk_GENERAL_NAME_pop_free(subjectAltNames, GENERAL_NAME_free);
                } else {
                    VLOG(2) << "\tClient certificate: no certificate";
                }
            }
        }

        static int serverNameCallback(SSL* ssl, [[maybe_unused]] int* al, [[maybe_unused]] void* arg) {
            int ret = SSL_TLSEXT_ERR_OK;

            SocketAcceptor* socketAcceptor = static_cast<SocketAcceptor*>(arg);
            std::shared_ptr<std::map<std::string, SSL_CTX*>> sniSslCtxs = socketAcceptor->sniSslCtxs;

            if (SSL_get_servername_type(ssl) != -1) {
                std::string serverNameIndication = SSL_get_servername(ssl, TLSEXT_NAMETYPE_host_name);

                if (!socketAcceptor->masterSslCtxDomains.contains(serverNameIndication)) {
                    if (sniSslCtxs->contains(serverNameIndication)) {
                        SSL_CTX* sniSslCtx = (*sniSslCtxs.get())[serverNameIndication];

                        SSL_CTX* nowUsedSslCtx = SSL_set_SSL_CTX(ssl, sniSslCtx);
                        if (nowUsedSslCtx == sniSslCtx) {
                            LOG(INFO) << "SSL_CTX: Switched for SNI '" << serverNameIndication << "'";
                        } else if (nowUsedSslCtx != nullptr) {
                            LOG(INFO) << "SSL_CTX: Not switcher for SNI '" << serverNameIndication << "': still using master SSL_CTX";
                            ret = SSL_TLSEXT_ERR_NOACK;
                        } else {
                            LOG(WARNING) << "SSL_CTX: Found but none used for SNI '" << serverNameIndication << '"';
                        }
                    } else {
                        LOG(INFO) << "SSL_CTX: Not found for SNI '" << serverNameIndication;
                        ret = SSL_TLSEXT_ERR_NOACK;
                    }
                } else {
                    LOG(INFO) << "SSL_CTX: Master SSL_CTX already provides: '" << serverNameIndication << "'";
                }
            }

            return ret;
        }

    private:
        SSL_CTX* masterSslCtx = nullptr;
        std::set<std::string> masterSslCtxDomains;

        std::shared_ptr<std::map<std::string, SSL_CTX*>> sniSslCtxs;
    };

} // namespace core::socket::stream::tls

#endif // NET_SOCKET_STREAM_TLS_SOCKETACCEPTOR_H
