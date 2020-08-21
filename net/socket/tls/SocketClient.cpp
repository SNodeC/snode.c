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

#include <cstring>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "socket/tls/SocketClient.h"
#include "timer/SingleshotTimer.h"

#define TLSCONNECT_TIMEOUT 10

namespace net::socket::tls {

    SocketClient::SocketClient(const std::function<void(SocketConnection* cs)>& onConnect,
                               const std::function<void(SocketConnection* cs)>& onDisconnect,
                               const std::function<void(SocketConnection* cs, const char* junk, ssize_t n)>& onRead,
                               const std::function<void(SocketConnection* cs, int errnum)>& onReadError,
                               const std::function<void(SocketConnection* cs, int errnum)>& onWriteError)
        : net::socket::SocketClient<SocketConnection>(
              [this, onConnect](SocketConnection* cs) -> void {
                  class TLSConnector
                      : public net::ReadEventReceiver
                      , public net::WriteEventReceiver
                      , public Socket {
                  public:
                      TLSConnector(SocketClient* sc, SocketConnection* cs, SSL_CTX* ctx,
                                   const std::function<void(SocketConnection* cs)>& onConnect)
                          : Descriptor(true)
                          , sc(sc)
                          , cs(cs)
                          , ssl(cs->startSSL(ctx))
                          , onConnect(onConnect)
                          , timeOut(net::timer::Timer::singleshotTimer(
                                [this]([[maybe_unused]] const void* arg) -> void {
                                    this->net::ReadEventReceiver::disable();
                                    this->net::WriteEventReceiver::disable();
                                    this->sc->onError(ETIMEDOUT);
                                    this->cs->net::ReadEventReceiver::disable();
                                },
                                (struct timeval){TLSCONNECT_TIMEOUT, 0}, nullptr)) {
                          this->attachFd(cs->getFd());

                          int err = SSL_connect(ssl);
                          int sslErr = SSL_get_error(ssl, err);

                          if (sslErr == SSL_ERROR_WANT_READ) {
                              this->net::ReadEventReceiver::enable();
                          } else if (sslErr == SSL_ERROR_WANT_WRITE) {
                              this->net::WriteEventReceiver::enable();
                          } else {
                              if (sslErr == SSL_ERROR_NONE) {
                                  sc->onError(0);
                                  onConnect(cs);
                              } else {
                                  sc->onError(-sslErr);
                              }
                              timeOut.cancel();
                              delete this;
                          }
                      }

                      void readEvent() override {
                          int err = SSL_connect(ssl);
                          int sslErr = SSL_get_error(ssl, err);

                          if (sslErr != SSL_ERROR_WANT_READ) {
                              if (sslErr == SSL_ERROR_WANT_WRITE) {
                                  this->net::ReadEventReceiver::disable();
                                  this->net::WriteEventReceiver::enable();
                              } else {
                                  timeOut.cancel();
                                  this->net::ReadEventReceiver::disable();
                                  if (sslErr == SSL_ERROR_NONE) {
                                      sc->onError(0);
                                      this->onConnect(cs);
                                  } else {
                                      sc->onError(-sslErr);
                                      cs->net::ReadEventReceiver::disable();
                                  }
                              }
                          }
                      }

                      void writeEvent() override {
                          int err = SSL_connect(ssl);
                          int sslErr = SSL_get_error(ssl, err);

                          if (sslErr != SSL_ERROR_WANT_WRITE) {
                              if (sslErr == SSL_ERROR_WANT_READ) {
                                  this->net::WriteEventReceiver::disable();
                                  this->net::ReadEventReceiver::enable();
                              } else {
                                  timeOut.cancel();
                                  this->net::WriteEventReceiver::disable();
                                  if (sslErr == SSL_ERROR_NONE) {
                                      sc->onError(0);
                                      this->onConnect(cs);
                                  } else {
                                      sc->onError(-sslErr);
                                      cs->net::ReadEventReceiver::disable();
                                  }
                              }
                          }
                      }

                      void unobserved() override {
                          delete this;
                      }

                  private:
                      SocketClient* sc = nullptr;
                      SocketConnection* cs = nullptr;
                      SSL* ssl = nullptr;
                      std::function<void(SocketConnection* cs)> onConnect;
                      net::timer::Timer& timeOut;
                  };

                  new TLSConnector(this, cs, ctx, onConnect);
              },
              [onDisconnect](SocketConnection* cs) -> void {
                  cs->stopSSL();
                  onDisconnect(cs);
              },
              onRead, onReadError, onWriteError) {
        ctx = SSL_CTX_new(TLS_client_method());
        if (!ctx) {
            sslErr = ERR_peek_error();
        }
    }

    SocketClient::SocketClient(const std::function<void(SocketConnection* cs)>& onConnect,
                               const std::function<void(SocketConnection* cs)>& onDisconnect,
                               const std::function<void(SocketConnection* cs, const char* junk, ssize_t n)>& onRead,
                               const std::function<void(SocketConnection* cs, int errnum)>& onReadError,
                               const std::function<void(SocketConnection* cs, int errnum)>& onWriteError, const std::string& certChain,
                               const std::string& keyPEM, const std::string& password)
        : SocketClient(onConnect, onDisconnect, onRead, onReadError, onWriteError) {
        if (sslErr == 0) {
            SSL_CTX_set_default_passwd_cb(ctx, SocketClient::passwordCallback);
            SSL_CTX_set_default_passwd_cb_userdata(ctx, ::strdup(password.c_str()));
            if (SSL_CTX_use_certificate_chain_file(ctx, certChain.c_str()) <= 0) {
                sslErr = ERR_peek_error();
            } else if (SSL_CTX_use_PrivateKey_file(ctx, keyPEM.c_str(), SSL_FILETYPE_PEM) <= 0) {
                sslErr = ERR_peek_error();
            } else if (!SSL_CTX_check_private_key(ctx)) {
                sslErr = ERR_peek_error();
            }
        }
    }

    SocketClient::~SocketClient() {
        if (ctx) {
            SSL_CTX_free(ctx);
            ctx = nullptr;
        }
    }

    // NOLINTNEXTLINE(google-default-arguments)
    void SocketClient::connect(const std::string& host, in_port_t port, const std::function<void(int err)>& onError,
                               const net::socket::InetAddress& localAddress) {
        this->onError = onError;
        if (sslErr != 0) {
            onError(-sslErr);
        } else {
            net::socket::SocketClient<SocketConnection>::connect(
                host, port,
                [this](int err) -> void {
                    if (err) {
                        this->onError(err);
                    }
                },
                localAddress);
        }
    }

    int SocketClient::passwordCallback(char* buf, int size, [[maybe_unused]] int rwflag, void* u) {
        strncpy(buf, static_cast<char*>(u), size);
        buf[size - 1] = '\0';

        memset(u, 0, ::strlen(static_cast<char*>(u))); // garble password
        free(u);

        return ::strlen(buf);
    }

}; // namespace net::socket::tls
