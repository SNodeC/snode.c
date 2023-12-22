/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021, 2022, 2023 Volker Christian <me@vchrist.at>
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

#include "core/socket/stream/SocketConnection.hpp" // IWYU pragma: export
#include "core/socket/stream/tls/SocketConnection.h"
#include "core/socket/stream/tls/TLSHandshake.h"
#include "core/socket/stream/tls/TLSShutdown.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "core/socket/stream/tls/ssl_utils.h"
#include "log/Logger.h"

#include <openssl/ssl.h>
#include <utility>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace core::socket::stream::tls {

    template <typename PhysicalSocket>
    SocketConnection<PhysicalSocket>::SocketConnection(const std::string& instanceName,
                                                       PhysicalSocket&& physicalSocket,
                                                       const std::function<void(SocketConnection*)>& onDisconnect,
                                                       const utils::Timeval& readTimeout,
                                                       const utils::Timeval& writeTimeout,
                                                       std::size_t readBlockSize,
                                                       std::size_t writeBlockSize,
                                                       const utils::Timeval& terminateTimeout)
        : Super(
              instanceName,
              std::move(physicalSocket),
              [onDisconnect, this]() -> void {
                  onDisconnect(this);
              },
              readTimeout,
              writeTimeout,
              readBlockSize,
              writeBlockSize,
              terminateTimeout) {
    }

    template <typename PhysicalSocket>
    SSL* SocketConnection<PhysicalSocket>::getSSL() const {
        return ssl;
    }

    template <typename PhysicalSocket>
    SSL* SocketConnection<PhysicalSocket>::startSSL(SSL_CTX* ctx,
                                                    const utils::Timeval& sslInitTimeout,
                                                    const utils::Timeval& sslShutdownTimeout) {
        this->sslInitTimeout = sslInitTimeout;
        this->sslShutdownTimeout = sslShutdownTimeout;
        if (ctx != nullptr) {
            ssl = SSL_new(ctx);

            if (ssl != nullptr) {
                if (SSL_set_fd(ssl, Super::physicalSocket.getFd()) == 1) {
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

    template <typename PhysicalSocket>
    void SocketConnection<PhysicalSocket>::stopSSL() {
        if (ssl != nullptr) {
            SSL_free(ssl);

            ssl = nullptr;
            SocketReader::ssl = nullptr;
            SocketWriter::ssl = nullptr;
        }
    }

    template <typename PhysicalSocket>
    void SocketConnection<PhysicalSocket>::doSSLHandshake(const std::function<void()>& onSuccess,
                                                          const std::function<void()>& onTimeout,
                                                          const std::function<void(int)>& onStatus) {
        if (!SocketReader::isSuspended()) {
            SocketReader::suspend();
        }
        if (!SocketWriter::isSuspended()) {
            SocketWriter::suspend();
        }

        TLSHandshake::doHandshake(
            ssl,
            [onSuccess, this]() -> void { // onSuccess
                SocketReader::span();
                onSuccess();
            },
            [onTimeout]() -> void { // onTimeout
                onTimeout();
            },
            [onStatus](int sslErr) -> void { // onStatus
                onStatus(sslErr);
            },
            sslInitTimeout);
    }

    template <typename PhysicalSocket>
    void SocketConnection<PhysicalSocket>::doSSLShutdown(const std::function<void()>& onSuccess,
                                                         const std::function<void()>& onTimeout,
                                                         const std::function<void(int)>& onStatus,
                                                         const utils::Timeval& shutdownTimeout) {
        bool resumeSocketReader = false;
        bool resumeSocketWriter = false;

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
            [onStatus, this, resumeSocketReader, resumeSocketWriter](int sslErr) -> void { // onStatus
                if (resumeSocketReader) {
                    SocketReader::resume();
                }
                if (resumeSocketWriter) {
                    SocketWriter::resume();
                }
                onStatus(sslErr);
            },
            shutdownTimeout);
    }

    template <typename PhysicalSocket>
    void SocketConnection<PhysicalSocket>::doSSLShutdown() {
        if (SSL_get_shutdown(ssl) == (SSL_SENT_SHUTDOWN | SSL_RECEIVED_SHUTDOWN)) {
            LOG(TRACE) << "SSL/TLS: SSL_Shutdown COMPLETED: Close_notify sent and received";
            if (SocketWriter::isEnabled()) {
                Super::doWriteShutdown([this]() -> void {
                    SocketWriter::disable();
                });
            }
        } else {
            LOG(TRACE) << "SSL/TLS: SSL_Shutdown WAITING: Close_notify received but not send";
        }
    }

    template <typename PhysicalSocket>
    void SocketConnection<PhysicalSocket>::doWriteShutdown(const std::function<void()>& onShutdown) {
        if ((SSL_get_shutdown(ssl) & SSL_SENT_SHUTDOWN) == 0) {
            doSSLShutdown(
                [this, &onShutdown]() -> void { // thus send one
                    if (SSL_get_shutdown(ssl) == (SSL_SENT_SHUTDOWN | SSL_RECEIVED_SHUTDOWN)) {
                        LOG(TRACE) << "SSL/TLS: SSL_Shutdown COMPLETED: Close_notify sent and received";
                        Super::doWriteShutdown(onShutdown);
                    } else {
                        LOG(TRACE) << "SSL/TLS: SSL_Shutdown WAITING: Close_notify sent but not received";
                    }
                },
                [this]() -> void {
                    LOG(TRACE) << "SSL/TLS: SSL_shutdown: Handshake timed out";
                    Super::doWriteShutdown([this]() -> void {
                        SocketConnection::close();
                    });
                },
                [this](int sslErr) -> void {
                    ssl_log("SSL_shutdown: Handshake failed", sslErr);
                    Super::doWriteShutdown([this]() -> void {
                        SocketConnection::close();
                    });
                },
                sslShutdownTimeout);
        } else {
            LOG(TRACE) << "SSL/TLS: SocketWriter::doWriteShutdown: Close_notify already send";
        }
    }

} // namespace core::socket::stream::tls
