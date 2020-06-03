#include "SocketReaderBase.h"

#include "Multiplexer.h"
#include "socket/SocketConnection.h"


#define MAX_JUNKSIZE 16384

void SocketReaderBase::readEvent() {
    static char junk[MAX_JUNKSIZE];

    ssize_t ret = recv(junk, MAX_JUNKSIZE);

    if (ret > 0) {
        readProcessor(dynamic_cast<::SocketConnection*>(this), junk, ret);
    } else {
        if (ret == 0) {
            onError(0);
        } else {
            onError(errno);
        } // todo: do not disconnect on EAGIN EINTR
        Multiplexer::instance().getManagedReader().remove(this);
    }
}
