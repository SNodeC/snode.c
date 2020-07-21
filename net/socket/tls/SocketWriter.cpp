#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <openssl/ssl.h> // for SSL_accept, SSL_free, SSL_get_error, SSL_new

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "socket/tls/SocketWriter.h"


namespace tls {

    ssize_t SocketWriter::send(const char* junk, const size_t junkSize) {
        ssize_t ret = err;

        if (err > 0) {
            ret = ::SSL_write(ssl, junk, junkSize);
        }

        return ret;
    }

}; // namespace tls
