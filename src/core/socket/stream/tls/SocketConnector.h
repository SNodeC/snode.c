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

#ifndef CORE_SOCKET_STREAM_TLS_SOCKETCONNECTOR_H
#define CORE_SOCKET_STREAM_TLS_SOCKETCONNECTOR_H

#include "core/socket/stream/SocketConnector.h"
#include "core/socket/stream/tls/SocketConnection.h" // IWYU pragma: export
#include "core/socket/stream/tls/ssl_utils.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::socket::stream::tls {

    template <typename SocketT>
    class SocketConnector : public core::socket::stream::SocketConnector<core::socket::stream::tls::SocketConnection<SocketT>> {
    private:
        using Super = core::socket::stream::SocketConnector<core::socket::stream::tls::SocketConnection<SocketT>>;

    public:
        using Socket = typename Super::Socket;
        using SocketConnection = typename Super::SocketConnection;
        using SocketAddress = typename Super::SocketAddress;

        SocketConnector(const std::shared_ptr<core::socket::SocketContextFactory>& socketContextFactory,
                        const std::function<void(SocketConnection*)>& onConnect,
                        const std::function<void(SocketConnection*)>& onConnected,
                        const std::function<void(SocketConnection*)>& onDisconnect,
                        const std::map<std::string, std::any>& options)
            : Super(
                  socketContextFactory,
                  onConnect,
                  [onConnected, this](SocketConnection* socketConnection) -> void { // onConnect
                      SSL* ssl = socketConnection->startSSL(this->ctx);

                      if (ssl != nullptr) {
                          ssl_set_sni(ssl, this->options);

                          SSL_set_connect_state(ssl);

                          socketConnection->doSSLHandshake(
                              [onConnected, socketConnection](void) -> void { // onSuccess
                                  LOG(INFO) << "SSL/TLS initial handshake success";
                                  onConnected(socketConnection);
                              },
                              [this](void) -> void { // onTimeout
                                  LOG(WARNING) << "SSL/TLS initial handshake timed out";
                                  this->onError(ETIMEDOUT);
                              },
                              [this](int sslErr) -> void { // onError
                                  ssl_log("SSL/TLS initial handshake failed", sslErr);
                                  this->onError(-sslErr);
                              });
                      } else {
                          socketConnection->SocketConnection::SocketReader::disable();
                          socketConnection->SocketConnection::SocketWriter::disable();
                          ssl_log_error("SSL/TLS initialization failed");
                          this->onError(-SSL_ERROR_SSL);
                      }
                  },
                  [onDisconnect](SocketConnection* socketConnection) -> void { // onDisconnect
                      socketConnection->stopSSL();
                      onDisconnect(socketConnection);
                  },
                  options) {
            ctx = ssl_ctx_new(options, false);
        }

        ~SocketConnector() override {
            ssl_ctx_free(ctx);
        }

        void connect(const SocketAddress& remoteAddress, const SocketAddress& bindAddress, const std::function<void(int)>& onError) {
            if (ctx == nullptr) {
                errno = EINVAL;
                onError(errno);
                Super::destruct();
            } else {
                Super::connect(remoteAddress, bindAddress, onError);
            }
        }

    protected:
        SSL_CTX* ctx = nullptr;
    };

} // namespace core::socket::stream::tls

#endif // CORE_SOCKET_STREAM_TLS_SOCKETCONNECTOR_H
