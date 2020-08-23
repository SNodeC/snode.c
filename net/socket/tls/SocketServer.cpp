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
#include "timer/SingleshotTimer.h"

#define TLSACCEPT_TIMEOUT 10
#define SSL_VERIFY_FLAGS SSL_VERIFY_PEER | SSL_VERIFY_CLIENT_ONCE

namespace net::socket::tls {

    SocketServer::SocketServer(const std::function<void(SocketConnection* socketConnection)>& onConnect,
                               const std::function<void(SocketConnection* socketConnection)>& onDisconnect,
                               const std::function<void(SocketConnection* socketConnection, const char* junk, ssize_t n)>& onRead,
                               const std::function<void(SocketConnection* socketConnection, int errnum)>& onReadError,
                               const std::function<void(SocketConnection* socketConnection, int errnum)>& onWriteError,
                               const std::string& certChain, const std::string& keyPEM, const std::string& password,
                               const std::string& caFile, const std::string& caDir, bool useDefaultCADir)
        : net::socket::SocketServer<SocketConnection>(
              [this, onConnect](SocketConnection* socketConnection) -> void {
                  class TLSAcceptor
                      : public ReadEventReceiver
                      , public WriteEventReceiver
                      , public Socket {
                  public:
                      TLSAcceptor(SocketConnection* socketConnection, SSL_CTX* ctx,
                                  const std::function<void(SocketConnection* socketConnection)>& onConnect)
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
                                      socketConnection->ReadEventReceiver::disable();
                                  }
                              }
                          }
                      }

                      void unobserved() override {
                          delete this;
                      }

                  private:
                      tls::SocketConnection* socketConnection = nullptr;
                      SSL* ssl = nullptr;
                      std::function<void(SocketConnection* socketConnection)> onConnect;
                      net::timer::Timer& timeOut;
                  };

                  new TLSAcceptor(socketConnection, ctx, onConnect);
              },
              [onDisconnect](SocketConnection* socketConnection) -> void {
                  onDisconnect(socketConnection);
                  socketConnection->stopSSL();
              },
              onRead, onReadError, onWriteError)
        , ctx(nullptr) {
        ctx = SSL_CTX_new(TLS_server_method());
        if (ctx != nullptr) {
            SSL_CTX_set_default_passwd_cb(ctx, SocketServer::passwordCallback);
            SSL_CTX_set_default_passwd_cb_userdata(ctx, ::strdup(password.c_str()));
            if (SSL_CTX_use_certificate_chain_file(ctx, certChain.c_str()) <= 0) {
                sslErr = ERR_peek_error();
            } else if (SSL_CTX_use_PrivateKey_file(ctx, keyPEM.c_str(), SSL_FILETYPE_PEM) <= 0) {
                sslErr = ERR_peek_error();
            } else if (!SSL_CTX_check_private_key(ctx)) {
                sslErr = ERR_peek_error();
            } else {
                if (!caFile.empty() || !caDir.empty()) {
                    if (!SSL_CTX_load_verify_locations(ctx, !caFile.empty() ? caFile.c_str() : nullptr,
                                                       !caDir.empty() ? caDir.c_str() : nullptr)) {
                        sslErr = ERR_peek_error();
                    }
                }
                if (sslErr == 0 && useDefaultCADir) {
                    if (!SSL_CTX_set_default_verify_paths(ctx)) {
                        sslErr = ERR_peek_error();
                    }
                }
                if (sslErr == 0 && (useDefaultCADir || !caFile.empty() || !caDir.empty())) {
                    SSL_CTX_set_verify(ctx, SSL_VERIFY_FLAGS, NULL);
                }
            }
        } else {
            sslErr = ERR_peek_error();
        }
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
            net::socket::SocketServer<SocketConnection>::listen(port, backlog, [&onError](int err) -> void {
                onError(err);
            });
        }
    }

    int SocketServer::passwordCallback(char* buf, int size, [[maybe_unused]] int rwflag, void* u) {
        strncpy(buf, static_cast<char*>(u), size);
        buf[size - 1] = '\0';

        memset(u, 0, ::strlen(static_cast<char*>(u))); // garble password
        free(u);

        return ::strlen(buf);
    }
}; // namespace net::socket::tls
