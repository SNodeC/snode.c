#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "SocketWriter.h"
#include "Multiplexer.h"


void SocketWriter::writeEvent() {
    ssize_t ret = ::send(this->getFd(), writePuffer.c_str(), (writePuffer.size() < 4096) ? writePuffer.size() : 4096, MSG_DONTWAIT | MSG_NOSIGNAL);
    
    if (ret >= 0) {
        writePuffer.erase(0, ret);
        if (writePuffer.empty()) {
            Multiplexer::instance().getWriteManager().unmanageSocket(this);
        }
    } else if (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR) {
        onError(errno);
        Multiplexer::instance().getWriteManager().unmanageSocket(this);
    }
}
