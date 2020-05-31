#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <errno.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "Multiplexer.h"
#include "socket/legacy/SocketWriter.h"


namespace legacy {

    ssize_t SocketWriter::send(const char* junk, const ssize_t& junkSize) {
        return send(junk, junkSize, MSG_DONTWAIT | MSG_NOSIGNAL);
    }

}; // namespace legacy
