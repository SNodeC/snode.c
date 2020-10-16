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

#ifndef NET_SOCKET_SOCK_STREAM_TLS_SOCKETREADER_H
#define NET_SOCKET_SOCK_STREAM_TLS_SOCKETREADER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef> // for size_t
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <sys/types.h> // for ssize_t

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "Descriptor.h"
#include "Logger.h"
#include "WriteEventReceiver.h"
#include "socket/sock_stream/SocketReader.h"

// IWYU pragma: no_forward_declare tls::Socket

namespace net::socket::stream::tls {

    template <typename SocketT>
    class SocketReader : public socket::stream::SocketReader<SocketT> {
    public:
        using socket::stream::SocketReader<SocketT>::SocketReader;

    protected:
        ssize_t read(char* junk, size_t junkLen) override {
            int ret = ::SSL_read(ssl, junk, junkLen);

            if (ret <= 0) {
                int sslErr = SSL_get_error(ssl, ret);

                switch (sslErr) {
                    case SSL_ERROR_WANT_WRITE:
                    case SSL_ERROR_WANT_READ:
                        class Renegotiator
                            : public ReadEventReceiver
                            , public WriteEventReceiver
                            , public Descriptor {
                        public:
                            Renegotiator(SSL* ssl, int sslErr)
                                : ssl(ssl) {
                                this->open(SSL_get_fd(ssl), FLAGS::dontClose);
                                switch (sslErr) {
                                    case SSL_ERROR_WANT_READ:
                                        ReadEventReceiver::enable();
                                        break;
                                    case SSL_ERROR_WANT_WRITE:
                                        WriteEventReceiver::enable();
                                        break;
                                    default:
                                        delete this;
                                }
                            }

                            void readEvent() override {
                                int ret = SSL_read(ssl, nullptr, 0);
                                int sslErr = SSL_get_error(ssl, ret);

                                switch (sslErr) {
                                    case SSL_ERROR_WANT_WRITE:
                                        ReadEventReceiver::disable();
                                        WriteEventReceiver::enable();
                                        break;
                                    case SSL_ERROR_WANT_READ:
                                        break;
                                    default:
                                        ReadEventReceiver::disable();
                                        break;
                                }
                            }

                            void writeEvent() override {
                                int ret = SSL_read(ssl, nullptr, 0);
                                int sslErr = SSL_get_error(ssl, ret);

                                switch (sslErr) {
                                    case SSL_ERROR_WANT_READ:
                                        WriteEventReceiver::disable();
                                        ReadEventReceiver::enable();
                                        break;
                                    case SSL_ERROR_WANT_WRITE:
                                        break;
                                    default:
                                        WriteEventReceiver::disable();
                                        break;
                                }
                            }

                            void unobserved() override {
                                delete this;
                            }

                        private:
                            SSL* ssl;
                        };

                        new Renegotiator(ssl, sslErr);

                        errno = EAGAIN;
                        [[fallthrough]];
                    case SSL_ERROR_NONE:
                    case SSL_ERROR_ZERO_RETURN:
                        ret = 0;
                        break;
                    case SSL_ERROR_SYSCALL:
                        ret = -1;
                        break;
                    default:
                        ret = -ERR_peek_error();
                        break;
                }
            }

            return ret;
        }

    protected:
        SSL* ssl = nullptr;
    };
}; // namespace net::socket::stream::tls

#endif // NET_SOCKET_SOCK_STREAM_TLS_SOCKETREADER_H
