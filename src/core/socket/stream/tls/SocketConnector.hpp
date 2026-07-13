/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
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
#include <unordered_map>
#include <utility>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace core::socket::stream::tls {

    namespace {
        inline std::unordered_map<const void*, bool>& tlsSetupState() {
            static std::unordered_map<const void*, bool> state;
            return state;
        }

        inline void setTlsSetupReady(const void* connection, bool ready) {
            tlsSetupState()[connection] = ready;
        }

        inline bool takeTlsSetupReady(const void* connection) {
            const auto it = tlsSetupState().find(connection);
            const bool ready = it != tlsSetupState().end() && it->second;
            if (it != tlsSetupState().end()) {
                tlsSetupState().erase(it);
            }
            return ready;
        }

        inline void closeAfterTlsSetupFailure(auto* socketConnection, const std::string& reason) {
            static_cast<core::socket::stream::SocketConnection*>(socketConnection)->log().error("SSL/TLS: {}", reason);
            socketConnection->close();
        }

        template <typename Config>
        std::string logicalRemoteHost(const std::shared_ptr<Config>& config) {
            if constexpr (requires { config->Remote::getHost(); }) {
                return config->Remote::getHost();
            } else {
                return {};
            }
        }

        template <typename Config>
        std::string resolveVerifyHost(const std::shared_ptr<Config>& config) {
            const std::string configuredVerifyHost = config->getVerifyHost();
            return !configuredVerifyHost.empty() ? configuredVerifyHost : logicalRemoteHost(config);
        }

        template <typename Config>
        std::string resolveSni(const std::shared_ptr<Config>& config) {
            const std::string configuredSni = config->getSni();
            if (!configuredSni.empty()) {
                return configuredSni;
            }
            const std::string remoteHost = logicalRemoteHost(config);
            return !remoteHost.empty() && !ssl_is_ip_address(remoteHost) ? remoteHost : std::string{};
        }
    } // namespace

    template <typename PhysicalClientSocket, typename Config>
    SocketConnector<PhysicalClientSocket, Config>::SocketConnector(
        const std::shared_ptr<SocketContextFactory>& socketContextFactory,
        const std::function<void(SocketConnection*)>& onConnect,
        const std::function<void(SocketConnection*)>& onConnected,
        const std::function<void(SocketConnection*)>& onDisconnect,
        const std::function<void(core::eventreceiver::ConnectEventReceiver*)>& onInitState,
        const std::function<void(const SocketAddress&, core::socket::State)>& onStatus,
        const std::shared_ptr<Config>& config)
        : Super(
              [onConnect, this](SocketConnection* socketConnection) { // onConnect
                  onConnect(socketConnection);

                  setTlsSetupReady(socketConnection, false);
                  SSL* ssl = socketConnection->startSSL(socketConnection->getFd(), Super::config->getSslCtx());
                  if (ssl == nullptr) {
                      closeAfterTlsSetupFailure(socketConnection, "startSSL failed");
                      return;
                  }

                  SSL_set_connect_state(ssl);
                  if (SSL_set_ex_data(ssl, 1, Super::config.get()) != 1) {
                      closeAfterTlsSetupFailure(socketConnection, "failed to configure SSL ex-data");
                      return;
                  }

                  const std::string sni = resolveSni(Super::config);
                  const std::string peerIdentity = resolveVerifyHost(Super::config);
                  if (!ssl_set_sni(ssl, sni)) {
                      closeAfterTlsSetupFailure(socketConnection, "SNI setup failed");
                      return;
                  }
                  if (!ssl_set_peer_identity(ssl, peerIdentity)) {
                      closeAfterTlsSetupFailure(socketConnection, "peer identity setup failed");
                      return;
                  }
                  setTlsSetupReady(socketConnection, true);
              },
              [socketContextFactory, onConnected](SocketConnection* socketConnection) { // onConnected
                  if (!takeTlsSetupReady(socketConnection)) {
                      static_cast<core::socket::stream::SocketConnection*>(socketConnection)
                          ->log()
                          .debug("SSL/TLS: Skipping handshake because setup did not complete");
                      return;
                  }
                  static_cast<core::socket::stream::SocketConnection*>(socketConnection)->log().trace("SSL/TLS: Start handshake");
                  if (!socketConnection->doSSLHandshake(
                          [socketContextFactory,
                           onConnected,
                           socketConnection,
                           log = static_cast<core::socket::stream::SocketConnection*>(socketConnection)->log()]() { // onSuccess
                              log.debug("SSL/TLS: Handshake success");

                              onConnected(socketConnection);

                              socketConnection->setSocketContext(socketContextFactory);
                          },
                          [socketConnection,
                           log = static_cast<core::socket::stream::SocketConnection*>(socketConnection)->log()]() { // onTimeout
                              log.error("SSL/TLS: Handshake timed out");

                              socketConnection->close();
                          },
                          [socketConnection](int sslErr) { // onError
                              ssl_log(socketConnection->getConnectionName() + " SSL/TLS: Handshake failed", sslErr);

                              socketConnection->close();
                          })) {
                      static_cast<core::socket::stream::SocketConnection*>(socketConnection)->log().error("SSL/TLS: Handshake failed");

                      socketConnection->close();
                  }
              },
              [onDisconnect](SocketConnection* socketConnection) { // onDisconnect
                  takeTlsSetupReady(socketConnection);
                  socketConnection->stopSSL();
                  onDisconnect(socketConnection);
              },
              onInitState,
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
            this->log().trace("SSL/TLS: SSL_CTX creating ...");

            if (config->getSslCtx() != nullptr) {
                this->log().debug("SSL/TLS: SSL_CTX created");

                Super::init();
            } else {
                this->log().error("SSL/TLS: SSL_CTX creation failed");

                Super::onStatus(config->Remote::getSocketAddress(), core::socket::STATE_FATAL);
                Super::destruct();
            }
        } else {
            Super::init();
        }
    }

} // namespace core::socket::stream::tls
