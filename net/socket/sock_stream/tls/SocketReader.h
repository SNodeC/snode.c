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
#include "TLSHandshake.h"
#include "WriteEventReceiver.h"
#include "socket/sock_stream/SocketReader.h"
#include "ssl_utils.h"

namespace net::socket::stream::tls {

    template <typename SocketT>
    class SocketReader : public stream::SocketReader<SocketT> {
        using stream::SocketReader<SocketT>::SocketReader;

    private:
        ssize_t read(char* junk, size_t junkLen) override {
            int ret = ::SSL_read(ssl, junk, junkLen);

            if (ret <= 0) {
                sslErr = SSL_get_error(ssl, ret);

                switch (sslErr) {
                    case SSL_ERROR_WANT_WRITE:
                    case SSL_ERROR_WANT_READ:
                        TLSHandshake::doHandshake(
                            ssl,
                            [](void) -> void {
                            },
                            [this](void) -> void {
                                ReadEventReceiver::disable();
                                PLOG(ERROR) << "TLS handshake timeout";
                            },
                            [this]([[maybe_unused]] int sslErr) -> void {
                                ReadEventReceiver::disable();
                                ssl_log_error("SSL/TLS handshake failed");
                            });
                        errno = EAGAIN;
                        break;
                    case SSL_ERROR_ZERO_RETURN:
                        ret = 0;
                        break;
                    case SSL_ERROR_SYSCALL:
                        ssl_log_warning("SSL/TLS read maybe failed");
                        break;
                    default:
                        ssl_log_error("SSL/TLS read failed");
                        break;
                }
            }

            return ret;
        }

        int getError() override {
            int ret = errno;

            if (sslErr != SSL_ERROR_SYSCALL) {
                ret = -sslErr;
            }
            sslErr = 0;

            return ret;
        }

    protected:
        SSL* ssl = nullptr;

        int sslErr = SSL_ERROR_NONE;
    };
}; // namespace net::socket::stream::tls

#endif // NET_SOCKET_SOCK_STREAM_TLS_SOCKETREADER_H
