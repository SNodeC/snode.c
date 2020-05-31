#include "SocketWriterBase.h"

#include "Multiplexer.h"
#include "socket/SocketConnection.h"

#define MAX_JUNKSIZE 4096

void SocketWriterBase::writeEvent() {
    ssize_t ret = send(writePuffer.c_str(), (writePuffer.size() < MAX_JUNKSIZE) ? writePuffer.size() : MAX_JUNKSIZE);

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
