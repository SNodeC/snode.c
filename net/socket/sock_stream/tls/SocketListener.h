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

#ifndef TLS_SOCKETLISTENER_H
#define TLS_SOCKETLISTENER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "Descriptor.h"
#include "SocketConnection.h"
#include "socket/sock_stream/SocketListener.h"
#include "ssl_utils.h"
#include "timer/SingleshotTimer.h"

#define TLSACCEPT_TIMEOUT 10

namespace net::socket::stream::tls {

    template <typename SocketT>
    class SocketListener : public net::socket::stream::SocketListener<net::socket::stream::tls::SocketConnection<SocketT>> {
    public:
        using Socket = SocketT;
        using SocketConnection =
            typename net::socket::stream::SocketListener<net::socket::stream::tls::SocketConnection<Socket>>::SocketConnection;

        SocketListener(const std::function<void(SocketConnection* socketConnection)>& onConstruct,
                       const std::function<void(SocketConnection* socketConnection)>& onDestruct,
                       const std::function<void(SocketConnection* socketConnection)>& onConnect,
                       const std::function<void(SocketConnection* socketConnection)>& onDisconnect,
                       const std::function<void(SocketConnection* socketConnection, const char* junk, ssize_t junkLen)>& onRead,
                       const std::function<void(SocketConnection* socketConnection, int errnum)>& onReadError,
                       const std::function<void(SocketConnection* socketConnection, int errnum)>& onWriteError,
                       const std::map<std::string, std::any>& options)
            : net::socket::stream::SocketListener<SocketConnection>(
                  [onConstruct, &ctx = this->ctx](SocketConnection* socketConnection) -> void {
                      socketConnection->setSSL_CTX(ctx);
                      onConstruct(socketConnection);
                  },
                  [onDestruct](SocketConnection* socketConnection) -> void {
                      socketConnection->clearSSL_CTX();
                      onDestruct(socketConnection);
                  },
                  [onConnect](SocketConnection* socketConnection) -> void {
                      SSL* ssl = socketConnection->startSSL();

                      class Acceptor
                          : public ReadEventReceiver
                          , public WriteEventReceiver
                          , public Descriptor {
                      public:
                          Acceptor(SocketConnection* socketConnection,
                                   SSL* ssl,
                                   const std::function<void(SocketConnection* socketConnection)>& onConnect)
                              : socketConnection(socketConnection)
                              , ssl(ssl)
                              , onConnect(onConnect)
                              , timeOut(net::timer::Timer::singleshotTimer(
                                    [this, socketConnection](const void*) -> void {
                                        ReadEventReceiver::disable();
                                        WriteEventReceiver::disable();
                                        socketConnection->ReadEventReceiver::disable();
                                    },
                                    (struct timeval){TLSACCEPT_TIMEOUT, 0},
                                    nullptr)) {
                              open(socketConnection->getFd(), FLAGS::dontClose);

                              if (ssl != nullptr) {
                                  int err = SSL_accept(ssl);
                                  int sslErr = SSL_get_error(ssl, err);

                                  if (sslErr == SSL_ERROR_WANT_READ) {
                                      ReadEventReceiver::enable();
                                  } else if (sslErr == SSL_ERROR_WANT_WRITE) {
                                      WriteEventReceiver::enable();
                                  } else {
                                      if (sslErr == SSL_ERROR_NONE) {
                                          onConnect(socketConnection);
                                      } else {
                                          socketConnection->ReadEventReceiver::disable();
                                      }
                                      timeOut.cancel();
                                      unobserved();
                                  }
                              } else {
                                  onConnect(socketConnection);
                                  socketConnection->ReadEventReceiver::disable();
                                  timeOut.cancel();
                                  unobserved();
                              }
                          }

                          void readEvent() override {
                              int err = SSL_accept(ssl);
                              int sslErr = SSL_get_error(ssl, err);

                              if (sslErr != SSL_ERROR_WANT_READ) {
                                  if (sslErr == SSL_ERROR_WANT_WRITE) {
                                      ReadEventReceiver::disable();
                                      WriteEventReceiver::enable();
                                  } else {
                                      timeOut.cancel();
                                      ReadEventReceiver::disable();
                                      if (sslErr == SSL_ERROR_NONE) {
                                          onConnect(socketConnection);
                                      } else {
                                          socketConnection->ReadEventReceiver::disable();
                                      }
                                  }
                              }
                          }

                          void writeEvent() override {
                              int err = SSL_accept(ssl);
                              int sslErr = SSL_get_error(ssl, err);

                              if (sslErr != SSL_ERROR_WANT_WRITE) {
                                  if (sslErr == SSL_ERROR_WANT_READ) {
                                      WriteEventReceiver::disable();
                                      ReadEventReceiver::enable();
                                  } else {
                                      timeOut.cancel();
                                      WriteEventReceiver::disable();
                                      if (sslErr == SSL_ERROR_NONE) {
                                          onConnect(socketConnection);
                                      } else {
                                          socketConnection->WriteEventReceiver::disable();
                                      }
                                  }
                              }
                          }

                          void unobserved() override {
                              delete this;
                          }

                      private:
                          SocketConnection* socketConnection = nullptr;
                          SSL* ssl = nullptr;
                          std::function<void(SocketConnection* socketConnection)> onConnect;
                          net::timer::Timer& timeOut;
                      };

                      new Acceptor(socketConnection, ssl, onConnect);
                  },
                  [onDisconnect](SocketConnection* socketConnection) -> void { // onDisconnect
                      socketConnection->stopSSL();
                      onDisconnect(socketConnection);
                  },
                  onRead,
                  onReadError,
                  onWriteError,
                  options)
            , ctx(nullptr) {
            ctx = SSL_CTX_new(TLS_server_method());
            sslErr = net::socket::stream::tls::ssl_init_ctx(ctx, options, true);
        }

        ~SocketListener() override {
            if (ctx != nullptr) {
                SSL_CTX_free(ctx);
            }
        }

        void listen(const typename SocketConnection::Socket::SocketAddress& localAddress,
                    int backlog,
                    const std::function<void(int err)>& onError) override {
            if (sslErr != 0) {
                onError(-sslErr);
            } else {
                socket::stream::SocketListener<SocketConnection>::listen(localAddress, backlog, onError);
            }
        }

    private:
        SSL_CTX* ctx = nullptr;
        unsigned long sslErr = 0;
    };

} // namespace net::socket::stream::tls

#endif // TLS_SOCKETLISTENER_H
