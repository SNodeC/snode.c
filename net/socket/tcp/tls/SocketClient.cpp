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

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <openssl/err.h>
#include <openssl/ssl.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "Logger.h"
#include "socket/tcp/tls/SocketClient.h"
#include "socket/tcp/tls/ssl_utils.h"
#include "timer/SingleshotTimer.h"

#define TLSCONNECT_TIMEOUT 10

namespace net::socket::tcp::tls {

    SocketClient::SocketClient(
        const std::function<void(SocketClient::SocketConnection* socketConnection)>& onConstruct,
        const std::function<void(SocketClient::SocketConnection* socketConnection)>& onDestruct,
        const std::function<void(SocketClient::SocketConnection* socketConnection)>& onConnect,
        const std::function<void(SocketClient::SocketConnection* socketConnection)>& onDisconnect,
        const std::function<void(SocketClient::SocketConnection* socketConnection, const char* junk, ssize_t junkLen)>& onRead,
        const std::function<void(SocketClient::SocketConnection* socketConnection, int errnum)>& onReadError,
        const std::function<void(SocketClient::SocketConnection* socketConnection, int errnum)>& onWriteError,
        const std::map<std::string, std::any>& options)
        : socket::tcp::SocketClient<SocketClient::SocketConnection>(
              [&ctx = this->ctx, onConstruct](SocketClient::SocketConnection* socketConnection) -> void { // onConstruct
                  socketConnection->setCTX(ctx);
                  onConstruct(socketConnection);
              },
              onDestruct,                                                                   // onDestruct
              [this, onConnect](SocketClient::SocketConnection* socketConnection) -> void { // onConnect
                  socketConnection->startSSL();

                  class Connector
                      : public ReadEventReceiver
                      , public WriteEventReceiver
                      , public Descriptor {
                  public:
                      Connector(tls::SocketClient* socketClient,
                                SocketClient::SocketConnection* socketConnection,
                                const std::function<void(tls::SocketConnection* socketConnection)>& onConnect)
                          : socketClient(socketClient)
                          , socketConnection(socketConnection)
                          , onConnect(onConnect)
                          , timeOut(timer::Timer::singleshotTimer(
                                [this](const void*) -> void {
                                    ReadEventReceiver::disable();
                                    WriteEventReceiver::disable();
                                    this->socketClient->onError(ETIMEDOUT);
                                    this->socketConnection->ReadEventReceiver::disable();
                                },
                                (struct timeval){TLSCONNECT_TIMEOUT, 0},
                                nullptr)) {
                          open(socketConnection->getFd(), FLAGS::dontClose);
                          ssl = socketConnection->getSSL();
                          if (ssl != nullptr) {
                              int err = SSL_connect(ssl);
                              int sslErr = SSL_get_error(ssl, err);

                              if (sslErr == SSL_ERROR_WANT_READ) {
                                  ReadEventReceiver::enable();
                              } else if (sslErr == SSL_ERROR_WANT_WRITE) {
                                  WriteEventReceiver::enable();
                              } else {
                                  if (sslErr == SSL_ERROR_NONE) {
                                      socketClient->onError(0);
                                      onConnect(socketConnection);
                                  } else {
                                      socketClient->onError(-ERR_peek_error());
                                  }
                                  timeOut.cancel();
                                  delete this;
                              }
                          } else {
                              socketClient->onError(-ERR_peek_error());
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
                                      socketClient->onError(0);
                                      onConnect(socketConnection);
                                  } else {
                                      socketClient->onError(-ERR_peek_error());
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
                                      socketClient->onError(0);
                                      onConnect(socketConnection);
                                  } else {
                                      socketClient->onError(-ERR_peek_error());
                                      socketConnection->ReadEventReceiver::disable();
                                  }
                              }
                          }
                      }

                      void unobserved() override {
                          delete this;
                      }

                  private:
                      tls::SocketClient* socketClient;
                      tls::SocketConnection* socketConnection = nullptr;
                      SSL* ssl = nullptr;
                      std::function<void(SocketClient::SocketConnection* socketConnection)> onConnect;
                      timer::Timer& timeOut;
                  };

                  new Connector(this, socketConnection, onConnect);
              },
              onDisconnect, // onDisconnect
              onRead,       // onRead
              onReadError,  // onReadError
              onWriteError, // onWriteError
              options) {
        ctx = SSL_CTX_new(TLS_client_method());
        sslErr = net::socket::tcp::tls::ssl_init_ctx(ctx, options, false);
    }

    SocketClient::~SocketClient() {
        if (ctx != nullptr) {
            SSL_CTX_free(ctx);
        }
    }

    void SocketClient::connect(const std::map<std::string, std::any>& options,
                               const std::function<void(int err)>& onError,
                               const socket::InetAddress& localAddress) {
        this->onError = onError;

        if (sslErr != 0) {
            onError(-sslErr);
        } else {
            socket::tcp::SocketClient<SocketClient::SocketConnection>::connect(
                options,
                [onError](int err) -> void {
                    if (err) {
                        onError(err);
                    }
                },
                localAddress);
        }
    }

}; // namespace net::socket::tcp::tls
