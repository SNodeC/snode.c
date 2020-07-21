#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <openssl/ssl.h> // for SSL_accept, SSL_free, SSL_get_error, SSL_new

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "socket/tls/SocketReader.h"


namespace tls {

    ssize_t SocketReader::recv(char* junk, size_t junkSize) {
        ssize_t ret = err;

        if (err > 0) {
            ret = ::SSL_read(ssl, junk, junkSize);
        }

        return ret;
    }

}; // namespace tls
