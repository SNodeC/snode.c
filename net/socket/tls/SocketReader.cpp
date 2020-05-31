#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#define MAX_JUNKSIZE 16384

#include "Multiplexer.h"
#include "socket/SocketConnection.h"
#include "socket/tls/SocketReader.h"


namespace tls {

    void SocketReader::readEvent() {
        static char junk[MAX_JUNKSIZE];

        ssize_t ret = recv(junk, MAX_JUNKSIZE, 0);

        if (ret > 0) {
            readProcessor(dynamic_cast<::SocketConnection*>(this), junk, ret);
        } else if (ret == 0) {
            onError(0);
            Multiplexer::instance().getManagedReader().remove(this);
        } else {
            onError(errno);
            Multiplexer::instance().getManagedReader().remove(this);
        }
    }

}; // namespace tls
