/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020 Volker Christian <me@vchrist.at>
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

#ifndef NET_SOCKET_STREAM_TLS_SOCKETCONNECTOR_H
#define NET_SOCKET_STREAM_TLS_SOCKETCONNECTOR_H

#include "log/Logger.h"
#include "net/socket/stream/SocketConnector.h"
#include "net/socket/stream/tls/SocketConnection.h"
#include "net/socket/stream/tls/TLSHandshake.h"
#include "net/socket/stream/tls/ssl_utils.h"
#include "ssl_utils.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::socket::stream {

    template <typename SocketConnector>
    class SocketClient;

    namespace tls {

        template <typename SocketT>
        class SocketConnector : public stream::SocketConnector<stream::tls::SocketConnection<SocketT>> {
        public:
            using SocketConnection = stream::tls::SocketConnection<SocketT>;
            using Socket = typename SocketConnection::Socket;
            using SocketAddress = typename Socket::SocketAddress;

            SocketConnector(const std::function<void(const SocketAddress& localAddress, const SocketAddress& remoteAddress)>& onConstruct,
                            const std::function<void(SocketConnection* socketConnection)>& onConnected,
                            const std::function<void(SocketConnection* socketConnection)>& onDisconnect,
                            const std::function<void(SocketConnection* socketConnection, const char* junk, std::size_t junkLen)>& onRead,
                            const std::function<void(SocketConnection* socketConnection, int errnum)>& onReadError,
                            const std::function<void(SocketConnection* socketConnection, int errnum)>& onWriteError,
                            const std::map<std::string, std::any>& options)
                : stream::SocketConnector<SocketConnection>(
                      onConstruct,
                      [onConnected, &onError = this->onError, &ctx = this->ctx, this, &options = this->options](
                          SocketConnection* socketConnection) -> void { // onConnect
                          SSL* ssl = socketConnection->startSSL(ctx);

                          if (ssl != nullptr) {
                              ssl_set_sni(ssl, options);

                              SSL_set_connect_state(ssl);

                              socketConnection->doSSLHandshake(
                                  [ssl, onConnected, socketConnection](void) -> void { // onSuccess
                                      LOG(INFO) << "SSL/TLS initial handshake success";
                                      onConnected(socketConnection);
                                  },
                                  [socketConnection, onError](void) -> void { // onTimeout
                                      LOG(WARNING) << "SSL/TLS initial handshake timed out";
                                      onError(ETIMEDOUT);
                                  },
                                  [socketConnection, onError](int sslErr) -> void { // onError
                                      ssl_log("SSL/TLS initial handshake failed", sslErr);
                                      onError(-sslErr);
                                  });
                          } else {
                              socketConnection->SocketConnection::SocketReader::disable();
                              ssl_log_error("SSL/TLS initialization failed");
                              onError(-SSL_ERROR_SSL);
                          }
                      },
                      [onDisconnect](SocketConnection* socketConnection) -> void { // onDisconnect
                          socketConnection->stopSSL();
                          onDisconnect(socketConnection);
                      },
                      onRead,       // onRead
                      onReadError,  // onReadError
                      onWriteError, // onWriteError
                      options) {
                ctx = ssl_ctx_new(options, false);
            }

            ~SocketConnector() override {
                ssl_ctx_free(ctx);
            }

            void
            connect(const SocketAddress& remoteAddress, const SocketAddress& bindAddress, const std::function<void(int err)>& onError) {
                if (ctx == nullptr) {
                    errno = EINVAL;
                    onError(errno);
                } else {
                    stream::SocketConnector<SocketConnection>::connect(remoteAddress, bindAddress, onError);
                }
            }

        protected:
            SSL_CTX* ctx = nullptr;
        };

    } // namespace tls

} // namespace net::socket::stream

#endif // NET_SOCKET_STREAM_TLS_SOCKETCONNECTOR_H
