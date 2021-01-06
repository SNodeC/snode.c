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

#include <cstddef>

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

            SocketConnector(const std::function<void(SocketConnection* socketConnection)>& onConstruct,
                            const std::function<void(SocketConnection* socketConnection)>& onDestruct,
                            const std::function<void(SocketConnection* socketConnection)>& onConnect,
                            const std::function<void(SocketConnection* socketConnection)>& onDisconnect,
                            const std::function<void(SocketConnection* socketConnection, const char* junk, std::size_t junkLen)>& onRead,
                            const std::function<void(SocketConnection* socketConnection, int errnum)>& onReadError,
                            const std::function<void(SocketConnection* socketConnection, int errnum)>& onWriteError,
                            const std::map<std::string, std::any>& options)
                : stream::SocketConnector<SocketConnection>(
                      onConstruct,
                      onDestruct,
                      [onConnect, &onError = this->onError, &ctx = this->ctx, this](
                          SocketConnection* socketConnection) -> void { // onConnect
                          stream::SocketConnector<SocketConnection>::ConnectEventReceiver::suspend();

                          SSL* ssl = socketConnection->startSSL(ctx);

                          if (ssl != nullptr) {
                              socketConnection->SocketConnection::SocketReader::suspend();

                              SSL_set_connect_state(ssl);

                              TLSHandshake::doHandshake(
                                  ssl,
                                  [&onConnect, socketConnection, this](void) -> void { // onSuccess
                                      stream::SocketConnector<SocketConnection>::ConnectEventReceiver::disable();
                                      socketConnection->SocketConnection::SocketReader::resume();
                                      onConnect(socketConnection);
                                  },
                                  [socketConnection, &onError, this](void) -> void { // onTimeout
                                      PLOG(ERROR) << "SSL/TLS handshake timeout";
                                      stream::SocketConnector<SocketConnection>::ConnectEventReceiver::disable();
                                      socketConnection->SocketConnection::SocketReader::disable();
                                      onError(ETIMEDOUT);
                                  },
                                  [socketConnection, &onError, this](int sslErr) -> void { // onError
                                      ssl_log("SSL/TLS handshake failed", -sslErr);
                                      socketConnection->setSSLError(-sslErr);
                                      stream::SocketConnector<SocketConnection>::ConnectEventReceiver::disable();
                                      socketConnection->SocketConnection::SocketReader::disable();
                                      onError(sslErr);
                                  });
                          } else {
                              stream::SocketConnector<SocketConnection>::ConnectEventReceiver::disable();
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

            void connect(const SocketAddress& remoteAddress,
                         const SocketAddress& bindAddress,
                         const std::function<void(int err)>& onError) override {
                if (ctx == nullptr) {
                    errno = EINVAL;
                    onError(errno);
                } else {
                    stream::SocketConnector<SocketConnection>::connect(remoteAddress, bindAddress, onError);
                }
            }

        private:
            void connectEvent() override {
                int cErrno = stream::SocketConnector<SocketConnection>::tryToCompleteConnect();

                if (cErrno != EINPROGRESS && cErrno != 0) {
                    stream::SocketConnector<SocketConnection>::ConnectEventReceiver::disable();
                }
            }

            SSL_CTX* ctx = nullptr;
        };

    } // namespace tls

} // namespace net::socket::stream

#endif // NET_SOCKET_SOCK_STREAM_TLS_SOCKETCONNECTOR_H
