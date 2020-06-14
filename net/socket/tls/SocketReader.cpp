#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <sys/socket.h>
#include <sys/types.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "Multiplexer.h"
#include "socket/SocketConnection.h"
#include "socket/tls/SocketReader.h"


namespace tls {

    ssize_t SocketReader::recv(char* junk, const ssize_t& junkSize) {
        ssize_t ret = err;

        if (err > 0) {
            ret = ::SSL_read(ssl, junk, junkSize);
        }

        return ret;
    }

}; // namespace tls
