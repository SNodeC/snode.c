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
#include <easylogging++.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "socket/tls/SocketClient.h"
#include "timer/SingleshotTimer.h"

#define TLSCONNECT_TIMEOUT 10

namespace net::socket::tls {

    SocketClient::SocketClient(
        const std::function<void(SocketClient::SocketConnection* socketConnection)>& onConnect,
        const std::function<void(SocketClient::SocketConnection* socketConnection)>& onDisconnect,
        const std::function<void(SocketClient::SocketConnection* socketConnection, const char* junk, ssize_t junkLen)>& onRead,
        const std::function<void(SocketClient::SocketConnection* socketConnection, int errnum)>& onReadError,
        const std::function<void(SocketClient::SocketConnection* socketConnection, int errnum)>& onWriteError,
        const std::map<std::string, std::any>& options)
        : socket::SocketClient<SocketClient::SocketConnection>(
              [this, onConnect](SocketClient::SocketConnection* socketConnection) -> void {
                  class TLSConnector
                      : public ReadEventReceiver
                      , public WriteEventReceiver
                      , public Socket {
                  public:
                      TLSConnector(tls::SocketClient* socketClient, SocketClient::SocketConnection* socketConnection, SSL_CTX* ctx,
                                   const std::function<void(tls::SocketConnection* socketConnection)>& onConnect)
                          : socketClient(socketClient)
                          , socketConnection(socketConnection)
                          , onConnect(onConnect)
                          , timeOut(timer::Timer::singleshotTimer(
                                [this]([[maybe_unused]] const void* arg) -> void {
                                    ReadEventReceiver::disable();
                                    WriteEventReceiver::disable();
                                    this->socketClient->onError(ETIMEDOUT);
                                    this->socketConnection->ReadEventReceiver::disable();
                                },
                                (struct timeval){TLSCONNECT_TIMEOUT, 0}, nullptr)) {
                          open(socketConnection->getFd(), FLAGS::dontClose);
                          ssl = socketConnection->startSSL(ctx);
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
                                  unobserved();
                              }
                          } else {
                              socketClient->onError(-ERR_peek_error());
                              socketConnection->ReadEventReceiver::disable();
                              timeOut.cancel();
                              unobserved();
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
                      tls::SocketClient* socketClient = nullptr;
                      tls::SocketConnection* socketConnection = nullptr;
                      SSL* ssl = nullptr;
                      std::function<void(SocketClient::SocketConnection* socketConnection)> onConnect;
                      timer::Timer& timeOut;
                  };

                  new TLSConnector(this, socketConnection, ctx, onConnect);
              },
              [onDisconnect](SocketClient::SocketConnection* socketConnection) -> void {
                  onDisconnect(socketConnection);
                  socketConnection->stopSSL();
              },
              onRead, onReadError, onWriteError, options) {
        std::string certChain = "";
        std::string keyPEM = "";
        std::string password = "";
        std::string caFile = "";
        std::string caDir = "";
        bool useDefaultCADir = false;

        for (auto& [name, value] : options) {
            if (name == "certChain") {
                certChain = std::any_cast<const char*>(value);
            } else if (name == "keyPEM") {
                keyPEM = std::any_cast<const char*>(value);
            } else if (name == "password") {
                password = std::any_cast<const char*>(value);
            } else if (name == "caFile") {
                caFile = std::any_cast<const char*>(value);
            } else if (name == "caDir") {
                caDir = std::any_cast<const char*>(value);
            } else if (name == "useDefaultCADir") {
                useDefaultCADir = std::any_cast<bool>(value);
            }
        }
        ctx = SSL_CTX_new(TLS_client_method());
        if (ctx != nullptr) {
            if (!caFile.empty() || !caDir.empty()) {
                if (!SSL_CTX_load_verify_locations(ctx, !caFile.empty() ? caFile.c_str() : nullptr,
                                                   !caDir.empty() ? caDir.c_str() : nullptr)) {
                    sslErr = ERR_peek_error();
                }
            }
            if (sslErr == SSL_ERROR_NONE && useDefaultCADir) {
                if (!SSL_CTX_set_default_verify_paths(ctx)) {
                    sslErr = ERR_peek_error();
                }
            }
            if (sslErr == SSL_ERROR_NONE) {
                if (!certChain.empty()) {
                    if (SSL_CTX_use_certificate_chain_file(ctx, certChain.c_str()) <= 0) {
                        sslErr = ERR_peek_error();
                    } else if (!keyPEM.empty()) {
                        if (!password.empty()) {
                            SSL_CTX_set_default_passwd_cb(ctx, SocketClient::passwordCallback);
                            SSL_CTX_set_default_passwd_cb_userdata(ctx, ::strdup(password.c_str()));
                        }
                        if (SSL_CTX_use_PrivateKey_file(ctx, keyPEM.c_str(), SSL_FILETYPE_PEM) <= 0) {
                            sslErr = ERR_peek_error();
                        } else if (!SSL_CTX_check_private_key(ctx)) {
                            sslErr = ERR_peek_error();
                        }
                    }
                }
            }
        } else {
            sslErr = ERR_peek_error();
        }
    }

    SocketClient::~SocketClient() {
        if (ctx != nullptr) {
            SSL_CTX_free(ctx);
            ctx = nullptr;
        }
    }

    // NOLINTNEXTLINE(google-default-arguments)
    void SocketClient::connect(const std::map<std::string, std::any>& options, const std::function<void(int err)>& onError,
                               const socket::InetAddress& localAddress) {
        this->onError = onError;
        if (sslErr != 0) {
            onError(-sslErr);
        } else {
            socket::SocketClient<SocketClient::SocketConnection>::connect(
                options,
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
