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

#ifndef NET_SYSTEM_SSL_H
#define NET_SYSTEM_SSL_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <openssl/opensslv.h> // IWYU pragma: export
#include <openssl/ssl.h>      // IWYU pragma: export

#if OPENSSL_VERSION_NUMBER >= 0x30000000L
#include <openssl/types.h>
#elif OPENSSL_VERSION_NUMBER >= 0x10100000L
#include <openssl/ossl_typ.h>
#endif

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::ssl {

    // #include <openssl/ssl.h>
    int SSL_read(SSL* ssl, void* buf, int num);
    int SSL_write(SSL* ssl, const void* buf, int num);

} // namespace core::ssl

#endif // NET_SYSTEM_SSL_H
