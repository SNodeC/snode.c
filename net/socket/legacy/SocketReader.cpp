#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <sys/socket.h> // for recv

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "socket/legacy/SocketReader.h"


namespace legacy {

    ssize_t SocketReader::recv(char* junk, size_t junkSize) {
        return ::recv(this->getFd(), junk, junkSize, 0);
        ;
    }

}; // namespace legacy
