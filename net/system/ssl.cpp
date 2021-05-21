#include "net/system/ssl.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cerrno>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::ssl {

    int SSL_read(SSL* ssl, void* buf, int num) {
        errno = 0;
        return ::SSL_read(ssl, buf, num);
    }
    int SSL_write(SSL* ssl, const void* buf, int num) {
        errno = 0;
        return ::SSL_write(ssl, buf, num);
    }

} // namespace net::ssl
