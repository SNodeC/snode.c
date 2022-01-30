/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021, 2022 Volker Christian <me@vchrist.at>
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
#include "core/socket/stream/tls/TLSShutdown.h"

namespace core::socket::stream {
    template <typename ServerConfig, typename SocketConnection>
    class SocketAcceptor;
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#include <cstddef>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::socket::stream::tls {

    template <typename SocketT>
    class SocketConnection
        : public core::socket::stream::SocketConnection<core::socket::stream::tls::SocketReader<SocketT>,
                                                        core::socket::stream::tls::SocketWriter<SocketT>,
                                                        typename SocketT::SocketAddress> {
    private:
        using Super = core::socket::stream::SocketConnection<core::socket::stream::tls::SocketReader<SocketT>,
                                                             core::socket::stream::tls::SocketWriter<SocketT>,
                                                             typename SocketT::SocketAddress>;

        using SocketReader = core::socket::stream::tls::SocketReader<SocketT>;
        using SocketWriter = core::socket::stream::tls::SocketWriter<SocketT>;

    public:
        using Socket = SocketT;
        using SocketAddress = typename Super::SocketAddress;

        SocketConnection(int fd,
                         const std::shared_ptr<core::socket::SocketContextFactory>& socketContextFactory,
                         const SocketAddress& localAddress,
                         const SocketAddress& remoteAddress,
                         const std::function<void(SocketConnection*)>& onConnect,
                         const std::function<void(SocketConnection*)>& onDisconnect)
            : Super::Descriptor(fd)
            , Super(
                  socketContextFactory,
                  localAddress,
                  remoteAddress,
                  [onConnect, this]() -> void {
                      onConnect(this);
                  },
                  [onDisconnect, this]() -> void {
                      onDisconnect(this);
                  }) {
        }

        SSL* getSSL() const {
            return ssl;
        }

    private:
        ~SocketConnection() override = default;

        void setInitTimeout(const utils::Timeval& timeout) {
            initTimeout = timeout;
        }

        void setShutdownTimeout(const utils::Timeval& timeout) {
            shutdownTimeout = timeout;
        }

        SSL* startSSL(SSL_CTX* ctx) {
            if (ctx != nullptr) {
                ssl = SSL_new(ctx);

                if (ssl != nullptr) {
                    if (SSL_set_fd(ssl, Socket::getFd()) == 1) {
                        SocketReader::ssl = ssl;
                        SocketWriter::ssl = ssl;
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
                SSL_free(ssl);

                ssl = nullptr;
                SocketReader::ssl = nullptr;
                SocketWriter::ssl = nullptr;
            }
        }

        void doSSLHandshake(const std::function<void()>& onSuccess,
                            const std::function<void()>& onTimeout,
                            const std::function<void(int)>& onError) override {
            int resumeSocketReader = false;
            int resumeSocketWriter = false;

            if (!SocketReader::isSuspended()) {
                SocketReader::suspend();
                resumeSocketReader = true;
            }

            if (!SocketWriter::isSuspended()) {
                SocketWriter::suspend();
                resumeSocketWriter = true;
            }

            TLSHandshake::doHandshake(
                ssl,
                [onSuccess, this, resumeSocketReader, resumeSocketWriter](void) -> void { // onSuccess
                    if (resumeSocketReader) {
                        SocketReader::resume();
                    }
                    if (resumeSocketWriter) {
                        SocketWriter::resume();
                    }
                    onSuccess();
                },
                [onTimeout, this](void) -> void { // onTimeout
                    if (SocketReader::isEnabled()) {
                        SocketReader::disable();
                    }
                    if (SocketWriter::isEnabled()) {
                        SocketWriter::disable();
                    }
                    onTimeout();
                },
                [onError, this](int sslErr) -> void { // onError
                    setSSLError(sslErr);
                    if (SocketReader::isEnabled()) {
                        SocketReader::disable();
                    }
                    if (SocketWriter::isEnabled()) {
                        SocketWriter::disable();
                    }
                    onError(sslErr);
                },
                initTimeout);
        }

        void doSSLShutdown(const std::function<void()>& onSuccess,
                           const std::function<void()>& onTimeout,
                           const std::function<void(int)>& onError) {
            int resumeSocketReader = false;
            int resumeSocketWriter = false;

            if (!SocketReader::isSuspended()) {
                SocketReader::suspend();
                resumeSocketReader = true;
            }

            if (!SocketWriter::isSuspended()) {
                SocketWriter::suspend();
                resumeSocketWriter = true;
            }

            TLSShutdown::doShutdown(
                ssl,
                [onSuccess, this, resumeSocketReader, resumeSocketWriter](void) -> void { // onSuccess
                    if (resumeSocketReader) {
                        SocketReader::resume();
                    }
                    if (resumeSocketWriter) {
                        SocketWriter::resume();
                    }
                    onSuccess();
                },
                [onTimeout, this, resumeSocketReader, resumeSocketWriter](void) -> void { // onTimeout
                    if (resumeSocketReader) {
                        SocketReader::resume();
                    }
                    if (resumeSocketWriter) {
                        SocketWriter::resume();
                    }
                    onTimeout();
                },
                [onError, this, resumeSocketReader, resumeSocketWriter](int sslErr) -> void { // onError
                    setSSLError(sslErr);
                    if (resumeSocketReader) {
                        SocketReader::resume();
                    }
                    if (resumeSocketWriter) {
                        SocketWriter::resume();
                    }
                    onError(sslErr);
                },
                shutdownTimeout);
        }

        void doReadShutdown() override {
            if (SSL_get_shutdown(ssl) == (SSL_SENT_SHUTDOWN | SSL_RECEIVED_SHUTDOWN)) {
                VLOG(0) << "SSL_Shutdown COMPLETED: Close_notify sent and received";
                SocketWriter::doWriteShutdown();
            } else {
                VLOG(0) << "SSL_Shutdown WAITING: Close_notify received but not send";
            }
        }

        void doWriteShutdown() override {
            if ((SSL_get_shutdown(ssl) & SSL_SENT_SHUTDOWN) == 0) {
                doSSLShutdown(
                    [this]() -> void { // thus send one
                        if (SSL_get_shutdown(ssl) == (SSL_SENT_SHUTDOWN | SSL_RECEIVED_SHUTDOWN)) {
                            VLOG(0) << "SSL_Shutdown COMPLETED: Close_notify sent and received";
                            SocketWriter::doWriteShutdown();
                        } else {
                            VLOG(0) << "SSL_Shutdown WAITING: Close_notify sent but not received";
                        }
                    },
                    [this]() -> void {
                        LOG(WARNING) << "SSL_shutdown: Handshake timed out";
                        SocketWriter::disable();
                    },
                    [this](int sslErr) -> void {
                        ssl_log("SSL_shutdown: Handshake failed", sslErr);
                        SocketWriter::disable();
                    });
            }
        }

        void setSSLError(int sslErr) {
            this->sslErr = sslErr;
        }

        SSL* ssl = nullptr;

        int sslErr = SSL_ERROR_NONE;

        utils::Timeval initTimeout;
        utils::Timeval shutdownTimeout;

        template <typename ServerConfig, typename Socket>
        friend class SocketAcceptor;

        template <typename ServerConfig, typename SocketConnection>
        friend class core::socket::stream::SocketAcceptor;

        template <typename Socket>
        friend class SocketConnector;
    };

} // namespace core::socket::stream::tls

#endif // CORE_SOCKET_STREAM_TLS_SOCKETCONNECTION_H
