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

#include "core/socket/stream/tls/system/ssl.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cerrno>
#include <openssl/ssl.h> // IWYU pragma: keep

// IWYU pragma: no_include <openssl/ssl3.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::ssl {

    int SSL_read(SSL* ssl, void* buf, int num) {
        errno = 0;
        return ::SSL_read(ssl, buf, num);
    }

    int SSL_write(SSL* ssl, const void* buf, int num) {
        errno = 0;
        return ::SSL_write(ssl, buf, num);
    }

} // namespace core::ssl
