#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "Multiplexer.h"
#include "socket/tls/SocketWriter.h"


namespace tls {

    ssize_t SocketWriter::send(const char* junk, const ssize_t& junkSize) {
        return tls::Socket::send(junk, junkSize, MSG_DONTWAIT | MSG_NOSIGNAL);
    }

}; // namespace tls
