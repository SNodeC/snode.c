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

#ifndef CORE_SOCKET_STREAM_TLS_SOCKETCONNECTOR_H
#define CORE_SOCKET_STREAM_TLS_SOCKETCONNECTOR_H

#include "core/socket/stream/SocketConnector.h"
#include "core/socket/stream/tls/SocketConnection.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "core/socket/stream/tls/ssl_utils.h"

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::socket::stream::tls {

    template <typename SocketClientT>
    class SocketConnector : protected core::socket::stream::SocketConnector<SocketClientT, core::socket::stream::tls::SocketConnection> {
    private:
        using Super = core::socket::stream::SocketConnector<SocketClientT, core::socket::stream::tls::SocketConnection>;
        using SocketClient = SocketClientT;
        using SocketAddress = typename Super::SocketAddress;
        using Config = typename Super::Config;

    public:
        using SocketConnection = typename Super::SocketConnection;

        SocketConnector(const std::shared_ptr<core::socket::SocketContextFactory>& socketContextFactory,
                        const std::function<void(SocketConnection*)>& onConnect,
                        const std::function<void(SocketConnection*)>& onConnected,
                        const std::function<void(SocketConnection*)>& onDisconnect,
                        const std::map<std::string, std::any>& options)
            : Super(
                  socketContextFactory,
                  onConnect,
                  [onConnected, this](SocketConnection* socketConnection) -> void { // onConnect
                      SSL* ssl = socketConnection->startSSL(this->ctx, this->config->getInitTimeout(), this->config->getShutdownTimeout());

                      if (ssl != nullptr) {
                          SSL_set_connect_state(ssl);
                          ssl_set_sni(ssl, this->options);

                          socketConnection->doSSLHandshake(
                              [onConnected, socketConnection]() -> void { // onSuccess
                                  LOG(INFO) << "SSL/TLS initial handshake success";
                                  onConnected(socketConnection);
                                  socketConnection->onConnected();
                              },
                              [this]() -> void { // onTimeout
                                  LOG(WARNING) << "SSL/TLS initial handshake timed out";
                                  this->onError(this->config->getRemoteAddress(), ETIMEDOUT);
                              },
                              [this](int sslErr) -> void { // onError
                                  ssl_log("SSL/TLS initial handshake failed", sslErr);
                                  this->onError(this->config->getRemoteAddress(), -sslErr);
                              });
                      } else {
                          socketConnection->close();
                          ssl_log_error("SSL/TLS initialization failed");
                          this->onError(this->config->getRemoteAddress(), -SSL_ERROR_SSL);
                      }
                  },
                  [onDisconnect](SocketConnection* socketConnection) -> void { // onDisconnect
                      socketConnection->stopSSL();
                      onDisconnect(socketConnection);
                  },
                  options)
            , ctx(ssl_ctx_new(options, false)) {
        }

        ~SocketConnector() override {
            ssl_ctx_free(ctx);
        }

        void connect(const std::shared_ptr<Config>& clientConfig, const std::function<void(const SocketAddress&, int)>& onError) {
            if (ctx == nullptr) {
                errno = EINVAL;
                onError(clientConfig->getRemoteAddress(), errno);
                Super::destruct();
            } else {
                Super::connect(clientConfig, onError);
            }
        }

    private:
        SSL_CTX* ctx = nullptr;
    };

} // namespace core::socket::stream::tls

#endif // CORE_SOCKET_STREAM_TLS_SOCKETCONNECTOR_H
