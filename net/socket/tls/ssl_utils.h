#ifndef SSL_UTILS_H
#define SSL_UTILS_H

#include <any>
#include <map>
#include <openssl/ossl_typ.h> // for SSL_CTX
#include <string>

namespace net::socket::tls {
    unsigned long ssl_init_ctx(SSL_CTX* ctx, const std::map<std::string, std::any>& options, bool server = false);
}

#endif // SSL_UTILS_H
