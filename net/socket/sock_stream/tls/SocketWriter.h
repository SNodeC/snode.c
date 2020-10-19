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

#ifndef NET_SOCKET_SOCK_STREAM_TLS_SOCKETWRITER_H
#define NET_SOCKET_SOCK_STREAM_TLS_SOCKETWRITER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef> // for size_t
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <sys/types.h> // for ssize_t

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "Descriptor.h"
#include "Logger.h"
#include "ReadEventReceiver.h"
#include "socket/sock_stream/SocketWriter.h"

#define TLSHANDSHAKE_TIMEOUT 10

namespace net::socket::stream::tls {

    template <typename SocketT>
    class SocketWriter : public socket::stream::SocketWriter<SocketT> {
    protected:
        using socket::stream::SocketWriter<SocketT>::SocketWriter;

        ssize_t write(const char* junk, size_t junkLen) override {
            int ret = ::SSL_write(ssl, junk, junkLen);

            if (ret <= 0) {
                int sslErr = SSL_get_error(ssl, ret);

                switch (sslErr) {
                    case SSL_ERROR_WANT_WRITE:
                    case SSL_ERROR_WANT_READ:
                        class TLSHandshaker
                            : public ReadEventReceiver
                            , public WriteEventReceiver
                            , public Descriptor {
                        public:
                            TLSHandshaker(SSL* ssl,
                                          int sslErr,
                                          const std::function<void(void)>& onSuccess,
                                          const std::function<void(void)>& onTimeout,
                                          const std::function<void(unsigned long)>& onError)
                                : ssl(ssl)
                                , onSuccess(onSuccess)
                                , onTimeout(onTimeout)
                                , onError(onError) {
                                this->ReadEventReceiver::setTimeout(TLSHANDSHAKE_TIMEOUT);
                                this->WriteEventReceiver::setTimeout(TLSHANDSHAKE_TIMEOUT);

                                this->open(SSL_get_fd(ssl), FLAGS::dontClose);

                                switch (sslErr) {
                                    case SSL_ERROR_WANT_READ:
                                        ReadEventReceiver::enable();
                                        break;
                                    case SSL_ERROR_WANT_WRITE:
                                        WriteEventReceiver::enable();
                                        break;
                                    case SSL_ERROR_NONE:
                                        ReadEventReceiver::disable();
                                        onSuccess();
                                        delete this;
                                        break;
                                    default:
                                        onError(ERR_peek_error());
                                        delete this;
                                }
                            }

                            void readEvent() override {
                                int ret = SSL_do_handshake(ssl);
                                int sslErr = SSL_get_error(ssl, ret);

                                switch (sslErr) {
                                    case SSL_ERROR_WANT_WRITE:
                                        ReadEventReceiver::disable();
                                        WriteEventReceiver::enable();
                                        break;
                                    case SSL_ERROR_WANT_READ:
                                        break;
                                    case SSL_ERROR_NONE:
                                        ReadEventReceiver::disable();
                                        onSuccess();
                                        break;
                                    default:
                                        ReadEventReceiver::disable();
                                        onError(ERR_peek_error());
                                        break;
                                }
                            }

                            void writeEvent() override {
                                int ret = SSL_do_handshake(ssl);
                                int sslErr = SSL_get_error(ssl, ret);

                                switch (sslErr) {
                                    case SSL_ERROR_WANT_READ:
                                        WriteEventReceiver::disable();
                                        ReadEventReceiver::enable();
                                        break;
                                    case SSL_ERROR_WANT_WRITE:
                                        break;
                                    case SSL_ERROR_NONE:
                                        ReadEventReceiver::disable();
                                        onSuccess();
                                        break;
                                    default:
                                        WriteEventReceiver::disable();
                                        onError(ERR_peek_error());
                                        break;
                                }
                            }

                            void timeoutEvent() override {
                                onTimeout();
                            }

                            void unobserved() override {
                                delete this;
                            }

                            static void doHandshake(SSL* ssl,
                                                    int sslErr,
                                                    const std::function<void(void)>& onSuccess,
                                                    const std::function<void(void)>& onTimeout,
                                                    const std::function<void(unsigned long)>& onError) {
                                new TLSHandshaker(ssl, sslErr, onSuccess, onTimeout, onError);
                            }

                        private:
                            SSL* ssl;

                            const std::function<void(void)> onSuccess;
                            const std::function<void(void)> onTimeout;
                            const std::function<void(unsigned long)> onError;
                        };

                        TLSHandshaker::doHandshake(
                            ssl,
                            sslErr,
                            [this](void) -> void {
                                WriteEventReceiver::disable();
                            },
                            [this](void) -> void {
                                WriteEventReceiver::disable();
                            },
                            []([[maybe_unused]] unsigned long sslErr) -> void {
                                PLOG(INFO) << "TLS handshake failed: " << ERR_error_string(sslErr, nullptr);
                            });

                        errno = EAGAIN;
                        [[fallthrough]];
                    case SSL_ERROR_NONE:
                    case SSL_ERROR_ZERO_RETURN:
                        ret = 0;
                        break;
                    case SSL_ERROR_SYSCALL:
                        if (errno == 0) {
                            ret = 0;
                        } else {
                            ret = -1;
                        }
                        break;
                    default:
                        ret = -ERR_peek_error();
                        break;
                }
            }

            return ret;
        }

        SSL* ssl = nullptr;
    };

}; // namespace net::socket::stream::tls

#endif // NET_SOCKET_SOCK_STREAM_TLS_SOCKETWRITER_H
