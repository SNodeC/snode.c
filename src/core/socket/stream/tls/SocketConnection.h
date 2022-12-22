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

#include "core/socket/stream/SocketConnection.h"
#include "core/socket/stream/tls/SocketReader.h"
#include "core/socket/stream/tls/SocketWriter.h"
#include "core/socket/stream/tls/TLSShutdown.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#include <cstddef>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::socket::stream::tls {

    template <typename SocketT>
    class SocketConnection
        : public core::socket::stream::
              SocketConnection<SocketT, core::socket::stream::tls::SocketReader, core::socket::stream::tls::SocketWriter> {
    private:
        using Super = core::socket::stream::
            SocketConnection<SocketT, core::socket::stream::tls::SocketReader, core::socket::stream::tls::SocketWriter>;

    private:
        using Socket = SocketT;
        using SocketReader = typename Super::SocketReader;
        using SocketWriter = typename Super::SocketWriter;

    public:
        using SocketAddress = typename Super::SocketAddress;

        SocketConnection(int fd,
                         const std::shared_ptr<core::socket::SocketContextFactory>& socketContextFactory,
                         const SocketAddress& localAddress,
                         const SocketAddress& remoteAddress,
                         const std::function<void(SocketConnection*)>& onDisconnect,
                         const utils::Timeval& readTimeout,
                         const utils::Timeval& writeTimeout,
                         std::size_t readBlockSize,
                         std::size_t writeBlockSize,
                         const utils::Timeval& terminateTimeout)
            : Super(
                  fd,
                  socketContextFactory,
                  localAddress,
                  remoteAddress,
                  [onDisconnect, this]() -> void {
                      onDisconnect(this);
                  },
                  readTimeout,
                  writeTimeout,
                  readBlockSize,
                  writeBlockSize,
                  terminateTimeout) {
        }

        SSL* getSSL() const {
            return ssl;
        }

    private:
        ~SocketConnection() override = default;

        SSL* startSSL(SSL_CTX* ctx, const utils::Timeval& initTimeout, const utils::Timeval& shutdownTimeout) {
            this->initTimeout = initTimeout;
            this->shutdownTimeout = shutdownTimeout;
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
            if (!SocketReader::isSuspended()) {
                SocketReader::suspend();
            }
            if (!SocketWriter::isSuspended()) {
                SocketWriter::suspend();
            }

            TLSHandshake::doHandshake(
                ssl,
                [onSuccess, this]() -> void { // onSuccess
                    SocketReader::publish();
                    onSuccess();
                },
                [onTimeout, this]() -> void { // onTimeout
                    SocketConnection::close();
                    onTimeout();
                },
                [onError, this](int sslErr) -> void { // onError
                    SocketConnection::close();
                    onError(sslErr);
                },
                initTimeout);
        }

        void doSSLShutdown(const std::function<void()>& onSuccess,
                           const std::function<void()>& onTimeout,
                           const std::function<void(int)>& onError,
                           const utils::Timeval& shutdownTimeout) {
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
                [onSuccess, this, resumeSocketReader, resumeSocketWriter]() -> void { // onSuccess
                    if (resumeSocketReader) {
                        SocketReader::resume();
                    }
                    if (resumeSocketWriter) {
                        SocketWriter::resume();
                    }
                    onSuccess();
                },
                [onTimeout, this, resumeSocketReader, resumeSocketWriter]() -> void { // onTimeout
                    if (resumeSocketReader) {
                        SocketReader::resume();
                    }
                    if (resumeSocketWriter) {
                        SocketWriter::resume();
                    }
                    onTimeout();
                },
                [onError, this, resumeSocketReader, resumeSocketWriter](int sslErr) -> void { // onError
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
                if (SocketWriter::isEnabled()) {
                    SocketWriter::doWriteShutdown([this]([[maybe_unused]] int errnum) -> void {
                        if (errno != 0) {
                            PLOG(INFO) << "SocketWriter::doWriteShutdown";
                        }
                        SocketWriter::disable();
                    });
                }
            } else {
                VLOG(0) << "SSL_Shutdown WAITING: Close_notify received but not send";
            }
        }

        void doWriteShutdown(const std::function<void(int)>& onShutdown) override {
            if ((SSL_get_shutdown(ssl) & SSL_SENT_SHUTDOWN) == 0) {
                doSSLShutdown(
                    [this, &onShutdown]() -> void { // thus send one
                        if (SSL_get_shutdown(ssl) == (SSL_SENT_SHUTDOWN | SSL_RECEIVED_SHUTDOWN)) {
                            VLOG(0) << "SSL_Shutdown COMPLETED: Close_notify sent and received";
                            SocketWriter::doWriteShutdown(onShutdown);
                        } else {
                            VLOG(0) << "SSL_Shutdown WAITING: Close_notify sent but not received";
                        }
                    },
                    [this]() -> void {
                        LOG(WARNING) << "SSL_shutdown: Handshake timed out";
                        SocketWriter::doWriteShutdown([this]([[maybe_unused]] int errnum) -> void {
                            if (errno != 0) {
                                PLOG(INFO) << "SocketWriter::doWriteShutdown";
                            }
                            SocketConnection::close();
                        });
                    },
                    [this](int sslErr) -> void {
                        ssl_log("SSL_shutdown: Handshake failed", sslErr);
                        SocketWriter::doWriteShutdown([this]([[maybe_unused]] int errnum) -> void {
                            if (errno != 0) {
                                PLOG(INFO) << "SocketWriter::doWriteShutdown";
                            }
                            SocketConnection::close();
                        });
                    },
                    shutdownTimeout);
            }
        }

        SSL* ssl = nullptr;

        utils::Timeval initTimeout;
        utils::Timeval shutdownTimeout;

        template <typename ServerSocket>
        friend class SocketAcceptor;

        template <typename ClientSocket>
        friend class SocketConnector;
    };

} // namespace core::socket::stream::tls

#endif // CORE_SOCKET_STREAM_TLS_SOCKETCONNECTION_H
