#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#define MAX_JUNKSIZE 16384

#include "Multiplexer.h"
#include "SocketConnectionInterface.h"
#include "SocketSSLReader.h"


void SocketSSLReader::readEvent() {
    static char junk[MAX_JUNKSIZE];

    ssize_t ret = socketRecv(junk, MAX_JUNKSIZE, 0);

    if (ret > 0) {
        readProcessor(dynamic_cast<SocketConnectionInterface*>(this), junk, ret);
    } else if (ret == 0) {
        onError(0);
        Multiplexer::instance().getReadManager().unmanageSocket(this);
    } else {
        onError(errno);
        Multiplexer::instance().getReadManager().unmanageSocket(this);
    }
}

