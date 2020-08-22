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
#define SSL_VERIFY_FLAGS SSL_VERIFY_PEER | SSL_VERIFY_CLIENT_ONCE // | SSL_VERIFY_FAIL_IF_NO_PEER_CERT

namespace net::socket::tls {

    SocketServer::SocketServer(const std::function<void(SocketConnection* socketConnection)>& onConnect,
                               const std::function<void(SocketConnection* socketConnection)>& onDisconnect,
                               const std::function<void(SocketConnection* socketConnection, const char* junk, ssize_t n)>& onRead,
                               const std::function<void(SocketConnection* socketConnection, int errnum)>& onReadError,
                               const std::function<void(SocketConnection* socketConnection, int errnum)>& onWriteError,
                               const std::string& certChain, const std::string& keyPEM, const std::string& password,
                               const std::string& caFile)
        : net::socket::SocketServer<SocketConnection>(
              [this, onConnect](SocketConnection* socketConnection) -> void {
                  class TLSAcceptor
                      : public ReadEventReceiver
                      , public WriteEventReceiver
                      , public Socket {
                  public:
                      TLSAcceptor(SocketConnection* socketConnection, SSL_CTX* ctx,
                                  const std::function<void(SocketConnection* socketConnection)>& onConnect)
                          : Descriptor(true)
                          , socketConnection(socketConnection)
                          , ssl(socketConnection->startSSL(ctx))
                          , onConnect(onConnect)
                          , timeOut(net::timer::Timer::singleshotTimer(
                                [this]([[maybe_unused]] const void* arg) -> void {
                                    this->ReadEventReceiver::disable();
                                    this->WriteEventReceiver::disable();
                                    this->socketConnection->ReadEventReceiver::disable();
                                },
                                (struct timeval){TLSACCEPT_TIMEOUT, 0}, nullptr)) {
                          if (ssl) {
                              this->attachFd(socketConnection->getFd());

                              int err = SSL_accept(ssl);
                              int sslErr = SSL_get_error(ssl, err);

                              if (sslErr == SSL_ERROR_WANT_READ) {
                                  this->ReadEventReceiver::enable();
                              } else if (sslErr == SSL_ERROR_WANT_WRITE) {
                                  this->WriteEventReceiver::enable();
                              } else {
                                  if (sslErr == SSL_ERROR_NONE) {
                                      onConnect(socketConnection);
                                  }
                                  timeOut.cancel();
                                  unobserved();
                              }
                          } else {
                              unobserved();
                          }
                      }

                      void readEvent() override {
                          int err = SSL_accept(ssl);
                          int sslErr = SSL_get_error(ssl, err);

                          if (sslErr != SSL_ERROR_WANT_READ) {
                              if (sslErr == SSL_ERROR_WANT_WRITE) {
                                  this->ReadEventReceiver::disable();
                                  this->WriteEventReceiver::enable();
                              } else {
                                  timeOut.cancel();
                                  this->ReadEventReceiver::disable();
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
                                  this->WriteEventReceiver::disable();
                                  this->ReadEventReceiver::enable();
                              } else {
                                  timeOut.cancel();
                                  this->WriteEventReceiver::disable();
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
                  socketConnection->stopSSL();
                  onDisconnect(socketConnection);
              },
              onRead, onReadError, onWriteError)
        , ctx(nullptr) {
        ctx = SSL_CTX_new(TLS_server_method());
        if (!ctx) {
            sslErr = ERR_peek_error();
        } else {
            SSL_CTX_set_default_passwd_cb(ctx, SocketServer::passwordCallback);
            SSL_CTX_set_default_passwd_cb_userdata(ctx, ::strdup(password.c_str()));
            if (SSL_CTX_use_certificate_chain_file(ctx, certChain.c_str()) <= 0) {
                sslErr = ERR_peek_error();
            } else if (SSL_CTX_use_PrivateKey_file(ctx, keyPEM.c_str(), SSL_FILETYPE_PEM) <= 0) {
                sslErr = ERR_peek_error();
            } else if (!SSL_CTX_check_private_key(ctx)) {
                sslErr = ERR_peek_error();
            } else if (!caFile.empty()) {
                if (!SSL_CTX_load_verify_locations(ctx, caFile.c_str(), nullptr)) {
                    sslErr = ERR_peek_error();
                } else {
                    SSL_CTX_set_verify(ctx, SSL_VERIFY_FLAGS, NULL);
                }
            }
        }
    }

    SocketServer::~SocketServer() {
        if (ctx) {
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
