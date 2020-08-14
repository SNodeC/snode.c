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

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "socket/tls/SocketClient.h"

#include "timer/SingleshotTimer.h"

#define TLSCONNECT_TIMEOUT 10

namespace tls {

    SocketClient::SocketClient(const std::function<void(tls::SocketConnection* cs)>& onConnect,
                               const std::function<void(tls::SocketConnection* cs)>& onDisconnect,
                               const std::function<void(tls::SocketConnection* cs, const char* junk, ssize_t n)>& onRead,
                               const std::function<void(tls::SocketConnection* cs, int errnum)>& onReadError,
                               const std::function<void(tls::SocketConnection* cs, int errnum)>& onWriteError)
        : ::SocketClient<tls::SocketConnection>(
              [this, onConnect](tls::SocketConnection* cs) -> void {
                  class TLSConnector
                      : public ReadEventReceiver
                      , public WriteEventReceiver
                      , public Socket {
                  public:
                      TLSConnector(tls::SocketClient* sc, tls::SocketConnection* cs, SSL_CTX* ctx,
                                   const std::function<void(tls::SocketConnection* cs)>& onConnect)
                          : Descriptor(true)
                          , sc(sc)
                          , cs(cs)
                          , ssl(cs->startSSL(ctx))
                          , onConnect(onConnect)
                          , timeOut(Timer::singleshotTimer(
                                [this]([[maybe_unused]] const void* arg) -> void {
                                    this->ReadEventReceiver::disable();
                                    this->WriteEventReceiver::disable();
                                    this->sc->onError(ETIMEDOUT);
                                    this->cs->ReadEventReceiver::disable();
                                },
                                (struct timeval){TLSCONNECT_TIMEOUT, 0}, nullptr)) {
                          this->attachFd(cs->getFd());

                          int err = SSL_connect(ssl);
                          int sslErr = SSL_get_error(ssl, err);

                          if (sslErr == SSL_ERROR_WANT_READ) {
                              this->ReadEventReceiver::enable();
                          } else if (sslErr == SSL_ERROR_WANT_WRITE) {
                              this->WriteEventReceiver::enable();
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
                                  this->ReadEventReceiver::disable();
                                  this->WriteEventReceiver::enable();
                              } else {
                                  timeOut.cancel();
                                  this->ReadEventReceiver::disable();
                                  if (sslErr == SSL_ERROR_NONE) {
                                      sc->onError(0);
                                      this->onConnect(cs);
                                  } else {
                                      sc->onError(-sslErr);
                                      cs->ReadEventReceiver::disable();
                                  }
                              }
                          }
                      }

                      void writeEvent() override {
                          int err = SSL_connect(ssl);
                          int sslErr = SSL_get_error(ssl, err);

                          if (sslErr != SSL_ERROR_WANT_WRITE) {
                              if (sslErr == SSL_ERROR_WANT_READ) {
                                  this->WriteEventReceiver::disable();
                                  this->ReadEventReceiver::enable();
                              } else {
                                  timeOut.cancel();
                                  this->WriteEventReceiver::disable();
                                  if (sslErr == SSL_ERROR_NONE) {
                                      sc->onError(0);
                                      this->onConnect(cs);
                                  } else {
                                      sc->onError(-sslErr);
                                      cs->ReadEventReceiver::disable();
                                  }
                              }
                          }
                      }

                      void unobserved() override {
                          delete this;
                      }

                  private:
                      tls::SocketClient* sc = nullptr;
                      tls::SocketConnection* cs = nullptr;
                      SSL* ssl = nullptr;
                      std::function<void(tls::SocketConnection* cs)> onConnect;
                      Timer& timeOut;
                  };

                  new TLSConnector(this, cs, ctx, onConnect);

                  /*
                   *   X509* client_cert = SSL_get_peer_certificate(ssl);
                   *   if (client_cert != NULL) {
                   *       printf("Client certificate:\n");
                   *
                   *       char* str = X509_NAME_oneline(X509_get_subject_name(client_cert), 0, 0);
                   *       printf("\t subject: %s\n", str);
                   *       OPENSSL_free(str);
                   *
                   *       str = X509_NAME_oneline(X509_get_issuer_name(client_cert), 0, 0);
                   *       printf("\t issuer: %s\n", str);
                   *       OPENSSL_free(str);
                   *
                   *       // We could do all sorts of certificate verification stuff here before deallocating the certificate.
                   *
                   *       X509_free(client_cert);
                   *   } else {
                   *       printf("Client does not have certificate.\n");
                   *   }
                   */
              },
              [onDisconnect](tls::SocketConnection* cs) -> void {
                  cs->stopSSL();
                  onDisconnect(cs);
              },
              onRead, onReadError, onWriteError) {
        ctx = SSL_CTX_new(TLS_client_method());
        if (!ctx) {
            ERR_print_errors_fp(stderr);
            sslErr = ERR_get_error();
        } else {
            /*
             *   SSL_CTX_set_default_passwd_cb(ctx, SocketServer::passwordCallback);
             *   SSL_CTX_set_default_passwd_cb_userdata(ctx, ::strdup(password.c_str()));
             *   if (SSL_CTX_use_certificate_chain_file(ctx, certChain.c_str()) <= 0) {
             *       ERR_print_errors_fp(stderr);
             *       sslErr = ERR_get_error();
             *   } else if (SSL_CTX_use_PrivateKey_file(ctx, keyPEM.c_str(), SSL_FILETYPE_PEM) <= 0) {
             *       ERR_print_errors_fp(stderr);
             *       sslErr = ERR_get_error();
             *   } else if (!SSL_CTX_check_private_key(ctx)) {
             *       sslErr = ERR_get_error();
             *   }
             */
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
                               const InetAddress& localAddress) {
        this->onError = onError;
        if (sslErr != 0) {
            onError(-sslErr);
        } else {
            ::SocketClient<tls::SocketConnection>::connect(
                host, port,
                [this](int err) -> void {
                    if (err) {
                        this->onError(err);
                    }
                },
                localAddress);
        }
    }

}; // namespace tls
