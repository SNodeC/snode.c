#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "SocketReader.h"

#include "Multiplexer.h"


#define MAX_JUNKSIZE 16384

void SocketReader::readEvent() {
    errno = 0;

    static char junk[MAX_JUNKSIZE];

    ssize_t ret = recv(junk, MAX_JUNKSIZE);

    if (ret > 0) {
        onRead(junk, ret);
    } else {
        if (ret == 0) {
            onError(0);
        } else {
            onError(errno);
        } // todo: do not disconnect on EAGIN EINTR
        Multiplexer::instance().getManagedReader().remove(this);
    }
}
