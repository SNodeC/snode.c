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

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <openssl/ssl.h> // IWYU pragma: keep // for SSL_accept, SSL_free, SSL_get_error, SSL_new

// IWYU pragma: no_include <openssl/ssl3.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "socket/tcp/tls/Socket.h"

namespace net::socket::tcp::tls {

    Socket::~Socket() {
        if (!dontClose()) {
            if (ssl != nullptr) {
                SSL_shutdown(ssl);
                SSL_free(ssl);
                ssl = nullptr;
            }
        }

        if (ctx != nullptr) {
            SSL_CTX_free(ctx);
        }
    }

    void Socket::setCTX(SSL_CTX* ctx) {
        if (ctx != nullptr) {
            this->ctx = ctx;
            SSL_CTX_up_ref(ctx);
        }
    }

    void Socket::startSSL() {
        if (ctx != nullptr) {
            ssl = SSL_new(ctx);

            if (ssl != nullptr) {
                SSL_set_fd(ssl, getFd());
            }
        }
    }

    SSL* Socket::getSSL() const {
        return ssl;
    }

}; // namespace net::socket::tcp::tls
