#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <openssl/ssl.h> // IWYU pragma: keep for SSL_accept, SSL_free, SSL_get_error, SSL_new

// IWYU pragma: no_include <openssl/ssl3.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "socket/tls/SocketWriter.h"

namespace tls {

    ssize_t SocketWriter::send(const char* junk, size_t junkSize) {
        return ::SSL_write(ssl, junk, junkSize);
    }

}; // namespace tls
