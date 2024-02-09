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
#include "utils/PreserveErrno.h"

#include <cerrno>
#include <openssl/ssl.h> // IWYU pragma: keep
#include <string>

// IWYU pragma: no_include <openssl/ssl3.h>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace core::socket::stream::tls {

    ssize_t SocketReader::read(char* junk, std::size_t junkLen) {
        const int sslShutdownState = SSL_get_shutdown(ssl);

        int ret = SSL_read(ssl, junk, static_cast<int>(junkLen));

        if (ret <= 0) {
            const int ssl_err = SSL_get_error(ssl, ret);

            switch (ssl_err) {
                case SSL_ERROR_NONE:
                    break;
                case SSL_ERROR_WANT_READ:
                    ret = -1;
                    break;
                case SSL_ERROR_WANT_WRITE: {
                    const utils::PreserveErrno preserveErrno;

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
                }
                    ret = -1;
                    break;
                case SSL_ERROR_ZERO_RETURN: // received close_notify
                    LOG(TRACE) << getName() << " SSL/TLS: Zero return";
                    SSL_set_shutdown(ssl, SSL_get_shutdown(ssl) | sslShutdownState);
                    doSSLShutdown();
                    errno = 0;
                    ret = 0;
                    break;
                case SSL_ERROR_SYSCALL: {
                    const utils::PreserveErrno preserveErrno;

                    SSL_set_shutdown(ssl, SSL_get_shutdown(ssl) | SSL_RECEIVED_SHUTDOWN);
                    LOG(TRACE) << getName() << " SSL/TLS: TCP-FIN without close_notify. Emulating SSL_RECEIVED_SHUTDOWN";
                    doSSLShutdown();
                }
                    ret = -1;
                    break;
                default:
                    ssl_log(getName() + " SSL/TLS: Error read failed", ssl_err);
                    errno = EIO;
                    ret = -1;
                    break;
            }
        }

        return ret;
    }

} // namespace core::socket::stream::tls
