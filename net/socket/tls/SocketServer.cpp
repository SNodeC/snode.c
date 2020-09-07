/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020  Volker Christian <me@vchrist.at>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <openssl/err.h>
#include <openssl/ssl.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "socket/tls/SocketServer.h"
#include "socket/tls/ssl_utils.h"
#include "timer/SingleshotTimer.h"

#define TLSACCEPT_TIMEOUT 10

namespace net::socket::tls {

    SocketServer::SocketServer(
        const std::function<void(SocketServer::SocketConnection* socketConnection)>& onConnect,
        const std::function<void(SocketServer::SocketConnection* socketConnection)>& onDisconnect,
        const std::function<void(SocketServer::SocketConnection* socketConnection, const char* junk, ssize_t junkLen)>& onRead,
        const std::function<void(SocketServer::SocketConnection* socketConnection, int errnum)>& onReadError,
        const std::function<void(SocketServer::SocketConnection* socketConnection, int errnum)>& onWriteError,
        const std::map<std::string, std::any>& options)
        : socket::SocketServer<SocketServer::SocketConnection>(
              [this, onConnect](SocketServer::SocketConnection* socketConnection) -> void {
                  class TLSAcceptor
                      : public ReadEventReceiver
                      , public WriteEventReceiver
                      , public Socket {
                  public:
                      TLSAcceptor(SocketServer::SocketConnection* socketConnection, SSL_CTX* ctx,
                                  const std::function<void(SocketServer::SocketConnection* socketConnection)>& onConnect)
                          : socketConnection(socketConnection)
                          , onConnect(onConnect)
                          , timeOut(net::timer::Timer::singleshotTimer(
                                [this]([[maybe_unused]] const void* arg) -> void {
                                    ReadEventReceiver::disable();
                                    WriteEventReceiver::disable();
                                    this->socketConnection->ReadEventReceiver::disable();
                                },
                                (struct timeval){TLSACCEPT_TIMEOUT, 0}, nullptr)) {
                          open(socketConnection->getFd(), FLAGS::dontClose);
                          ssl = socketConnection->startSSL(ctx);
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
                      SocketServer::SocketConnection* socketConnection = nullptr;
                      SSL* ssl = nullptr;
                      std::function<void(SocketServer::SocketConnection* socketConnection)> onConnect;
                      net::timer::Timer& timeOut;
                  };

                  new TLSAcceptor(socketConnection, ctx, onConnect);
              },
              [onDisconnect](SocketServer::SocketConnection* socketConnection) -> void {
                  onDisconnect(socketConnection);
                  socketConnection->stopSSL();
              },
              onRead, onReadError, onWriteError, options)
        , ctx(nullptr) {
        ctx = SSL_CTX_new(TLS_server_method());
        sslErr = net::socket::tls::ssl_init_ctx(ctx, options, true);
    }

    SocketServer::~SocketServer() {
        if (ctx != nullptr) {
            SSL_CTX_free(ctx);
            ctx = nullptr;
        }
    }

    void SocketServer::listen(in_port_t port, int backlog, const std::function<void(int err)>& onError) {
        if (sslErr != 0) {
            onError(-sslErr);
        } else {
            socket::SocketServer<SocketServer::SocketConnection>::listen(port, backlog, [&onError](int err) -> void {
                onError(err);
            });
        }
    }

    void SocketServer::listen(const std::string& host, in_port_t port, int backlog, const std::function<void(int err)>& onError) {
        if (sslErr != 0) {
            onError(-sslErr);
        } else {
            socket::SocketServer<SocketServer::SocketConnection>::listen(host, port, backlog, [&onError](int err) -> void {
                onError(err);
            });
        }
    }

}; // namespace net::socket::tls
