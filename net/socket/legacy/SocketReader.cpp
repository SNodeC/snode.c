#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "socket/legacy/SocketReader.h"

#include "Multiplexer.h"
#include "socket/SocketConnection.h"


namespace legacy {

    ssize_t SocketReader::recv(char* junk, const ssize_t& junkSize) {
        return ::recv(this->getFd(), junk, junkSize, 0);
        ;
    }

}; // namespace legacy
