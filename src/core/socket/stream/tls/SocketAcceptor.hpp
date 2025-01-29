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

#include "core/socket/stream/SocketAcceptor.hpp"
#include "core/socket/stream/tls/SocketAcceptor.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "core/socket/stream/tls/ssl_utils.h"
#include "log/Logger.h"

#include <algorithm>
#include <openssl/ssl.h>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::socket::stream::tls {

    template <typename PhysicalServerSocket, typename Config>
    core::socket::stream::tls::SocketAcceptor<PhysicalServerSocket, Config>::SocketAcceptor(
        const std::shared_ptr<SocketContextFactory>& socketContextFactory,
        const std::function<void(SocketConnection*)>& onConnect,
        const std::function<void(SocketConnection*)>& onConnected,
        const std::function<void(SocketConnection*)>& onDisconnect,
        const std::function<void(const SocketAddress&, core::socket::State)>& onStatus,
        const std::shared_ptr<Config>& config)
        : Super(
              socketContextFactory,
              [onConnect, this](SocketConnection* socketConnection) { // onConnect
                  onConnect(socketConnection);

                  SSL* ssl = socketConnection->startSSL(socketConnection->getFd(),
                                                        Super::config->getSslCtx(),
                                                        Super::config->getInitTimeout(),
                                                        Super::config->getShutdownTimeout(),
                                                        !Super::config->getNoCloseNotifyIsEOF());
                  if (ssl != nullptr) {
                      SSL_set_accept_state(ssl);
                  }
              },
              [socketContextFactory, onConnected](SocketConnection* socketConnection) { // on Connected
                  LOG(TRACE) << socketConnection->getConnectionName() << " SSL/TLS: Start handshake";
                  if (!socketConnection->doSSLHandshake(
                          [socketContextFactory,
                           onConnected,
                           socketConnection]() { // onSuccess
                              LOG(DEBUG) << socketConnection->getConnectionName() << " SSL/TLS: Handshake success";

                              onConnected(socketConnection);

                              socketConnection->connectSocketContext(socketContextFactory);
                          },
                          [socketConnection]() { // onTimeout
                              LOG(ERROR) << socketConnection->getConnectionName() << "SSL/TLS: Handshake timed out";

                              socketConnection->close();
                          },
                          [socketConnection](int sslErr) { //
                              ssl_log(socketConnection->getConnectionName() + " SSL/TLS: Handshake failed", sslErr);

                              socketConnection->close();
                          })) {
                      LOG(ERROR) << socketConnection->getConnectionName() + " SSL/TLS: Handshake failed";

                      socketConnection->close();
                  }
              },
              [onDisconnect, instanceName = config->getInstanceName()](SocketConnection* socketConnection) { // onDisconnect
                  socketConnection->stopSSL();
                  onDisconnect(socketConnection);
              },
              onStatus,
              config) {
    }

    template <typename PhysicalSocketServer, typename Config>
    SocketAcceptor<PhysicalSocketServer, Config>::SocketAcceptor(const SocketAcceptor& socketAcceptor)
        : Super(socketAcceptor) {
    }

    template <typename PhysicalClientSocket, typename Config>
    void SocketAcceptor<PhysicalClientSocket, Config>::useNextSocketAddress() {
        new SocketAcceptor(*this);
    }

    template <typename PhysicalSocketServer, typename Config>
    void SocketAcceptor<PhysicalSocketServer, Config>::init() {
        if (!config->getDisabled()) {
            LOG(TRACE) << config->getInstanceName() << " SSL/TLS: SSL_CTX creating ...";

            if (config->getSslCtx() != nullptr) {
                LOG(DEBUG) << config->getInstanceName() << " SSL/TLS: SSL_CTX created";
                Super::init();
            } else {
                LOG(ERROR) << config->getInstanceName() << " SSL/TLS: SSL/TLS creation failed";

                Super::onStatus(Super::config->Local::getSocketAddress(), core::socket::STATE_ERROR);
                Super::destruct();
            }
        } else {
            Super::init();
        }
    }

} // namespace core::socket::stream::tls
