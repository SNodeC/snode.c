#ifndef SIGNAL_H
#define SIGNAL_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

// IWYU pragma: begin_exports

#include <csignal>

// IWYU pragma: end_exports

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::system {

    // #include <csignal>
    sighandler_t signal(int signum, sighandler_t handler);

} // namespace net::system

#endif // SIGNAL_H
