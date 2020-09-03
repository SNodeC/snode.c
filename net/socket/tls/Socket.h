/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020  Volker Christian <me@vchrist.at>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef TLS_SOCKET_H
#define TLS_SOCKET_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <openssl/ssl.h> // IWYU pragma: keep // for SSL, SSL_CTX

// IWYU pragma: no_include <openssl/ossl_typ.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "socket/Socket.h"

namespace net::socket::tls {

    class Socket : public socket::Socket {
    public:
        using socket::Socket::Socket;

        SSL* startSSL(SSL_CTX* ctx);
        void stopSSL();
        SSL* getSSL();

    protected:
        SSL* ssl = nullptr;
    };

}; // namespace net::socket::tls

#endif // TLS_SOCKET_H
