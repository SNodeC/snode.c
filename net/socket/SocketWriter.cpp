#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <errno.h> // for errno, EAGAIN, EINTR, EWOULDBLOCK

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "SocketWriter.h"


#define MAX_JUNKSIZE 4096

void SocketWriter::writeEvent() {
    errno = 0;

    ssize_t ret = send(writePuffer.c_str(), (writePuffer.size() < MAX_JUNKSIZE) ? writePuffer.size() : MAX_JUNKSIZE);

    if (ret >= 0) {
        writePuffer.erase(0, ret);
        if (writePuffer.empty()) {
            Writer::stop();
        }
    } else if (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR) {
        Writer::stop();
        onError(errno);
    }
}


void SocketWriter::enqueue(const char* buffer, int size) {
    writePuffer.append(buffer, size);
    Writer::start();
}
