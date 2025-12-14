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
    SocketAcceptor<PhysicalServerSocket, Config>::SocketAcceptor(
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
                      SSL_set_accept_state(ssl);
                      SSL_set_ex_data(ssl, 1, Super::config.get());
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

                              socketConnection->setSocketContext(socketContextFactory);
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
        if (core::eventLoopState() == core::State::RUNNING) {
            init();
        } else {
            Super::destruct();
        }
    }

    template <typename PhysicalSocketServer, typename Config>
    SocketAcceptor<PhysicalSocketServer, Config>::SocketAcceptor(const SocketAcceptor& socketAcceptor)
        : Super(socketAcceptor) {
        if (core::eventLoopState() == core::State::RUNNING) {
            init();
        } else {
            Super::destruct();
        }
    }

    template <typename PhysicalClientSocket, typename Config>
    void SocketAcceptor<PhysicalClientSocket, Config>::useNextSocketAddress() {
        new SocketAcceptor(*this);
    }

    template <typename PhysicalSocketServer, typename Config>
    void SocketAcceptor<PhysicalSocketServer, Config>::init() {
        if (core::eventLoopState() == core::State::RUNNING && !config->getDisabled()) {
            LOG(TRACE) << config->getInstanceName() << " SSL/TLS: SSL_CTX creating ...";
            SSL_CTX* sslCtx = config->getSslCtx();

            if (sslCtx != nullptr) {
                LOG(DEBUG) << config->getInstanceName() << " SSL/TLS: SSL_CTX created";

                SSL_CTX_set_client_hello_cb(sslCtx, clientHelloCallback, nullptr);

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

    template <typename PhysicalSocketServer, typename Config>
    int SocketAcceptor<PhysicalSocketServer, Config>::clientHelloCallback(SSL* ssl, int* al, [[maybe_unused]] void* arg) {
        int ret = SSL_CLIENT_HELLO_SUCCESS;

        const std::string connectionName = *static_cast<std::string*>(SSL_get_ex_data(ssl, 0));
        Config* config = static_cast<Config*>(SSL_get_ex_data(ssl, 1));

        std::string serverNameIndication = core::socket::stream::tls::ssl_get_servername_from_client_hello(ssl);

        if (!serverNameIndication.empty()) {
            SSL_CTX* sniSslCtx = config->getSniCtx(serverNameIndication);

            if (sniSslCtx != nullptr) {
                LOG(DEBUG) << connectionName << " SSL/TLS: Setting sni certificate for '" << serverNameIndication << "'";
                core::socket::stream::tls::ssl_set_ssl_ctx(ssl, sniSslCtx);
            } else if (config->getForceSni()) {
                LOG(ERROR) << connectionName << " SSL/TLS: No sni certificate found for '" << serverNameIndication
                           << "' but forceSni set - terminating";
                ret = SSL_CLIENT_HELLO_ERROR;
                *al = SSL_AD_UNRECOGNIZED_NAME;
            } else {
                LOG(WARNING) << connectionName << " SSL/TLS: No sni certificate found for '" << serverNameIndication
                             << "'. Still using master certificate";
            }
        } else {
            LOG(DEBUG) << connectionName << " SSL/TLS: No sni certificate requested from client. Still using master certificate";
        }

        return ret;
    }

} // namespace core::socket::stream::tls
