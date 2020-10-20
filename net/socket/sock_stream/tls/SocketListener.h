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
#include "TLSHandshake.h"
#include "socket/sock_stream/SocketListener.h"
#include "ssl_utils.h"

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
                      SSL* ssl = socketConnection->startSSL(ctx);

                      if (ssl != nullptr) {
                          int ret = SSL_accept(ssl);
                          int sslErr = SSL_get_error(ssl, ret);

                          if (sslErr == SSL_ERROR_WANT_READ || sslErr == SSL_ERROR_WANT_WRITE) {
                              TLSHandshake::doHandshake(
                                  ssl,
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
                          } else if (sslErr == SSL_ERROR_NONE) {
                              onConnect(socketConnection);
                          } else {
                              socketConnection->ReadEventReceiver::disable();
                              PLOG(ERROR) << "TLS accept failed: " << ERR_error_string(ERR_get_error(), nullptr);
                          }
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
