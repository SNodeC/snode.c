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

#ifndef TLS_SOCKETCLIENT_H
#define TLS_SOCKETCLIENT_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <any>
#include <map>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "socket/tcp/SocketClient.h"
#include "socket/tcp/tls/SocketConnection.h"
#include "socket/tcp/tls/ssl_utils.h"
#include "timer/SingleshotTimer.h"

#define TLSCONNECT_TIMEOUT 10

namespace net::socket::tcp::tls {

    template <typename SocketT>
    class SocketClient : public socket::tcp::SocketClient<tls::SocketConnection<SocketT>> {
    public:
        using Socket = SocketT;
        using SocketConnection = net::socket::tcp::tls::SocketConnection<Socket>;

        using socket::tcp::SocketClient<SocketConnection>::SocketClient;

        SocketClient(const std::function<void(SocketConnection* socketConnection)>& onConstruct,
                     const std::function<void(SocketConnection* socketConnection)>& onDestruct,
                     const std::function<void(SocketConnection* socketConnection)>& onConnect,
                     const std::function<void(SocketConnection* socketConnection)>& onDisconnect,
                     const std::function<void(SocketConnection* socketConnection, const char* junk, ssize_t junkLen)>& onRead,
                     const std::function<void(SocketConnection* socketConnection, int errnum)>& onReadError,
                     const std::function<void(SocketConnection* socketConnection, int errnum)>& onWriteError,
                     const std::map<std::string, std::any>& options = {{}})
            : socket::tcp::SocketClient<SocketConnection>(
                  [onConstruct, &ctx = this->ctx](SocketConnection* socketConnection) -> void { // onConstruct
                      socketConnection->setSSL_CTX(ctx);
                      onConstruct(socketConnection);
                  },
                  [onDestruct](SocketConnection* socketConnection) -> void { // onDestruct
                      socketConnection->clearSSL_CTX();
                      onDestruct(socketConnection);
                  },
                  [onConnect, &onError = this->onError](SocketConnection* socketConnection) -> void { // onConnect
                      SSL* ssl = socketConnection->startSSL();

                      class Connector
                          : public ReadEventReceiver
                          , public WriteEventReceiver
                          , public Descriptor {
                      public:
                          Connector(SocketConnection* socketConnection,
                                    SSL* ssl,
                                    const std::function<void(SocketConnection* socketConnection)>& onConnect,
                                    const std::function<void(int err)>& onError)
                              : socketConnection(socketConnection)
                              , ssl(ssl)
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
                                  int err = SSL_connect(ssl);
                                  int sslErr = SSL_get_error(ssl, err);

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
                              int err = SSL_connect(ssl);
                              int sslErr = SSL_get_error(ssl, err);

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
                              int err = SSL_connect(ssl);
                              int sslErr = SSL_get_error(ssl, err);

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

                      private:
                          SocketConnection* socketConnection = nullptr;
                          SSL* ssl = nullptr;
                          std::function<void(SocketConnection* socketConnection)> onConnect;
                          std::function<void(int err)> onError;
                          timer::Timer& timeOut;
                      };

                      new Connector(socketConnection, ssl, onConnect, onError);
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
            sslErr = net::socket::tcp::tls::ssl_init_ctx(ctx, options, false);
        }

        ~SocketClient() override {
            if (ctx != nullptr) {
                SSL_CTX_free(ctx);
            }
        }

        // NOLINTNEXTLINE(google-default-arguments)
        void connect(const std::map<std::string, std::any>& options,
                     const std::function<void(int err)>& onError,
                     const typename SocketConnection::Socket::SocketAddress& localAddress =
                         typename SocketConnection::Socket::SocketAddress()) override {
            this->onError = onError;

            if (sslErr != 0) {
                onError(-sslErr);
            } else {
                socket::tcp::SocketClient<SocketConnection>::connect(
                    options,
                    [onError](int err) -> void {
                        if (err) {
                            onError(err);
                        }
                    },
                    localAddress);
            }
        }

    private:
        SSL_CTX* ctx = nullptr;
        unsigned long sslErr = 0;

        std::function<void(int err)> onError;
    };

} // namespace net::socket::tcp::tls

#endif // TLS_SOCKETCLIENT_H
