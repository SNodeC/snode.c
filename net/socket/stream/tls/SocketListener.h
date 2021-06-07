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

#ifndef NET_SOCKET_STREAM_TLS_SOCKETLISTENER_H
#define NET_SOCKET_STREAM_TLS_SOCKETLISTENER_H

#include "log/Logger.h"
#include "net/socket/stream/SocketListener.h"
#include "net/socket/stream/tls/SocketConnection.h"
#include "net/socket/stream/tls/TLSHandshake.h"
#include "net/socket/stream/tls/ssl_utils.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::socket::stream {

    template <typename SocketProtocol, typename SocketListener>
    class SocketServer;

    namespace tls {

        template <typename SocketT>
        class SocketListener : public stream::SocketListener<stream::tls::SocketConnection<SocketT>> {
        public:
            using SocketConnection = stream::tls::SocketConnection<SocketT>;
            using Socket = typename SocketConnection::Socket;
            using SocketAddress = typename Socket::SocketAddress;

            SocketListener(const std::shared_ptr<const SocketContextFactory>& socketContextFactory,
                           const std::function<void(const SocketAddress&, const SocketAddress&)>& onConnect,
                           const std::function<void(SocketConnection*)>& onConnected,
                           const std::function<void(SocketConnection*)>& onDisconnect,
                           const std::map<std::string, std::any>& options)
                : stream::SocketListener<SocketConnection>(
                      socketContextFactory,
                      onConnect,
                      [onConnected, &ctx = this->ctx](SocketConnection* socketConnection) -> void {
                          SSL* ssl = socketConnection->startSSL(ctx);

                          if (ssl != nullptr) {
                              SSL_CTX_set_tlsext_servername_arg(ctx, socketConnection);

                              SSL_set_accept_state(ssl);

                              socketConnection->doSSLHandshake(
                                  [&onConnected, socketConnection](void) -> void { // onSuccess
                                      LOG(INFO) << "SSL/TLS initial handshake success";
                                      onConnected(socketConnection);
                                  },
                                  [socketConnection](void) -> void { // onTimeout
                                      LOG(WARNING) << "SSL/TLS initial handshake timed out";
                                  },
                                  [socketConnection](int sslErr) -> void { // onError
                                      ssl_log("SSL/TLS initial handshake failed", sslErr);
                                  });
                          } else {
                              socketConnection->SocketConnection::SocketReader::disable();
                              ssl_log_error("SSL/TLS initialization failed");
                          }
                      },
                      [onDisconnect](SocketConnection* socketConnection) -> void { // onDisconnect
                          socketConnection->stopSSL();
                          onDisconnect(socketConnection);
                      },
                      options) {
                ctx = ssl_ctx_new(options, true);
                if (ctx != nullptr) {
                    SSL_CTX_set_tlsext_servername_callback(ctx, serverNameCallback);
                }
            }

            ~SocketListener() override {
                ssl_ctx_free(ctx);
            }

            void listen(const SocketAddress& localAddress, int backlog, const std::function<void(int)>& onError) {
                if (ctx == nullptr) {
                    errno = EINVAL;
                    onError(errno);
                    stream::SocketListener<SocketConnection>::destruct();
                } else {
                    stream::SocketListener<SocketConnection>::listen(localAddress, backlog, onError);
                }
            }

        protected:
            static int serverNameCallback(SSL* ssl, [[maybe_unused]] int* al, void* arg) {
                SocketConnection* socketConnection = static_cast<SocketConnection*>(arg);

                if (SSL_get_servername_type(ssl) != -1) {
                    socketConnection->serverNameIndication = SSL_get_servername(ssl, TLSEXT_NAMETYPE_host_name);
                    LOG(INFO) << "ServerNameIndication: " << socketConnection->serverNameIndication;
                }

                return SSL_TLSEXT_ERR_OK;
            }

            SSL_CTX* ctx = nullptr;
        };

    } // namespace tls

} // namespace net::socket::stream

#endif // NET_SOCKET_STREAM_TLS_SOCKETLISTENER_H
