#include "net/system/select.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cerrno>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::system {

    int select(int nfds, fd_set* readfds, fd_set* writefds, fd_set* exceptfds, struct timeval* timeout) {
        errno = 0;
        return ::select(nfds, readfds, writefds, exceptfds, timeout);
    }

} // namespace net::system
