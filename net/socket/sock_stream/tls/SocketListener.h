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

#ifndef NET_SOCKET_SOCK_STREAM_TLS_SOCKETLISTENER_H
#define NET_SOCKET_SOCK_STREAM_TLS_SOCKETLISTENER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "Descriptor.h"
#include "Logger.h"
#include "SocketConnection.h"
#include "socket/sock_stream/SocketListener.h"
#include "ssl_utils.h"

#define TLSHANDSHAKE_TIMEOUT 10

namespace net::socket::stream {

    template <typename SocketConnector>
    class SocketServer;

}

namespace net::socket::stream::tls {

    template <typename SocketT>
    class SocketListener : public net::socket::stream::SocketListener<net::socket::stream::tls::SocketConnection<SocketT>> {
    public:
        using SocketConnection = net::socket::stream::tls::SocketConnection<SocketT>;

    protected:
        SocketListener(const std::function<void(SocketConnection* socketConnection)>& onConstruct,
                       const std::function<void(SocketConnection* socketConnection)>& onDestruct,
                       const std::function<void(SocketConnection* socketConnection)>& onConnect,
                       const std::function<void(SocketConnection* socketConnection)>& onDisconnect,
                       const std::function<void(SocketConnection* socketConnection, const char* junk, ssize_t junkLen)>& onRead,
                       const std::function<void(SocketConnection* socketConnection, int errnum)>& onReadError,
                       const std::function<void(SocketConnection* socketConnection, int errnum)>& onWriteError,
                       const std::map<std::string, std::any>& options)
            : net::socket::stream::SocketListener<SocketConnection>(
                  [onConstruct, &ctx = this->ctx, onDestruct, onRead, onReadError, onWriteError, onDisconnect](
                      SocketConnection* socketConnection) -> void {
                      onConstruct(socketConnection);
                  },
                  [onDestruct](SocketConnection* socketConnection) -> void {
                      onDestruct(socketConnection);
                  },
                  [onConnect, &ctx = this->ctx](SocketConnection* socketConnection) -> void {
                      class TLSHandshaker
                          : public ReadEventReceiver
                          , public WriteEventReceiver
                          , public Descriptor {
                      public:
                          TLSHandshaker(SSL* ssl,
                                        int sslErr,
                                        const std::function<void(void)>& onSuccess,
                                        const std::function<void(void)>& onTimeout,
                                        const std::function<void(int err)>& onError)
                              : ssl(ssl)
                              , onSuccess(onSuccess)
                              , onTimeout(onTimeout)
                              , onError(onError) {
                              this->ReadEventReceiver::setTimeout(TLSHANDSHAKE_TIMEOUT);
                              this->WriteEventReceiver::setTimeout(TLSHANDSHAKE_TIMEOUT);

                              open(SSL_get_fd(ssl), FLAGS::dontClose);

                              switch (sslErr) {
                                  case SSL_ERROR_WANT_READ:
                                      ReadEventReceiver::enable();
                                      break;
                                  case SSL_ERROR_WANT_WRITE:
                                      WriteEventReceiver::enable();
                                      break;
                                  case SSL_ERROR_NONE:
                                      onSuccess();
                                      delete this;
                                      break;
                                  default:
                                      onError(ERR_peek_error());
                                      delete this;
                              }
                          }

                          void readEvent() override {
                              int ret = SSL_do_handshake(ssl);
                              int sslErr = SSL_get_error(ssl, ret);

                              switch (sslErr) {
                                  case SSL_ERROR_WANT_WRITE:
                                      ReadEventReceiver::disable();
                                      WriteEventReceiver::enable();
                                      break;
                                  case SSL_ERROR_WANT_READ:
                                      break;
                                  case SSL_ERROR_NONE:
                                      ReadEventReceiver::disable();
                                      onSuccess();
                                      break;
                                  default:
                                      ReadEventReceiver::disable();
                                      onError(ERR_peek_error());
                                      break;
                              }
                          }

                          void writeEvent() override {
                              int ret = SSL_do_handshake(ssl);
                              int sslErr = SSL_get_error(ssl, ret);

                              switch (sslErr) {
                                  case SSL_ERROR_WANT_READ:
                                      WriteEventReceiver::disable();
                                      ReadEventReceiver::enable();
                                      break;
                                  case SSL_ERROR_WANT_WRITE:
                                      break;
                                  case SSL_ERROR_NONE:
                                      ReadEventReceiver::disable();
                                      onSuccess();
                                      break;
                                  default:
                                      WriteEventReceiver::disable();
                                      onError(ERR_peek_error());
                                      break;
                              }
                          }

                          void timeoutEvent() override {
                              onTimeout();
                          }

                          void unobserved() override {
                              delete this;
                          }

                          static void doHandshake(SSL* ssl,
                                                  int sslErr,
                                                  const std::function<void(void)>& onSuccess,
                                                  const std::function<void(void)>& onTimeout,
                                                  const std::function<void(int err)>& onError) {
                              new TLSHandshaker(ssl, sslErr, onSuccess, onTimeout, onError);
                          }

                      private:
                          SSL* ssl = nullptr;
                          std::function<void(void)> onSuccess;
                          std::function<void(void)> onTimeout;
                          std::function<void(int err)> onError;
                      };

                      SSL* ssl = socketConnection->startSSL(ctx);

                      if (ssl != nullptr) {
                          int sslErr = SSL_get_error(ssl, SSL_accept(ssl));

                          TLSHandshaker::doHandshake(
                              ssl,
                              sslErr,
                              [&onConnect, socketConnection](void) -> void {
                                  onConnect(socketConnection);
                              },
                              [socketConnection](void) -> void {
                                  socketConnection->ReadEventReceiver::disable();
                                  PLOG(ERROR) << "TLS handshake timeout";
                              },
                              [socketConnection](int sslErr) -> void {
                                  socketConnection->ReadEventReceiver::disable();
                                  PLOG(ERROR) << "TLS handshake failed: " << ERR_error_string(sslErr, nullptr);
                              });
                      } else {
                          socketConnection->ReadEventReceiver::disable();
                          PLOG(ERROR) << "TLS handshake failed: " << ERR_error_string(ERR_get_error(), nullptr);
                      }
                  },
                  [onDisconnect](SocketConnection* socketConnection) -> void { // onDisconnect
                      socketConnection->stopSSL();
                      onDisconnect(socketConnection);
                  },
                  onRead,
                  onReadError,
                  onWriteError,
                  options) {
            ctx = SSL_CTX_new(TLS_server_method());
            sslErr = ssl_init_ctx(ctx, options, true);
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

        template <typename SocketListener>
        friend class net::socket::stream::SocketServer;
    };

} // namespace net::socket::stream::tls

#endif // NET_SOCKET_SOCK_STREAM_TLS_SOCKETLISTENER_H
