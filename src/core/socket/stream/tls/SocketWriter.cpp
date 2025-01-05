/*
 * SNode.C - a slim toolkit for network communication
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

#include "core/socket/stream/tls/SocketWriter.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "core/socket/stream/tls/ssl_utils.h"
#include "log/Logger.h"
#include "utils/PreserveErrno.h"

#include <cerrno>
#include <openssl/ssl.h>
#include <string>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace core::socket::stream::tls {

    ssize_t SocketWriter::write(const char* chunk, std::size_t chunkLen) {
        ssize_t ret = 0;

        if ((SSL_get_shutdown(ssl) & SSL_SENT_SHUTDOWN) != 0) {
            ret = Super::write(chunk, chunkLen);
        } else {
            ret = SSL_write(ssl, chunk, static_cast<int>(chunkLen));

            if (ret <= 0) {
                const int ssl_err = SSL_get_error(ssl, static_cast<int>(ret));

                switch (ssl_err) {
                    case SSL_ERROR_WANT_READ:
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
                    case SSL_ERROR_WANT_WRITE:
                        errno = EAGAIN;
                        ret = -1;
                        break;
                    case SSL_ERROR_ZERO_RETURN: // shutdown cleanly
                        LOG(TRACE) << getName() << " SSL/TLS: Close_notify received. Is EOF? " << (closeNotifyIsEOF ? "true" : "false");
                        errno = closeNotifyIsEOF ? EPIPE : EAGAIN;
                        ret = -1; // on the write side this means a TCP broken pipe
                        break;
                    case SSL_ERROR_SYSCALL:
                        // In case ret is -1 a real syscall error (RST = ECONNRESET)
                        {
                            utils::PreserveErrno pe;

                            if (errno == EPIPE) {
                                PLOG(TRACE) << getName() << " SSL/TLS: Syscal error (SIGPIPE detected) on write.";
                            } else if (errno == ECONNRESET) {
                                PLOG(TRACE) << getName() << " SSL/TLS: Connection reset by peer (ECONNRESET).";
                            } else {
                                PLOG(TRACE) << getName() << " SSL/TLS: Syscall error on write";
                            }
                        }
                        ret = -1;
                        break;
                    case SSL_ERROR_SSL:
                        ssl_log(getName() + " SSL/TLS: Failed", ssl_err);
                        ret = -1;
                        break;
                    default:
                        ssl_log(getName() + " SSL/TLS: Unexpected error", ssl_err);
                        errno = EIO;
                        ret = -1;
                        break;
                }
            }
        }

        return ret;
    }

} // namespace core::socket::stream::tls
