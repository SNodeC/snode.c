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

#ifndef NET_SOCKET_SOCK_STREAM_TLS_SOCKETCONNECTION_H
#define NET_SOCKET_SOCK_STREAM_TLS_SOCKETCONNECTION_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "socket/sock_stream/SocketConnection.h"
#include "socket/sock_stream/tls/SocketReader.h"
#include "socket/sock_stream/tls/SocketWriter.h"

namespace net::socket::stream::tls {

    template <typename SocketT>
    class SocketConnection
        : public stream::SocketConnection<tls::SocketReader<SocketT>, tls::SocketWriter<SocketT>, typename SocketT::SocketAddress> {
    public:
        using Socket = SocketT;

    private:
        using SocketConnectionSuper =
            stream::SocketConnection<tls::SocketReader<Socket>, tls::SocketWriter<Socket>, typename Socket::SocketAddress>;

    public:
        SocketConnection(const std::function<void(SocketConnection* socketConnection)>& onConstruct,
                         const std::function<void(SocketConnection* socketConnection)>& onDestruct,
                         const std::function<void(SocketConnection* socketConnection, const char* junk, ssize_t junkLen)>& onRead,
                         const std::function<void(SocketConnection* socketConnection, int errnum)>& onReadError,
                         const std::function<void(SocketConnection* socketConnection, int errnum)>& onWriteError,
                         const std::function<void(SocketConnection* socketConnection)>& onDisconnect)
            : SocketConnectionSuper::SocketConnection(
                  []([[maybe_unused]] SocketConnectionSuper* socketConnection) -> void {
                  },
                  [onDestruct](SocketConnectionSuper* socketConnection) -> void {
                      onDestruct(static_cast<SocketConnection*>(socketConnection));
                  },
                  [onRead](SocketConnectionSuper* socketConnection, const char* junk, ssize_t junkLen) -> void {
                      onRead(static_cast<SocketConnection*>(socketConnection), junk, junkLen);
                  },
                  [onReadError, &sslErr = this->sslErr](SocketConnectionSuper* socketConnection, int errnum) -> void {
                      sslErr = errnum <= 0 ? -errnum : SSL_ERROR_SYSCALL;
                      onReadError(static_cast<SocketConnection*>(socketConnection), errnum);
                  },
                  [onWriteError, &sslErr = this->sslErr](SocketConnectionSuper* socketConnection, int errnum) -> void {
                      sslErr = errnum <= 0 ? -errnum : SSL_ERROR_SYSCALL;
                      onWriteError(static_cast<SocketConnection*>(socketConnection), errnum);
                  },
                  [onDisconnect](SocketConnectionSuper* socketConnection) -> void {
                      onDisconnect(static_cast<SocketConnection*>(socketConnection));
                  }) {
            onConstruct(this);
        }

    protected:
        SSL* startSSL(SSL_CTX* ctx) {
            if (ctx != nullptr) {
                ssl = SSL_new(ctx);

                if (ssl != nullptr) {
                    if (SSL_set_fd(ssl, Socket::getFd()) == 1) {
                        SocketReader<Socket>::ssl = ssl;
                        SocketWriter<Socket>::ssl = ssl;
                    } else {
                        SSL_free(ssl);
                        ssl = nullptr;
                    }
                }
            }

            return ssl;
        }

        void stopSSL() {
            if (ssl != nullptr) {
                if (sslErr == 0) {
                    SSL_shutdown(ssl);
                } else {
                    SSL_set_shutdown(ssl, SSL_SENT_SHUTDOWN | SSL_RECEIVED_SHUTDOWN);
                }
                SSL_free(ssl);

                ssl = nullptr;
                SocketReader<Socket>::ssl = nullptr;
                SocketWriter<Socket>::ssl = nullptr;
            }
        }

    public:
        SSL* getSSL() const {
            return ssl;
        }

    protected:
        void setSSLError(int sslErr) {
            this->sslErr = sslErr;
        }

        SSL* ssl = nullptr;

        int sslErr = SSL_ERROR_NONE;

        template <typename Socket>
        friend class SocketListener;

        template <typename Socket>
        friend class SocketConnector;
    };

} // namespace net::socket::stream::tls

#endif // NET_SOCKET_SOCK_STREAM_TLS_SOCKETCONNECTION_H
