#include "net/system/time.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cerrno>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::system {

    time_t time(time_t* tloc) {
        errno = 0;
        return ::time(tloc);
    }

    int gettimeofday(struct timeval* tv, struct timezone* tz) {
        errno = 0;
        return ::gettimeofday(tv, tz);
    }

    struct tm* gmtime(const time_t* timep) {
        errno = 0;
        return ::gmtime(timep);
    }

    time_t mktime(struct tm* tm) {
        errno = 0;
        return ::mktime(tm);
    }

} // namespace net::system
