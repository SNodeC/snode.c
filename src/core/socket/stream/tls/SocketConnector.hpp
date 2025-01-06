/*
 * SNode.C - a slim toolkit for network communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025
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

#include "core/socket/stream/SocketConnector.hpp"
#include "core/socket/stream/tls/SocketConnector.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "core/socket/stream/tls/ssl_utils.h"
#include "log/Logger.h"

#include <memory>
#include <openssl/ssl.h>
#include <openssl/x509v3.h>
#include <string>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace core::socket::stream::tls {

    template <typename PhysicalClientSocket, typename Config>
    SocketConnector<PhysicalClientSocket, Config>::SocketConnector(
        const std::shared_ptr<SocketContextFactory>& socketContextFactory,
        const std::function<void(SocketConnection*)>& onConnect,
        const std::function<void(SocketConnection*)>& onConnected,
        const std::function<void(SocketConnection*)>& onDisconnect,
        const std::function<void(const SocketAddress&, core::socket::State)>& onStatus,
        const std::shared_ptr<Config>& config)
        : Super(
              socketContextFactory,
              [onConnect, this](SocketConnection* socketConnection) -> void { // onConnect
                  SSL* ssl = socketConnection->startSSL(socketConnection->getFd(),
                                                        this->config->getSslCtx(),
                                                        Super::config->getInitTimeout(),
                                                        Super::config->getShutdownTimeout(),
                                                        !Super::config->getNoCloseNotifyIsEOF());
                  if (ssl != nullptr) {
                      SSL_set_connect_state(ssl);
                      ssl_set_sni(ssl, Super::config->getSni());
                  }

                  onConnect(socketConnection);
              },
              [socketContextFactory, onConnected, this](SocketConnection* socketConnection) -> void { // onConnected
                  LOG(TRACE) << Super::config->getInstanceName() << " SSL/TLS: Start handshake";
                  if (!socketConnection->doSSLHandshake(
                          [socketContextFactory, onConnected, socketConnection, instanceName = Super::config->getInstanceName()]()
                              -> void { // onSuccess
                              LOG(TRACE) << instanceName << " SSL/TLS: Handshake success";

                              socketConnection->connectSocketContext(socketContextFactory);

                              onConnected(socketConnection);
                          },
                          [socketConnection, instanceName = Super::config->getInstanceName()]() -> void { // onTimeout
                              LOG(TRACE) << instanceName << " SSL/TLS: Handshake timed out";

                              socketConnection->close();
                          },
                          [socketConnection, instanceName = Super::config->getInstanceName()](int sslErr) -> void { // onError
                              ssl_log(instanceName + " SSL/TLS: Handshake failed", sslErr);

                              socketConnection->close();
                          })) {
                      LOG(TRACE) << Super::config->getInstanceName() + " SSL/TLS: Handshake failed";

                      socketConnection->close();
                  }
              },
              [onDisconnect, instanceName = config->getInstanceName()](SocketConnection* socketConnection) -> void { // onDisconnect
                  socketConnection->stopSSL();
                  onDisconnect(socketConnection);
              },
              onStatus,
              config) {
    }

    template <typename PhysicalSocketServer, typename Config>
    SocketConnector<PhysicalSocketServer, Config>::SocketConnector(const SocketConnector& socketConnector)
        : Super::SocketConnector(socketConnector) {
    }

    template <typename PhysicalClientSocket, typename Config>
    void SocketConnector<PhysicalClientSocket, Config>::useNextSocketAddress() {
        new SocketConnector(*this);
    }

    template <typename PhysicalSocketClient, typename Config>
    void SocketConnector<PhysicalSocketClient, Config>::init() {
        if (!config->getDisabled()) {
            LOG(TRACE) << config->getInstanceName() << " SSL/TLS: SSL_CTX creating ...";

            const SSL_CTX* ctx = config->getSslCtx();

            if (ctx != nullptr) {
                LOG(TRACE) << config->getInstanceName() << " SSL/TLS: SSL_CTX created";
                Super::init();
            } else {
                LOG(TRACE) << config->getInstanceName() << " SSL/TLS: SSL_CTX creation failed";

                Super::onStatus(config->Remote::getSocketAddress(), core::socket::STATE_FATAL);
                Super::destruct();
            }
        } else {
            Super::init();
        }
    }

} // namespace core::socket::stream::tls
