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

#ifndef CORE_SOCKET_STREAM_TLS_SOCKETACCEPTOR_H
#define CORE_SOCKET_STREAM_TLS_SOCKETACCEPTOR_H

#include "core/socket/stream/SocketAcceptor.h"
#include "core/socket/stream/tls/SocketConnection.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "core/socket/stream/tls/ssl_utils.h"
#include "log/Logger.h"

#include <cstddef>
#include <map>
#include <memory>
#include <openssl/x509v3.h>
#include <set>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::socket::stream::tls {

    template <typename SocketServerT>
    class SocketAcceptor : protected core::socket::stream::SocketAcceptor<SocketServerT, core::socket::stream::tls::SocketConnection> {
    private:
        using Super = core::socket::stream::SocketAcceptor<SocketServerT, core::socket::stream::tls::SocketConnection>;

        using SocketAddress = typename Super::SocketAddress;

    public:
        using Config = typename Super::Config;
        using SocketServer = SocketServerT;
        using SocketConnection = typename Super::SocketConnection;

        SocketAcceptor(const std::shared_ptr<core::socket::SocketContextFactory>& socketContextFactory,
                       const std::function<void(SocketConnection*)>& onConnect,
                       const std::function<void(SocketConnection*)>& onConnected,
                       const std::function<void(SocketConnection*)>& onDisconnect,
                       const std::map<std::string, std::any>& options)
            : Super(
                  socketContextFactory,
                  onConnect,
                  [onConnected, this](SocketConnection* socketConnection) -> void {
                      SSL* ssl = socketConnection->startSSL(
                          this->masterSslCtx, this->config->getInitTimeout(), this->config->getShutdownTimeout());

                      if (ssl != nullptr) {
                          SSL_set_accept_state(ssl);

                          socketConnection->doSSLHandshake(
                              [&onConnected, socketConnection]() -> void { // onSuccess
                                  LOG(INFO) << "SSL/TLS initial handshake success";
                                  onConnected(socketConnection);
                                  socketConnection->onConnected();
                              },
                              []() -> void { // onTimeout
                                  LOG(WARNING) << "SSL/TLS initial handshake timed out";
                              },
                              [](int sslErr) -> void { // onError
                                  ssl_log("SSL/TLS initial handshake failed", sslErr);
                              });
                      } else {
                          socketConnection->close();
                          ssl_log_error("SSL/TLS initialization failed");
                      }
                  },
                  [onDisconnect](SocketConnection* socketConnection) -> void { // onDisconnect
                      socketConnection->stopSSL();
                      onDisconnect(socketConnection);
                  },
                  options)
            , masterSslCtx(ssl_ctx_new(options, true))
            //, masterSslCtx(SSL_CTX_new(TLS_server_method()))
            , masterSslCtxDomains(ssl_get_sans(masterSslCtx)) {
            if (masterSslCtx != nullptr) {
                SSL_CTX_set_client_hello_cb(masterSslCtx, clientHelloCallback, this);
            }

            sniSslCtxs = std::any_cast<std::shared_ptr<std::map<std::string, SSL_CTX*>>>(options.find("SNI_SSL_CTXS")->second);
            forceSni = std::any_cast<bool>(options.find("FORCE_SNI")->second);
        }

        ~SocketAcceptor() override {
            ssl_ctx_free(masterSslCtx);
        }

        void listen(const std::shared_ptr<Config>& config, const std::function<void(const SocketAddress&, int)>& onError) {
            if (masterSslCtx == nullptr) {
                errno = EINVAL;
                onError(config->getLocalAddress(), errno);
                Super::destruct();
            } else {
                Super::listen(config, onError);
            }
        }

    private:
        static int clientHelloCallback(SSL* ssl, int* al, void* arg) {
            int ret = SSL_CLIENT_HELLO_SUCCESS;

            SocketAcceptor* socketAcceptor = static_cast<SocketAcceptor*>(arg);

            std::string serverNameIndication = ssl_get_servername_from_client_hello(ssl);

            if (!serverNameIndication.empty()) {
                LOG(INFO) << "ServerNameIndication: " << serverNameIndication;

                if (socketAcceptor->masterSslCtxDomains.contains(serverNameIndication)) {
                    LOG(INFO) << "SSL_CTX: Master SSL_CTX already provides SNI '" << serverNameIndication << "'";
                } else if (socketAcceptor->sniSslCtxs->contains(serverNameIndication)) {
                    SSL_CTX* sniSslCtx = (*socketAcceptor->sniSslCtxs.get())[serverNameIndication];

                    SSL_CTX* nowUsedSslCtx = ssl_set_ssl_ctx(ssl, sniSslCtx);

                    if (nowUsedSslCtx == sniSslCtx) {
                        LOG(INFO) << "SSL_CTX: Switched for SNI '" << serverNameIndication << "'";
                    } else if (nowUsedSslCtx != nullptr) {
                        if (!socketAcceptor->forceSni) {
                            LOG(WARNING) << "SSL_CTX: Not switcher for SNI '" << serverNameIndication << "'. Master SSL_CTX still used.";
                        } else {
                            LOG(ERROR) << "SSL_CTX: Not switched for SNI '" << serverNameIndication << "'.";
                            ret = SSL_CLIENT_HELLO_ERROR;
                            *al = SSL_AD_UNRECOGNIZED_NAME;
                        }
                    } else if (!socketAcceptor->forceSni) {
                        LOG(WARNING) << "SSL_CTX: Not switched for SNI '" << serverNameIndication << "'. Master SSL_CTX still used.";
                    } else {
                        LOG(ERROR) << "SSL_CTX: Found but none used for SNI '" << serverNameIndication << '"';
                        ret = SSL_CLIENT_HELLO_ERROR;
                        *al = SSL_AD_UNRECOGNIZED_NAME;
                    }
                } else if (!socketAcceptor->forceSni) {
                    LOG(WARNING) << "SSL_CTX: Not found for SNI '" << serverNameIndication << "'. Master SSL_CTX still used.";
                } else {
                    LOG(ERROR) << "SSL_CTX: Not found for SNI '" << serverNameIndication << "'.";
                    ret = SSL_CLIENT_HELLO_ERROR;
                    *al = SSL_AD_UNRECOGNIZED_NAME;
                }
            } else {
                LOG(INFO) << "ServerNameIndication: client did not request a concrete server name";
            }

            return ret;
        }

    private:
        SSL_CTX* masterSslCtx = nullptr;
        std::set<std::string> masterSslCtxDomains;

        std::shared_ptr<std::map<std::string, SSL_CTX*>> sniSslCtxs;
        bool forceSni = false;
    };

} // namespace core::socket::stream::tls

#endif // CORE_SOCKET_STREAM_TLS_SOCKETACCEPTOR_H
