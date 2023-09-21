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

#include "core/socket/stream/SocketConnector.hpp" // IWYU pragma: export
#include "core/socket/stream/tls/SocketConnector.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "core/socket/stream/tls/ssl_utils.h"
#include "log/Logger.h"

#include <openssl/ssl.h>    // IWYU pragma: export
#include <openssl/x509v3.h> // IWYU pragma: export

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace core::socket::stream::tls {

    template <typename PhysicalClientSocket, typename Config>
    core::socket::stream::tls::SocketConnector<PhysicalClientSocket, Config>::SocketConnector(
        const std::shared_ptr<SocketContextFactory>& socketContextFactory,
        const std::function<void(SocketConnection*)>& onConnect,
        const std::function<void(SocketConnection*)>& onConnected,
        const std::function<void(SocketConnection*)>& onDisconnect,
        const std::function<void(const core::ProgressLog&)>& onError,
        const std::shared_ptr<Config>& config)
        : Super(
              socketContextFactory,
              [onConnect, this](SocketConnection* socketConnection) -> void { // onConnect
                  socketConnection->startSSL(this->ctx, this->config->getInitTimeout(), this->config->getShutdownTimeout());
                  onConnect(socketConnection);
              },
              [onConnected, this](SocketConnection* socketConnection) -> void { // onConnect
                  SSL* ssl = socketConnection->getSSL();

                  if (ssl != nullptr) {
                      SSL_set_connect_state(ssl);
                      ssl_set_sni(ssl, this->config);

                      socketConnection->doSSLHandshake(
                          [onConnected, socketConnection]() -> void { // onSuccess
                              LOG(INFO) << "SSL/TLS initial handshake success";
                              onConnected(socketConnection);
                              socketConnection->onConnected();
                          },
                          [onError = this->onError, progressLog = Super::progressLog, config = this->config]() -> void { // onTimeout
                              LOG(WARNING) << "SSL/TLS initial handshake timed out";
                              onError(*progressLog.get());
                          },
                          [onError = this->onError, progressLog = Super::progressLog, config = this->config](
                              int sslErr) -> void { // onError
                              ssl_log("SSL/TLS initial handshake failed", sslErr);
                              onError(*progressLog.get());
                          });
                  } else {
                      socketConnection->close();
                      ssl_log_error("SSL/TLS initialization failed");
                      this->onError(*Super::progressLog.get());
                  }
              },
              [onDisconnect](SocketConnection* socketConnection) -> void { // onDisconnect
                  socketConnection->stopSSL();
                  onDisconnect(socketConnection);
              },
              onError,
              config) {
    }

    template <typename PhysicalClientSocket, typename Config>
    SocketConnector<PhysicalClientSocket, Config>::~SocketConnector() {
        if (ctx != nullptr) {
            ssl_ctx_free(ctx);
        }
    }

    template <typename PhysicalClientSocket, typename Config>
    void SocketConnector<PhysicalClientSocket, Config>::initConnectEvent() {
        if (!config->getDisabled()) {
            ctx = ssl_ctx_new(config);

            if (ctx != nullptr) {
                Super::initConnectEvent();
            } else {
                Super::onError(*Super::progressLog.get());
                Super::destruct();
            }
        } else {
            Super::initConnectEvent();
        }
    }

} // namespace core::socket::stream::tls
