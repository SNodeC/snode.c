#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <errno.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "SocketReader.h"


#define MAX_JUNKSIZE 16384

void SocketReader::readEvent() {
    errno = 0;

    static char junk[MAX_JUNKSIZE];

    ssize_t ret = recv(junk, MAX_JUNKSIZE);

    if (ret > 0) {
        onRead(junk, ret);
    } else {
        Reader::stop();
        this->onError(ret == 0 ? 0 : errno);
    }
}
