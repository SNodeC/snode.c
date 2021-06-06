/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021 Volker Christian <me@vchrist.at>
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

#ifndef NET_SOCKET_STREAM_TLS_SOCKETCONNECTION_H
#define NET_SOCKET_STREAM_TLS_SOCKETCONNECTION_H

#include "log/Logger.h"
#include "net/socket/stream/SocketConnection.h"
#include "net/socket/stream/tls/SocketReader.h"
#include "net/socket/stream/tls/SocketWriter.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::socket::stream::tls {

    template <typename SocketT>
    class SocketConnection
        : public stream::SocketConnection<tls::SocketReader<SocketT>, tls::SocketWriter<SocketT>, typename SocketT::SocketAddress> {
    public:
        using Socket = SocketT;
        using SocketAddress = typename Socket::SocketAddress;

    public:
        SocketConnection(int fd,
                         const std::shared_ptr<const SocketContextFactory>& socketProtocolFactory,
                         const SocketAddress& localAddress,
                         const SocketAddress& remoteAddress,
                         const std::function<void(const SocketAddress& localAddress, const SocketAddress& remoteAddress)>& onConnect,
                         const std::function<void(SocketConnection* socketConnection)>& onDisconnect)
            : stream::SocketConnection<tls::SocketReader<Socket>, tls::SocketWriter<Socket>, typename Socket::SocketAddress>::
                  SocketConnection(fd, socketProtocolFactory, localAddress, remoteAddress, onConnect, [onDisconnect, this]() -> void {
                      onDisconnect(this);
                  }) {
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

        void doSSLHandshake(const std::function<void()>& onSuccess,
                            const std::function<void()>& onTimeout,
                            const std::function<void(int sslErr)>& onError) override {
            SocketConnection::SocketReader::suspend();
            SocketConnection::SocketWriter::suspend();

            TLSHandshake::doHandshake(
                ssl,
                [onSuccess, this](void) -> void { // onSuccess
                    SocketConnection::SocketReader::resume();
                    SocketConnection::SocketWriter::resume();
                    onSuccess();
                },
                [onTimeout, this](void) -> void { // onTimeout
                    if (SocketConnection::SocketReader::isEnabled()) {
                        SocketConnection::SocketReader::disable();
                    }
                    if (SocketConnection::SocketWriter::isEnabled()) {
                        SocketConnection::SocketWriter::disable();
                    }
                    onTimeout();
                },
                [onError, this](int sslErr) -> void { // onError
                    setSSLError(sslErr);
                    if (SocketConnection::SocketReader::isEnabled()) {
                        SocketConnection::SocketReader::disable();
                    }
                    if (SocketConnection::SocketWriter::isEnabled()) {
                        SocketConnection::SocketWriter::disable();
                    }
                    onError(sslErr);
                });
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

        std::string serverNameIndication;

        int sslErr = SSL_ERROR_NONE;

    private:
        template <typename Socket>
        friend class SocketListener;

        template <typename Socket>
        friend class SocketConnector;
    };

} // namespace net::socket::stream::tls

#endif // NET_SOCKET_STREAM_TLS_SOCKETCONNECTION_H
