/*
 * SNode.C - a slim toolkit for network communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024
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
// #include <openssl/x509v3.h>
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
              [onConnect, this](SocketConnection* socketConnection) -> void { // onConnect
                  onConnect(socketConnection);

                  SSL* ssl = socketConnection->startSSL(socketConnection->getFd(),
                                                        this->config->getSslCtx(),
                                                        Super::config->getInitTimeout(),
                                                        Super::config->getShutdownTimeout());

                  if (ssl != nullptr) {
                      SSL_set_accept_state(ssl);
                  }
              },
              [socketContextFactory, onConnected, this](SocketConnection* socketConnection) -> void { // on Connected
                  if (socketConnection->doSSLHandshake(
                          [socketContextFactory, onConnected, socketConnection, instanceName = Super::config->getInstanceName()]()
                              -> void { // onSuccess
                              LOG(TRACE) << instanceName << " SSL/TLS: Initial handshake success";

                              onConnected(socketConnection);
                              socketConnection->connectSocketContext(socketContextFactory);
                          },
                          [socketConnection, instanceName = Super::config->getInstanceName()]() -> void { // onTimeout
                              LOG(TRACE) << instanceName << "SSL/TLS: Initial handshake timed out";

                              socketConnection->close();
                          },
                          [socketConnection, instanceName = Super::config->getInstanceName()](int sslErr) -> void { //
                              ssl_log(instanceName + " SSL/TLS: Initial handshake failed", sslErr);

                              socketConnection->close();
                          })) {
                      LOG(TRACE) << Super::config->getInstanceName() << " SSL/TLS: Initial handshake running";
                  } else {
                      ssl_log_error(Super::config->getInstanceName() + " SSL/TLS: Initialization failed");

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
    SocketAcceptor<PhysicalSocketServer, Config>::SocketAcceptor(const SocketAcceptor& socketAcceptor)
        : core::Observer(socketAcceptor)
        , Super(socketAcceptor) {
    }

    template <typename PhysicalSocketServer, typename Config>
    SocketAcceptor<PhysicalSocketServer, Config>::~SocketAcceptor() {
    }

    template <typename PhysicalClientSocket, typename Config>
    void core::socket::stream::tls::SocketAcceptor<PhysicalClientSocket, Config>::useNextSocketAddress() {
        new SocketAcceptor(*this);
    }

    template <typename PhysicalSocketServer, typename Config>
    void SocketAcceptor<PhysicalSocketServer, Config>::initAcceptEvent() {
        if (!config->getDisabled()) {
            LOG(TRACE) << config->getInstanceName() << " SSL/TLS: Initializing";

            masterSslCtx = config->getSslCtx();

            if (masterSslCtx != nullptr) {
                LOG(TRACE) << config->getInstanceName() << " SSL/TLS: Master SSL_CTX created";
                Super::initAcceptEvent();
            } else {
                LOG(TRACE) << config->getInstanceName() << " SSL/TLS: Master SSL/TLS creation failed";

                Super::onStatus(Super::config->Local::getSocketAddress(), core::socket::STATE_ERROR);
                Super::destruct();
            }
        } else {
            Super::initAcceptEvent();
        }
    }

} // namespace core::socket::stream::tls
