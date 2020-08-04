#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <sys/socket.h> // for send, MSG_DONTWAIT, MSG_NOSIGNAL

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "socket/legacy/SocketWriter.h"

namespace legacy {

    ssize_t SocketWriter::write(const char* junk, size_t junkSize) {
        return ::send(this->getFd(), junk, junkSize, MSG_NOSIGNAL);
    }

}; // namespace legacy
