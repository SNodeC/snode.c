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

#ifndef CORE_SOCKET_STREAM_TLS_SSL_UTILS_H
#define CORE_SOCKET_STREAM_TLS_SSL_UTILS_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstdint>
#include <map>
#include <string>

//

#include <openssl/opensslv.h>

#if OPENSSL_VERSION_NUMBER >= 0x30000000L
#include <openssl/types.h>
#elif OPENSSL_VERSION_NUMBER >= 0x10100000L
#include <openssl/ossl_typ.h>

#endif

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#if OPENSSL_VERSION_NUMBER >= 0x30000000L
using ssl_option_t = uint64_t;

#elif OPENSSL_VERSION_NUMBER >= 0x10100000L
using ssl_option_t = uint32_t;
#endif

namespace core::socket::stream::tls {

    struct SslConfig {
        explicit SslConfig(bool server);

        std::string instanceName;
        std::string cert;
        std::string certKey;
        std::string password;
        std::string caCert;
        std::string caCertDir;
        bool caCertUseDefaultDir = false;
        std::string cipherList;
        ssl_option_t sslOptions = 0;
        bool server = false;
    };

    SSL_CTX* ssl_ctx_new(const SslConfig& sslConfig);
    std::map<std::string, SSL_CTX*> ssl_get_sans(SSL_CTX* sslCtx);

    void ssl_set_sni(SSL* ssl, const std::string& sni);
    SSL_CTX* ssl_set_ssl_ctx(SSL* ssl, SSL_CTX* sslCtx);

    void ssl_ctx_free(SSL_CTX* ctx);

    std::string ssl_get_servername_from_client_hello(SSL* ssl);

    void ssl_log_error(const std::string& message);
    void ssl_log_warning(const std::string& message);
    void ssl_log_info(const std::string& message);
    void ssl_log(const std::string& message, int sslErr);

    // From https://www.geeksforgeeks.org/wildcard-character-matching/
    //
    // The main function that checks if two given strings
    // match. The first string may contain wildcard characters
    bool match(const char* first, const char* second);

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
