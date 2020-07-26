#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <openssl/ssl.h> // for SSL_accept, SSL_free, SSL_get_error, SSL_new

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "socket/tls/SocketReader.h"


namespace tls {

    ssize_t SocketReader::recv(char* junk, size_t junkSize) {
        return ::SSL_read(ssl, junk, junkSize);
    }

}; // namespace tls
