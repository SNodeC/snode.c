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

#ifndef CORE_SOCKET_STREAM_TLS_SOCKETREADER_H
#define CORE_SOCKET_STREAM_TLS_SOCKETREADER_H

#include "core/socket/stream/SocketReader.h"
#include "core/socket/stream/tls/TLSHandshake.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "core/socket/stream/tls/ssl_utils.h"
#include "log/Logger.h"

#include <openssl/err.h>
#include <openssl/ssl.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::socket::stream::tls {

    template <typename SocketT>
    class SocketReader : public core::socket::stream::SocketReader<SocketT> {
    private:
        using Super = core::socket::stream::SocketReader<SocketT>;
        using Super::Super;

        ssize_t read(char* junk, std::size_t junkLen) override {
            int ret = SSL_read(ssl, junk, static_cast<int>(junkLen));

            if (ret <= 0) {
                int ssl_err = SSL_get_error(ssl, ret);

                switch (ssl_err) {
                    case SSL_ERROR_NONE:
                        break;
                    case SSL_ERROR_WANT_READ:
                        ret = -1;
                        break;
                    case SSL_ERROR_WANT_WRITE:
                        LOG(INFO) << "SSL/TLS start renegotiation on read";
                        {
                            int tmpErrno = errno;
                            doSSLHandshake(
                                [](void) -> void {
                                    LOG(INFO) << "SSL/TLS renegotiation on read success";
                                },
                                [](void) -> void {
                                    LOG(WARNING) << "SSL/TLS renegotiation on read timed out";
                                },
                                [](int ssl_err) -> void {
                                    ssl_log("SSL/TLS renegotiation", ssl_err);
                                });
                            errno = tmpErrno;
                        }
                        ret = -1;
                        break;
                    case SSL_ERROR_ZERO_RETURN: // received close_notify
                        doReadShutdown();
                        errno = 0;
                        ret = 0;
                        break;
                    case SSL_ERROR_SYSCALL:
                        VLOG(0) << "SSL/TLS: TCP-FIN without close_notify. Emulating SSL_RECEIVED_SHUTDOWN";
                        SSL_set_shutdown(ssl, SSL_get_shutdown(ssl) | SSL_RECEIVED_SHUTDOWN);
                        {
                            int tmpErrno = errno;
                            doReadShutdown();
                            errno = tmpErrno;
                        }
                        ret = -1;
                        break;
                    default:
                        ssl_log("SSL/TLS error read failed", ssl_err);
                        errno = EIO;
                        ret = -1;
                        break;
                }
            }

            return ret;
        }

    protected:
        virtual void doReadShutdown() = 0;

        virtual void doSSLHandshake(const std::function<void()>& onSuccess,
                                    const std::function<void()>& onTimeout,
                                    const std::function<void(int)>& onError) = 0;

        SSL* ssl = nullptr;
    };

} // namespace core::socket::stream::tls

#endif // CORE_SOCKET_STREAM_TLS_SOCKETREADER_H
