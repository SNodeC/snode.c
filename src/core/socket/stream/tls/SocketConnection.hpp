/*
 * SNode.C - a slim toolkit for network communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024
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

#include "core/socket/stream/SocketConnection.hpp"
#include "core/socket/stream/tls/SocketConnection.h"
#include "core/socket/stream/tls/TLSHandshake.h"
#include "core/socket/stream/tls/TLSShutdown.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "core/socket/stream/tls/ssl_utils.h"
#include "log/Logger.h"

#include <openssl/ssl.h>
#include <string>
#include <utility>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace core::socket::stream::tls {

    template <typename PhysicalSocket>
    SocketConnection<PhysicalSocket>::SocketConnection(const std::string& instanceName,
                                                       PhysicalSocket&& physicalSocket,
                                                       const std::function<void(SocketConnection*)>& onDisconnect,
                                                       const std::string& configuredServer,
                                                       const SocketAddress& localAddress,
                                                       const SocketAddress& remoteAddress,
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
              configuredServer,
              localAddress,
              remoteAddress,
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
    SSL* SocketConnection<PhysicalSocket>::startSSL(int fd,
                                                    SSL_CTX* ctx,
                                                    const utils::Timeval& sslInitTimeout,
                                                    const utils::Timeval& sslShutdownTimeout) {
        this->sslInitTimeout = sslInitTimeout;
        this->sslShutdownTimeout = sslShutdownTimeout;
        if (ctx != nullptr) {
            ssl = SSL_new(ctx);

            if (ssl != nullptr) {
                if (SSL_set_fd(ssl, fd) == 1) {
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
    bool SocketConnection<PhysicalSocket>::doSSLHandshake(const std::function<void()>& onSuccess,
                                                          const std::function<void()>& onTimeout,
                                                          const std::function<void(int)>& onStatus) {
        if (ssl != nullptr) {
            if (!SocketReader::isSuspended()) {
                SocketReader::suspend();
            }
            if (!SocketWriter::isSuspended()) {
                SocketWriter::suspend();
            }

            TLSHandshake::doHandshake(
                Super::getInstanceName(),
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

        return ssl != nullptr;
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
            Super::getInstanceName(),
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
            LOG(TRACE) << Super::getInstanceName() << " SSL/TLS: Shutdown complete. Close_notify sent and received";
            if (SocketWriter::isEnabled()) {
                Super::doWriteShutdown([this]() -> void {
                    SocketWriter::disable();
                });
            }
        } else {
            LOG(TRACE) << Super::getInstanceName() << " SSL/TLS: Shutdown waiting. Close_notify received but not send";
        }
    }

    template <typename PhysicalSocket>
    void SocketConnection<PhysicalSocket>::doWriteShutdown(const std::function<void()>& onShutdown) {
        if ((SSL_get_shutdown(ssl) & SSL_SENT_SHUTDOWN) == 0) {
            doSSLShutdown(
                [this, &onShutdown]() -> void { // thus send one
                    if (SSL_get_shutdown(ssl) == (SSL_SENT_SHUTDOWN | SSL_RECEIVED_SHUTDOWN)) {
                        LOG(TRACE) << Super::getInstanceName() << " SSL/TLS: Shutdown complete. Close_notify sent and received";
                        Super::doWriteShutdown(onShutdown);
                    } else {
                        LOG(TRACE) << Super::getInstanceName() << " SSL/TLS: Shutdown waiting. Close_notify sent but not received";
                    }
                },
                [this]() -> void {
                    LOG(TRACE) << Super::getInstanceName() << " SSL/TLS: Shutdown handshake timed out";
                    Super::doWriteShutdown([this]() -> void {
                        SocketConnection::close();
                    });
                },
                [this](int sslErr) -> void {
                    ssl_log(Super::getInstanceName() + " SSL/TLS: Shutdown handshake failed", sslErr);
                    Super::doWriteShutdown([this]() -> void {
                        SocketConnection::close();
                    });
                },
                sslShutdownTimeout);
        } else {
            LOG(TRACE) << "SSL/TLS: Shutdown close_notify already send";
        }
    }

} // namespace core::socket::stream::tls
