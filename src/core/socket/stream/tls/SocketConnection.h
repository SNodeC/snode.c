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
                });
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
                });
        }

        void doShutdown() override {
            int sh = SSL_get_shutdown(ssl);

            if ((sh & SSL_SENT_SHUTDOWN) || (sh & SSL_RECEIVED_SHUTDOWN)) { // Did we already received or sent close_notify?
                VLOG(0) << "SSL_shutdown: Close_notify received and/or already sent ... checking: sh =  " << sh; // We already have sent or
                                                                                                                 // received close_notify.
                if (sh == SSL_RECEIVED_SHUTDOWN) { // We have only received close_notify
                    VLOG(0) << "SSL_shutdown: Close_notify received but not sent, now send close_notify"; // We can get a TCP-RST if peer
                                                                                                          // immediately shuts down the read
                                                                                                          // side after sending
                                                                                                          // close_notify. Thus, peer is not
                                                                                                          // waiting for our close_notify.
                                                                                                          // E.g. firefox behaves like that
                    doSSLShutdown(
                        [this]() -> void {                                                            // Send our close_notify
                            VLOG(0) << "SSL_shutdown: Shutdown completed. Closing underlying Writer"; // SSL shutdown completed!
                            SocketWriter::doShutdown();                                               // So shutdown the underlying Writer
                        },
                        []() -> void {
                            LOG(WARNING) << "SSL/TLS shutdown handshake timed out";
                        },
                        [](int sslErr) -> void {
                            ssl_log("SSL/TLS shutdown handshake failed", sslErr);
                        });
                } else if (sh == SSL_SENT_SHUTDOWN) {
                    if (SocketReader::isEnabled()) {
                        VLOG(0) << "Pending: receiving of close notify";
                    } else {
                        VLOG(0) << "Pending: closing underlying Writer because we can not receive the close_notify reply";
                        SocketWriter::doShutdown();
                    }
                } else { // Both close_notify present
                    VLOG(0) << "SSL_shutdown: Close_notify received and sent";
                    VLOG(0) << "SSL_shutdown: Shutdown completed."; // Closing underlying Writer";
                    //                    SocketWriter::doShutdown(); // SSL shutdown completed - shutdown the underlying socket
                }
            } else { // We neighter have sent nor received close_notify
                VLOG(0) << "SSL_shutdown: Close_notify neither received nor sent: sh = " << sh;
                VLOG(0) << "SSL_shutdown: Good: Beeing the first to send close_notify";
                doSSLShutdown(
                    [this]() -> void { // thus send one
                        VLOG(0) << "SSL_shutdown: Close_notify sent.";
                        if (SocketReader::isEnabled()) {
                            VLOG(0) << "SSL_shutdown: Waiting for peer's close_notify";
                            SocketWriter::doShutdown(); // SSL shutdown completed - shutdown the underlying socket
                        } else {
                            VLOG(0) << "SSL_shutdown: Shutdown underlying Writer because we can not receive the close_notify reply";
                            SocketWriter::doShutdown();
                        }
                    },
                    []() -> void {
                        LOG(WARNING) << "SSL/TLS shutdown handshake timed out";
                    },
                    [](int sslErr) -> void {
                        ssl_log("SSL/TLS shutdown handshake failed", sslErr);
                    });
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
