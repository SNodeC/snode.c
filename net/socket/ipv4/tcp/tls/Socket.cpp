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

#include "Socket.h"

namespace net::socket::ipv4::tcp::tls {

    void Socket::setSSL_CTX(SSL_CTX* ctx) {
        if (ctx != nullptr) {
            this->ctx = ctx;
            SSL_CTX_up_ref(ctx);
        }
    }

    void Socket::clearSSL_CTX() {
        if (ctx != nullptr) {
            SSL_CTX_free(ctx);
            ctx = nullptr;
        }
    }

    SSL* Socket::startSSL() {
        int ret = 0;

        if (ctx != nullptr) {
            ssl = SSL_new(ctx);

            if (ssl != nullptr) {
                ret = SSL_set_fd(ssl, getFd());
            }
        }

        return ret == 1 ? ssl : nullptr;
    }

    void Socket::stopSSL() {
        if (!dontClose()) {
            if (ssl != nullptr) {
                SSL_shutdown(ssl);
                SSL_free(ssl);
                ssl = nullptr;
            }
        }
    }

    SSL* Socket::getSSL() const {
        return ssl;
    }

}; // namespace net::socket::ipv4::tcp::tls
