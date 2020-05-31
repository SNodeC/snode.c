#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <errno.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#define MAX_JUNKSIZE 4096

#include "Multiplexer.h"
#include "SocketLegacyWriter.h"


namespace legacy {

void SocketLegacyWriter::writeEvent() {
    ssize_t ret = socketSend(writePuffer.c_str(), (writePuffer.size() < MAX_JUNKSIZE) ? writePuffer.size() : MAX_JUNKSIZE,
                             MSG_DONTWAIT | MSG_NOSIGNAL);

    if (ret >= 0) {
        writePuffer.erase(0, ret);
        if (writePuffer.empty()) {
            Multiplexer::instance().getManagedWriter().remove(this);
        }
    } else if (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR) {
        onError(errno);
        Multiplexer::instance().getManagedWriter().remove(this);
    }
}

};
