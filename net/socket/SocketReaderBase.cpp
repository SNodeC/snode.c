#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "SocketReaderBase.h"

#include "Multiplexer.h"
#include "socket/SocketConnection.h"


#define MAX_JUNKSIZE 16384

void SocketReaderBase::readEvent() {
    errno = 0;

    static char junk[MAX_JUNKSIZE];

    ssize_t ret = recv(junk, MAX_JUNKSIZE);

    if (ret > 0) {
        readProcessor(junk, ret);
    } else {
        if (ret == 0) {
            onError(0);
        } else {
            onError(errno);
        } // todo: do not disconnect on EAGIN EINTR
        Multiplexer::instance().getManagedReader().remove(this);
    }
}
