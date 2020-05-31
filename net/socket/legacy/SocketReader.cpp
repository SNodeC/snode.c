#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "Multiplexer.h"
#include "socket/SocketConnection.h"
#include "socket/legacy/SocketReader.h"


namespace legacy {

    ssize_t SocketReader::recv(char* junk, const ssize_t& junkSize) {
        return legacy::Socket::recv(junk, junkSize, 0);
    }

}; // namespace legacy
