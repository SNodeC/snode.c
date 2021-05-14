/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021 Volker Christian <me@vchrist.at>
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

#ifndef NET_SOCKET_STREAM_TLS_SSL_UTILS_H
#define NET_SOCKET_STREAM_TLS_SSL_UTILS_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <any>
#include <map>
#include <openssl/ossl_typ.h> // for SSL_CTX
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::socket::stream::tls {

    SSL_CTX* ssl_ctx_new(const std::map<std::string, std::any>& options, bool server = false);
    void ssl_ctx_free(SSL_CTX* ctx);

    void ssl_set_sni(SSL* ssl, std::map<std::string, std::any>& options);

    void ssl_log_error(const std::string& message);
    void ssl_log_warning(const std::string& message);
    void ssl_log_info(const std::string& message);
    void ssl_log(const std::string& message, int sslErr);

} // namespace net::socket::stream::tls

#endif // NET_SOCKET_STREAM_TLS_SSL_UTILS_H
