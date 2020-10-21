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

#ifndef NET_SOCKET_SOCK_STREAM_TLS_SOCKETCONNECTOR_H
#define NET_SOCKET_SOCK_STREAM_TLS_SOCKETCONNECTOR_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "Logger.h"
#include "SocketConnection.h"
#include "TLSHandshake.h"
#include "socket/sock_stream/SocketConnector.h"
#include "ssl_utils.h"

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

        protected:
            SocketConnector(const std::function<void(SocketConnection* socketConnection)>& onConstruct,
                            const std::function<void(SocketConnection* socketConnection)>& onDestruct,
                            const std::function<void(SocketConnection* socketConnection)>& onConnect,
                            const std::function<void(SocketConnection* socketConnection)>& onDisconnect,
                            const std::function<void(SocketConnection* socketConnection, const char* junk, ssize_t junkLen)>& onRead,
                            const std::function<void(SocketConnection* socketConnection, int errnum)>& onReadError,
                            const std::function<void(SocketConnection* socketConnection, int errnum)>& onWriteError,
                            const std::map<std::string, std::any>& options)
                : stream::SocketConnector<SocketConnection>(
                      [onConstruct](SocketConnection* socketConnection) -> void { // onConstruct
                          onConstruct(socketConnection);
                      },
                      [onDestruct](SocketConnection* socketConnection) -> void { // onDestruct
                          onDestruct(socketConnection);
                      },
                      [onConnect, &onError = this->onError, &ctx = this->ctx](SocketConnection* socketConnection) -> void { // onConnect
                          SSL* ssl = socketConnection->startSSL(ctx);

                          if (ssl != nullptr) {
                              int ret = SSL_connect(ssl);
                              int sslErr = SSL_get_error(ssl, ret);

                              if (sslErr == SSL_ERROR_WANT_READ || sslErr == SSL_ERROR_WANT_WRITE) {
                                  socketConnection->ReadEventReceiver::suspend();

                                  TLSHandshake::doHandshake(
                                      ssl,
                                      [&onConnect, socketConnection](void) -> void {
                                          socketConnection->ReadEventReceiver::resume();
                                          onConnect(socketConnection);
                                      },
                                      [socketConnection](void) -> void {
                                          socketConnection->ReadEventReceiver::disable();
                                          PLOG(ERROR) << "TLS handshake timeout";
                                      },
                                      [socketConnection, &onError](int sslErr) -> void {
                                          socketConnection->ReadEventReceiver::disable();
                                          PLOG(ERROR) << "TLS handshake failed: " << ERR_error_string(sslErr, nullptr);
                                          onError(-sslErr);
                                      });
                              } else if (sslErr == SSL_ERROR_NONE) {
                                  onConnect(socketConnection);
                              } else {
                                  socketConnection->ReadEventReceiver::disable();
                                  PLOG(ERROR) << "TLS connect failed: " << ERR_error_string(ERR_get_error(), nullptr);
                                  onError(-sslErr);
                              }
                          } else {
                              socketConnection->ReadEventReceiver::disable();
                              unsigned long sslErr = ERR_get_error();
                              PLOG(ERROR) << "TLS handshake failed: " << ERR_error_string(sslErr, nullptr);
                              onError(-sslErr);
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
                ctx = SSL_CTX_new(TLS_client_method());
                sslErr = ssl_init_ctx(ctx, options, false);
            }

            ~SocketConnector() override {
                if (ctx != nullptr) {
                    SSL_CTX_free(ctx);
                }
            }

            void connect(const SocketAddress& remoteAddress,
                         const std::function<void(int err)>& onError,
                         const SocketAddress& bindAddress = SocketAddress()) override {
                if (sslErr != 0) {
                    onError(-sslErr);
                } else {
                    stream::SocketConnector<SocketConnection>::connect(remoteAddress, onError, bindAddress);
                }
            }

            SSL_CTX* ctx = nullptr;
            unsigned long sslErr = 0;

            template <typename SocketConnector>
            friend class stream::SocketClient;
        };

    } // namespace tls

} // namespace net::socket::stream

#endif // NET_SOCKET_SOCK_STREAM_TLS_SOCKETCONNECTOR_H
