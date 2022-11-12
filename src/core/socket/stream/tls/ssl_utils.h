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

#ifndef CORE_SOCKET_STREAM_TLS_SSL_UTILS_H
#define CORE_SOCKET_STREAM_TLS_SSL_UTILS_H

namespace net::config {
    class ConfigTls;
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <any>
#include <map>
#include <memory>
#include <openssl/opensslv.h>
#include <set>
#include <string>

#if OPENSSL_VERSION_NUMBER >= 0x30000000L
#include <openssl/types.h>
#elif OPENSSL_VERSION_NUMBER >= 0x10100000L
#include <openssl/ossl_typ.h>
#endif

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::socket::stream::tls {

    SSL_CTX* ssl_ctx_new(const std::shared_ptr<net::config::ConfigTls>& configTls, bool server = false);
    SSL_CTX* ssl_ctx_new(const std::map<std::string, std::any>& options, bool server = false);
    void ssl_ctx_free(SSL_CTX* ctx);

    void ssl_set_sni(SSL* ssl, std::map<std::string, std::any>& options);
    void ssl_set_sni(SSL* ssl, const std::shared_ptr<net::config::ConfigTls>& configTls);
    std::set<std::string> ssl_get_sans(SSL_CTX* sslCtx);
    std::string ssl_get_servername_from_client_hello(SSL* ssl);
    SSL_CTX* ssl_set_ssl_ctx(SSL* ssl, SSL_CTX* sslCtx);

    void ssl_log_error(const std::string& message);
    void ssl_log_warning(const std::string& message);
    void ssl_log_info(const std::string& message);
    void ssl_log(const std::string& message, int sslErr);

} // namespace core::socket::stream::tls

#endif // CORE_SOCKET_STREAM_TLS_SSL_UTILS_H

/*
ssl.h:# define SSL_ERROR_NONE                  0
ssl.h:# define SSL_ERROR_SSL                   1
ssl.h:# define SSL_ERROR_WANT_READ             2
ssl.h:# define SSL_ERROR_WANT_WRITE            3
ssl.h:# define SSL_ERROR_WANT_X509_LOOKUP      4
ssl.h:# define SSL_ERROR_SYSCALL               5 // look at error stack/return
ssl.h:# define SSL_ERROR_ZERO_RETURN           6
ssl.h:# define SSL_ERROR_WANT_CONNECT          7
ssl.h:# define SSL_ERROR_WANT_ACCEPT           8
ssl.h:# define SSL_ERROR_WANT_ASYNC            9
ssl.h:# define SSL_ERROR_WANT_ASYNC_JOB       10
ssl.h:# define SSL_ERROR_WANT_CLIENT_HELLO_CB 11
*/
