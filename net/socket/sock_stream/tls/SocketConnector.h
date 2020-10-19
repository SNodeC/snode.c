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

#include "Descriptor.h"
#include "SocketConnection.h"
#include "socket/sock_stream/SocketConnector.h"
#include "ssl_utils.h"
#include "timer/SingleshotTimer.h"

#define TLSCONNECT_TIMEOUT 10

namespace net::socket::stream::tls {

    template <typename SocketT>
    class SocketConnector : public net::socket::stream::SocketConnector<net::socket::stream::tls::SocketConnection<SocketT>> {
    public:
        using SocketConnection = net::socket::stream::tls::SocketConnection<SocketT>;
        using Socket = typename SocketConnection::Socket;

    protected:
        SocketConnector(const std::function<void(SocketConnection* socketConnection)>& onConstruct,
                        const std::function<void(SocketConnection* socketConnection)>& onDestruct,
                        const std::function<void(SocketConnection* socketConnection)>& onConnect,
                        const std::function<void(SocketConnection* socketConnection)>& onDisconnect,
                        const std::function<void(SocketConnection* socketConnection, const char* junk, ssize_t junkLen)>& onRead,
                        const std::function<void(SocketConnection* socketConnection, int errnum)>& onReadError,
                        const std::function<void(SocketConnection* socketConnection, int errnum)>& onWriteError,
                        const std::map<std::string, std::any>& options)
            : net::socket::stream::SocketConnector<SocketConnection>(
                  [onConstruct, &ctx = this->ctx](SocketConnection* socketConnection) -> void { // onConstruct
                      socketConnection->setSSL_CTX(ctx);
                      onConstruct(socketConnection);
                  },
                  [onDestruct](SocketConnection* socketConnection) -> void { // onDestruct
                      socketConnection->clearSSL_CTX();
                      onDestruct(socketConnection);
                  },
                  [onConnect, &onError = this->onError](SocketConnection* socketConnection) -> void { // onConnect
                      class TLSHandshaker
                          : public ReadEventReceiver
                          , public WriteEventReceiver
                          , public Descriptor {
                      public:
                          TLSHandshaker(SSL* ssl,
                                        int sslErr,
                                        SocketConnection* socketConnection,
                                        const std::function<void(SocketConnection* socketConnection)>& onConnect,
                                        const std::function<void(int err)>& onError)
                              : ssl(ssl)
                              , socketConnection(socketConnection)
                              , onConnect(onConnect)
                              , onError(onError)
                              , timeOut(timer::Timer::singleshotTimer(
                                    [this, socketConnection, onError](const void*) -> void {
                                        ReadEventReceiver::disable();
                                        WriteEventReceiver::disable();
                                        onError(ETIMEDOUT);
                                        socketConnection->ReadEventReceiver::disable();
                                    },
                                    (struct timeval){TLSCONNECT_TIMEOUT, 0},
                                    nullptr)) {
                              open(socketConnection->getFd(), FLAGS::dontClose);

                              if (ssl != nullptr) {
                                  if (sslErr == SSL_ERROR_WANT_READ) {
                                      ReadEventReceiver::enable();
                                  } else if (sslErr == SSL_ERROR_WANT_WRITE) {
                                      WriteEventReceiver::enable();
                                  } else {
                                      if (sslErr == SSL_ERROR_NONE) {
                                          onError(0);
                                          onConnect(socketConnection);
                                      } else {
                                          onError(-ERR_peek_error());
                                          socketConnection->ReadEventReceiver::disable();
                                      }
                                      timeOut.cancel();
                                      delete this;
                                  }
                              } else {
                                  onError(-ERR_peek_error());
                                  socketConnection->ReadEventReceiver::disable();
                                  timeOut.cancel();
                                  delete this;
                              }
                          }

                          void readEvent() override {
                              int ret = SSL_do_handshake(ssl);
                              int sslErr = SSL_get_error(ssl, ret);

                              if (sslErr != SSL_ERROR_WANT_READ) {
                                  if (sslErr == SSL_ERROR_WANT_WRITE) {
                                      ReadEventReceiver::disable();
                                      WriteEventReceiver::enable();
                                  } else {
                                      timeOut.cancel();
                                      ReadEventReceiver::disable();
                                      if (sslErr == SSL_ERROR_NONE) {
                                          onError(0);
                                          onConnect(socketConnection);
                                      } else {
                                          onError(-ERR_peek_error());
                                          socketConnection->ReadEventReceiver::disable();
                                      }
                                  }
                              }
                          }

                          void writeEvent() override {
                              int ret = SSL_do_handshake(ssl);
                              int sslErr = SSL_get_error(ssl, ret);

                              if (sslErr != SSL_ERROR_WANT_WRITE) {
                                  if (sslErr == SSL_ERROR_WANT_READ) {
                                      WriteEventReceiver::disable();
                                      ReadEventReceiver::enable();
                                  } else {
                                      timeOut.cancel();
                                      WriteEventReceiver::disable();
                                      if (sslErr == SSL_ERROR_NONE) {
                                          onError(0);
                                          onConnect(socketConnection);
                                      } else {
                                          onError(-ERR_peek_error());
                                          socketConnection->ReadEventReceiver::disable();
                                      }
                                  }
                              }
                          }

                          void unobserved() override {
                              delete this;
                          }

                          static void doHandshake(SSL* ssl,
                                                  int sslErr,
                                                  SocketConnection* socketConnection,
                                                  const std::function<void(SocketConnection* socketConnection)>& onConnect,
                                                  const std::function<void(int err)>& onError) {
                              new TLSHandshaker(ssl, sslErr, socketConnection, onConnect, onError);
                          }

                      private:
                          SSL* ssl = nullptr;
                          SocketConnection* socketConnection = nullptr;
                          std::function<void(SocketConnection* socketConnection)> onConnect;
                          std::function<void(int err)> onError;
                          timer::Timer& timeOut;
                      };

                      SSL* ssl = socketConnection->startSSL();
                      int sslErr = SSL_get_error(ssl, SSL_connect(ssl));

                      TLSHandshaker::doHandshake(ssl, sslErr, socketConnection, onConnect, onError);
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

        void connect(const typename SocketConnection::Socket::SocketAddress& remoteAddress,
                     const std::function<void(int err)>& onError,
                     const typename SocketConnection::Socket::SocketAddress& bindAddress =
                         typename SocketConnection::Socket::SocketAddress()) override {
            if (sslErr != 0) {
                onError(-sslErr);
            } else {
                socket::stream::SocketConnector<SocketConnection>::connect(remoteAddress, onError, bindAddress);
            }
        }

        SSL_CTX* ctx = nullptr;
        unsigned long sslErr = 0;

        template <typename SocketConnectorT>
        friend class net::socket::stream::SocketClient;
    };

} // namespace net::socket::stream::tls

#endif // NET_SOCKET_SOCK_STREAM_TLS_SOCKETCONNECTOR_H
