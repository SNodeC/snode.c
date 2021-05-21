#ifndef TIME_H
#define TIME_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

// IWYU pragma: begin_exports

#include <ctime>
#include <sys/time.h>

// IWYU pragma: end_exports

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::system {

    // #include <time.h> = <ctime>
    time_t time(time_t* tloc);
    struct tm* gmtime(const time_t* timep);
    time_t mktime(struct tm* tm);

    // #include <sys/time.h>
    int gettimeofday(struct timeval* tv, struct timezone* tz);

} // namespace net::system

#endif // TIME_H
