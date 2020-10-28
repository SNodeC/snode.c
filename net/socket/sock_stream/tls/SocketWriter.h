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
#include "TLSHandshake.h"
#include "socket/sock_stream/SocketWriter.h"
#include "ssl_utils.h"

namespace net::socket::stream::tls {

    template <typename SocketT>
    class SocketWriter : public stream::SocketWriter<SocketT> {
        using stream::SocketWriter<SocketT>::SocketWriter;

    private:
        ssize_t write(const char* junk, size_t junkLen) override {
            int ret = ::SSL_write(ssl, junk, junkLen);

            if (ret <= 0) {
                sslErr = SSL_get_error(ssl, ret);

                switch (sslErr) {
                    case SSL_ERROR_WANT_WRITE:
                    case SSL_ERROR_WANT_READ:
                        WriteEventReceiver::suspend();
                        TLSHandshake::doHandshake(
                            ssl,
                            [this](void) -> void {
                                WriteEventReceiver::resume();
                            },
                            [this](void) -> void {
                                WriteEventReceiver::disable();
                                PLOG(ERROR) << "TLS handshake timeout";
                            },
                            [this](int sslErr) -> void {
                                WriteEventReceiver::disable();
                                ssl_log("SSL/TLS handshake failed", sslErr);
                            });
                        errno = EAGAIN;
                        break;
                    case SSL_ERROR_ZERO_RETURN:
                        ret = 0;
                        break;
                    case SSL_ERROR_SYSCALL:
                        ssl_log("SSL/TLS write maybe failed", sslErr);
                        break;
                    default:
                        ssl_log("SSL/TLS write failed", sslErr);
                        break;
                }
            }

            return ret;
        }

        int getError() override {
            int ret = errno;

            switch (sslErr) {
                case SSL_ERROR_NONE:
                case SSL_ERROR_ZERO_RETURN:
                    ret = 0;
                    break;
                case SSL_ERROR_SYSCALL:
                    break;
                default:
                    ret = -sslErr;
                    break;
            };

            sslErr = 0;

            return ret;
        }

    protected:
        SSL* ssl = nullptr;

        int sslErr = SSL_ERROR_NONE;
    };

}; // namespace net::socket::stream::tls

#endif // NET_SOCKET_SOCK_STREAM_TLS_SOCKETWRITER_H
