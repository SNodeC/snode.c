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

#include "core/socket/stream/tls/SocketReader.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "core/socket/stream/tls/ssl_utils.h"
#include "log/Logger.h"

#include <cerrno>
#include <limits>
#include <openssl/ssl.h>
#include <string>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace core::socket::stream::tls {

    ssize_t SocketReader::read(char* chunk, std::size_t chunkLen) {
        chunkLen = chunkLen > std::numeric_limits<int>::max() ? std::numeric_limits<int>::max() : chunkLen;
        int ret = SSL_read(ssl, chunk, static_cast<int>(chunkLen));

        if (ret <= 0) {
            const int ssl_err = SSL_get_error(ssl, ret);

            switch (ssl_err) {
                case SSL_ERROR_WANT_READ:
                    errno = EAGAIN;
                    ret = -1;
                    break;
                case SSL_ERROR_WANT_WRITE:
                    LOG(TRACE) << getName() << " SSL/TLS: Start renegotiation on read";
                    doSSLHandshake(
                        [this]() -> void {
                            LOG(TRACE) << getName() << " SSL/TLS: Renegotiation on read success";
                        },
                        [this]() -> void {
                            LOG(TRACE) << getName() << " SSL/TLS: Renegotiation on read timed out";
                        },
                        [this](int ssl_err) -> void {
                            ssl_log(getName() + " SSL/TLS: Renegotiation", ssl_err);
                        });
                    errno = EAGAIN;
                    ret = -1;
                    break;
                case SSL_ERROR_ZERO_RETURN: // received close_notify
                    LOG(TRACE) << getName() << " SSL/TLS: Close_notify received.";
                    errno = closeNotifyIsEOF ? 0 : EAGAIN;
                    ret = closeNotifyIsEOF ? 0 : -1;
                    break;
                case SSL_ERROR_SYSCALL: // When SSL_get_error(ssl, ret) returns SSL_ERROR_SYSCALL
                                        // and ret is 0, it indicates that the underlying TCP connection
                                        // was closed unexpectedly by the peer. This situation typically
                                        // happens when the peer closes (FIN) the connection without
                                        // sending a close_notify alert, which violates the SSL/TLS
                                        // protocol’s  graceful shutdown procedure.
                    // In case ret is -1 a real syscall error (RST = ECONNRESET)
                    if (ret == 0) {
                        LOG(TRACE) << getName() << " SSL/TLS: EOF detected: Connection closed by peer.";
                        errno = ECONNRESET;
                    } else if (errno == ECONNRESET) {
                        PLOG(TRACE) << getName() << " SSL/TLS: Connection reset by peer (ECONNRESET).";
                    } else {
                        PLOG(TRACE) << getName() + " SSL/TLS: Syscall error on read";
                    }
                    ret = -1;
                    break;
                case SSL_ERROR_SSL:
                    ssl_log(getName() + " SSL/TLS: Error read failed", ssl_err);
                    errno = EIO;
                    break;
                default:
                    LOG(TRACE) << getName() + " SSL/TLS: Unexpected error read failed (" << ssl_err << ")";
                    errno = EIO;
                    ret = -1;
                    break;
            }
        }

        return ret;
    }

} // namespace core::socket::stream::tls
