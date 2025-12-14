/*
 * SNode.C - A Slim Toolkit for Network Communication
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

/*
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
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
              [onConnect, this](SocketConnection* socketConnection) { // onConnect
                  onConnect(socketConnection);

                  SSL* ssl = socketConnection->startSSL(socketConnection->getFd(), Super::config->getSslCtx());
                  if (ssl != nullptr) {
                      SSL_set_connect_state(ssl);
                      SSL_set_ex_data(ssl, 1, Super::config.get());

                      ssl_set_sni(ssl, Super::config->getSni());
                  }
              },
              [socketContextFactory, onConnected](SocketConnection* socketConnection) { // onConnected
                  LOG(TRACE) << socketConnection->getConnectionName() << " SSL/TLS: Start handshake";
                  if (!socketConnection->doSSLHandshake(
                          [socketContextFactory,
                           onConnected,
                           socketConnection]() { // onSuccess
                              LOG(DEBUG) << socketConnection->getConnectionName() << " SSL/TLS: Handshake success";

                              onConnected(socketConnection);

                              socketConnection->setSocketContext(socketContextFactory);
                          },
                          [socketConnection]() { // onTimeout
                              LOG(ERROR) << socketConnection->getConnectionName() << " SSL/TLS: Handshake timed out";

                              socketConnection->close();
                          },
                          [socketConnection](int sslErr) { // onError
                              ssl_log(socketConnection->getConnectionName() + " SSL/TLS: Handshake failed", sslErr);

                              socketConnection->close();
                          })) {
                      LOG(ERROR) << socketConnection->getConnectionName() + " SSL/TLS: Handshake failed";

                      socketConnection->close();
                  }
              },
              [onDisconnect](SocketConnection* socketConnection) { // onDisconnect
                  socketConnection->stopSSL();
                  onDisconnect(socketConnection);
              },
              onStatus,
              config) {
        if (core::eventLoopState() == core::State::RUNNING) {
            init();
        } else {
            Super::destruct();
        }
    }

    template <typename PhysicalSocketServer, typename Config>
    SocketConnector<PhysicalSocketServer, Config>::SocketConnector(const SocketConnector& socketConnector)
        : Super(socketConnector) {
        if (core::eventLoopState() == core::State::RUNNING) {
            init();
        } else {
            Super::destruct();
        }
    }

    template <typename PhysicalClientSocket, typename Config>
    void SocketConnector<PhysicalClientSocket, Config>::useNextSocketAddress() {
        new SocketConnector(*this);
    }

    template <typename PhysicalSocketClient, typename Config>
    void SocketConnector<PhysicalSocketClient, Config>::init() {
        if (core::eventLoopState() == core::State::RUNNING && !config->getDisabled()) {
            LOG(TRACE) << config->getInstanceName() << " SSL/TLS: SSL_CTX creating ...";

            if (config->getSslCtx() != nullptr) {
                LOG(DEBUG) << config->getInstanceName() << " SSL/TLS: SSL_CTX created";

                Super::init();
            } else {
                LOG(ERROR) << config->getInstanceName() << " SSL/TLS: SSL_CTX creation failed";

                Super::onStatus(config->Remote::getSocketAddress(), core::socket::STATE_FATAL);
                Super::destruct();
            }
        } else {
            Super::init();
        }
    }

} // namespace core::socket::stream::tls
