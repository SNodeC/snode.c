#include "net/system/signal.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cerrno>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::system {

    sighandler_t signal(int signum, sighandler_t handler) {
        errno = 0;
        return ::signal(signum, handler);
    }

} // namespace net::system
