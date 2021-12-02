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

#ifndef CORE_SOCKET_STREAM_TLS_SOCKETCONNECTION_H
#define CORE_SOCKET_STREAM_TLS_SOCKETCONNECTION_H

#include "core/socket/stream/SocketConnection.h" // IWYU pragma: export
#include "core/socket/stream/tls/SocketReader.h"
#include "core/socket/stream/tls/SocketWriter.h"
#include "log/Logger.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::socket::stream::tls {

    template <typename SocketT>
    class SocketConnection
        : public core::socket::stream::SocketConnection<core::socket::stream::tls::SocketReader<SocketT>,
                                                        core::socket::stream::tls::SocketWriter<SocketT>,
                                                        typename SocketT::SocketAddress> {
    public:
        using Socket = SocketT;
        using SocketAddress = typename Socket::SocketAddress;

        SocketConnection(int fd,
                         const std::shared_ptr<SocketContextFactory>& socketContextFactory,
                         const SocketAddress& localAddress,
                         const SocketAddress& remoteAddress,
                         const std::function<void(const SocketAddress&, const SocketAddress&)>& onConnect,
                         const std::function<void(SocketConnection*)>& onDisconnect)
            : SocketConnection::Descriptor(fd)
            , core::socket::stream::SocketConnection<core::socket::stream::tls::SocketReader<Socket>,
                                                     core::socket::stream::tls::SocketWriter<Socket>,
                                                     typename Socket::SocketAddress>::SocketConnection(socketContextFactory,
                                                                                                       localAddress,
                                                                                                       remoteAddress,
                                                                                                       onConnect,
                                                                                                       [onDisconnect, this]() -> void {
                                                                                                           onDisconnect(this);
                                                                                                       }) {
        }

        SSL* getSSL() const {
            return ssl;
        }

    private:
        SSL* startSSL(SSL_CTX* ctx) {
            if (ctx != nullptr) {
                ssl = SSL_new(ctx);

                if (ssl != nullptr) {
                    if (SSL_set_fd(ssl, Socket::getFd()) == 1) {
                        SocketConnection::SocketReader::ssl = ssl;
                        SocketConnection::SocketWriter::ssl = ssl;
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
                            const std::function<void(int)>& onError) override {
            int resumeSocketReader = false;
            int resumeSocketWriter = false;

            if (!SocketConnection::SocketReader::isSuspended()) {
                SocketConnection::SocketReader::suspend();
                resumeSocketReader = true;
            }

            if (!SocketConnection::SocketWriter::isSuspended()) {
                SocketConnection::SocketWriter::suspend();
                resumeSocketWriter = true;
            }

            TLSHandshake::doHandshake(
                ssl,
                [onSuccess, this, resumeSocketReader, resumeSocketWriter](void) -> void { // onSuccess
                    if (resumeSocketReader) {
                        SocketConnection::SocketReader::resume();
                    }
                    if (resumeSocketWriter) {
                        SocketConnection::SocketWriter::resume();
                    }
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
                SSL_free(ssl);

                ssl = nullptr;
                SocketConnection::SocketReader::ssl = nullptr;
                SocketConnection::SocketWriter::ssl = nullptr;
            }
        }

        void setSSLError(int sslErr) {
            this->sslErr = sslErr;
        }

        SSL* ssl = nullptr;

        int sslErr = SSL_ERROR_NONE;

        template <typename Socket>
        friend class SocketAcceptor;

        template <typename Socket>
        friend class SocketConnector;
    };

} // namespace core::socket::stream::tls

#endif // CORE_SOCKET_STREAM_TLS_SOCKETCONNECTION_H
