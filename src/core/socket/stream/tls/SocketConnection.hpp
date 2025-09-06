/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025
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

/*
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
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

    template <typename Config, typename PhysicalSocket>
    SocketConnection<Config, PhysicalSocket>::SocketConnection(const std::shared_ptr<Config>& config,
                                                               PhysicalSocket&& physicalSocket,
                                                               const std::function<void(SocketConnection*)>& onDisconnect)
        : Super(config, std::move(physicalSocket), [onDisconnect, this]() {
            onDisconnect(this);
        }) {
    }

    template <typename Config, typename PhysicalSocket>
    SSL* SocketConnection<Config, PhysicalSocket>::getSSL() const {
        return ssl;
    }

    template <typename Config, typename PhysicalSocket>
    SSL* SocketConnection<Config, PhysicalSocket>::startSSL(
        int fd, SSL_CTX* ctx, const utils::Timeval& sslInitTimeout, const utils::Timeval& sslShutdownTimeout, bool closeNotifyIsEOF) {
        this->sslInitTimeout = sslInitTimeout;
        this->sslShutdownTimeout = sslShutdownTimeout;
        if (ctx != nullptr) {
            ssl = SSL_new(ctx);

            if (ssl != nullptr) {
                SSL_set_ex_data(ssl, 0, const_cast<std::string*>(&Super::getConnectionName()));

                if (SSL_set_fd(ssl, fd) == 1) {
                    SocketReader::ssl = ssl;
                    SocketWriter::ssl = ssl;
                    SocketReader::closeNotifyIsEOF = closeNotifyIsEOF;
                    SocketWriter::closeNotifyIsEOF = closeNotifyIsEOF;
                } else {
                    SSL_free(ssl);
                    ssl = nullptr;
                }
            }
        }

        return ssl;
    }

    template <typename Config, typename PhysicalSocket>
    void SocketConnection<Config, PhysicalSocket>::stopSSL() {
        if (ssl != nullptr) {
            SSL_free(ssl);

            ssl = nullptr;
            SocketReader::ssl = nullptr;
            SocketWriter::ssl = nullptr;
        }
    }

    template <typename Config, typename PhysicalSocket>
    bool SocketConnection<Config, PhysicalSocket>::doSSLHandshake(const std::function<void()>& onSuccess,
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
                Super::getConnectionName(),
                ssl,
                [onSuccess, this]() { // onSuccess
                    SocketReader::span();
                    onSuccess();
                },
                [onTimeout]() { // onTimeout
                    onTimeout();
                },
                [onStatus](int sslErr) { // onStatus
                    onStatus(sslErr);
                },
                sslInitTimeout);
        }

        return ssl != nullptr;
    }

    template <typename Config, typename PhysicalSocket>
    void SocketConnection<Config, PhysicalSocket>::doSSLShutdown() {
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
            Super::getConnectionName(),
            ssl,
            [this, resumeSocketReader, resumeSocketWriter]() { // onSuccess
                if (resumeSocketReader) {
                    SocketReader::resume();
                }
                if (resumeSocketWriter) {
                    SocketWriter::resume();
                }
                if (SSL_get_shutdown(ssl) == (SSL_SENT_SHUTDOWN | SSL_RECEIVED_SHUTDOWN)) {
                    LOG(DEBUG) << Super::getConnectionName() << " SSL/TLS: Close_notify received and sent";
                } else {
                    LOG(DEBUG) << Super::getConnectionName() << " SSL/TLS: Close_notify sent";

                    if (SSL_get_shutdown(ssl) == SSL_SENT_SHUTDOWN && SocketWriter::closeNotifyIsEOF) {
                        LOG(TRACE) << Super::getConnectionName() << " SSL/TLS: Close_notify is EOF: setting sslShutdownTimeout to "
                                   << sslShutdownTimeout << " sec";
                        Super::setTimeout(sslShutdownTimeout);
                    }
                }
            },
            [this, resumeSocketReader, resumeSocketWriter]() { // onTimeout
                if (resumeSocketReader) {
                    SocketReader::resume();
                }
                if (resumeSocketWriter) {
                    SocketWriter::resume();
                }
                LOG(ERROR) << Super::getConnectionName() << " SSL/TLS: Shutdown handshake timed out";
                Super::doWriteShutdown([this]() {
                    SocketConnection::close();
                });
            },
            [this, resumeSocketReader, resumeSocketWriter](int sslErr) { // onStatus
                if (resumeSocketReader) {
                    SocketReader::resume();
                }
                if (resumeSocketWriter) {
                    SocketWriter::resume();
                }
                ssl_log(Super::getConnectionName() + " SSL/TLS: Shutdown handshake failed", sslErr);
                Super::doWriteShutdown([this]() {
                    SocketConnection::close();
                });
            },
            sslShutdownTimeout);
    }

    template <typename Config, typename PhysicalSocket>
    void SocketConnection<Config, PhysicalSocket>::onReadShutdown() {
        if ((SSL_get_shutdown(ssl) & SSL_RECEIVED_SHUTDOWN) != 0) {
            if ((SSL_get_shutdown(ssl) & SSL_SENT_SHUTDOWN) != 0) {
                LOG(DEBUG) << Super::getConnectionName() << " SSL/TLS: Close_notify sent and received";

                SocketWriter::shutdownInProgress = false;
            } else {
                LOG(DEBUG) << Super::getConnectionName() << " SSL/TLS: Close_notify received";

                doSSLShutdown();
            }
        } else {
            LOG(ERROR) << Super::getConnectionName() << " SSL/TLS: Unexpected EOF error";

            SocketWriter::shutdownInProgress = false;
            SSL_set_shutdown(ssl, SSL_SENT_SHUTDOWN | SSL_RECEIVED_SHUTDOWN);
        }
    }

    template <typename Config, typename PhysicalSocket>
    void SocketConnection<Config, PhysicalSocket>::doWriteShutdown(const std::function<void()>& onShutdown) {
        if ((SSL_get_shutdown(ssl) & SSL_SENT_SHUTDOWN) == 0) {
            LOG(DEBUG) << Super::getConnectionName() << " SSL/TLS: Send close_notify";

            doSSLShutdown();
        } else {
            Super::doWriteShutdown(onShutdown);
        }
    }

} // namespace core::socket::stream::tls
