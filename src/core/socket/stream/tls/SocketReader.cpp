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
                case SSL_ERROR_NONE:
                    errno = EAGAIN;
                    break;
                case SSL_ERROR_WANT_READ:
                    errno = EAGAIN;
                    ret = -1;
                    break;
                case SSL_ERROR_WANT_WRITE:
                    errno = EAGAIN;
                    ret = -1;
                    break;
                case SSL_ERROR_ZERO_RETURN: // received close_notify
                    errno = 0;
                    ret = 0;
                    break;
                case SSL_ERROR_SYSCALL:
                    if (ret == 0) { // Do not check errno as it is not set in that case.
                        std::cerr << "EOF detected: Connection closed by peer." << std::endl;
                        errno = 0;
                    } else {
                        std::cerr << "SSL_read syscall error: " << strerror(errno) << std::endl;
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
