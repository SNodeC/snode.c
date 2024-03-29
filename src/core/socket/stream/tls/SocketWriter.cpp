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

#include "core/socket/stream/tls/SocketWriter.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "core/socket/stream/tls/ssl_utils.h"
#include "log/Logger.h"
#include "utils/PreserveErrno.h"

#include <cerrno>
#include <openssl/ssl.h> // IWYU pragma: keep
#include <string>

// IWYU pragma: no_include <openssl/ssl3.h>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace core::socket::stream::tls {

    ssize_t SocketWriter::write(const char* chunk, std::size_t chunkLen) {
        int ret = SSL_write(ssl, chunk, static_cast<int>(chunkLen));

        if (ret <= 0) {
            const int ssl_err = SSL_get_error(ssl, ret);

            switch (ssl_err) {
                case SSL_ERROR_NONE:
                    break;
                case SSL_ERROR_WANT_READ: {
                    const utils::PreserveErrno preserveErrno;

                    LOG(TRACE) << getName() << " SSL/TLS: Start renegotiation on write";
                    doSSLHandshake(
                        [instanceName = getName()]() -> void {
                            LOG(TRACE) << instanceName << " SSL/TLS: Renegotiation on write success";
                        },
                        [instanceName = getName()]() -> void {
                            LOG(TRACE) << instanceName << " SSL/TLS: Renegotiation on write timed out";
                        },
                        [instanceName = getName()](int ssl_err) -> void {
                            ssl_log(instanceName + " SSL/TLS: Renegotiation", ssl_err);
                        });
                }
                    ret = -1;
                    break;
                case SSL_ERROR_WANT_WRITE:
                    ret = -1;
                    break;
                case SSL_ERROR_ZERO_RETURN: // shutdown cleanly
                    errno = EPIPE;
                    ret = -1; // on the write side this means a TCP broken pipe
                    break;
                case SSL_ERROR_SYSCALL:
                    ret = -1;
                    break;
                default:
                    ssl_log(getName() + " SSL/TLS: Write failed", ssl_err);
                    errno = EIO;
                    ret = -1;
                    break;
            }
        }

        return ret;
    }

} // namespace core::socket::stream::tls
