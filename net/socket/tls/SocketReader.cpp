#ifndef DOXYGEN_SHOULD_SKIP_THIS

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
