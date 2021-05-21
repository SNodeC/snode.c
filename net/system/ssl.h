#ifndef SSL_H
#define SSL_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

// IWYU pragma: begin_exports

#include <openssl/ossl_typ.h> // for SSL
#include <openssl/ssl.h>

// IWYU pragma: end_exports

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::ssl {

    // #include <openssl/ssl.h>
    int SSL_read(SSL* ssl, void* buf, int num);
    int SSL_write(SSL* ssl, const void* buf, int num);

} // namespace net::ssl

#endif // SSL_H
