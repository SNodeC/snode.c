#include "Timer.h"

struct timeval& Timer::absolutTimeout() {
    return absoluteTimeout;
}


Timer::operator struct timeval() const {
    return absoluteTimeout;
}


bool operator<(const struct timeval& tv1, const struct timeval& tv2) {
    return (tv1.tv_sec < tv2.tv_sec) || ((tv1.tv_sec == tv2.tv_sec) && (tv1.tv_usec < tv2.tv_usec));
}

struct timeval operator-(const struct timeval& tv1, const struct timeval& tv2) {
    struct timeval help;
    
    help.tv_sec = tv1.tv_sec - tv2.tv_sec;
    help.tv_usec = tv1.tv_usec - tv2.tv_usec;
    
    if (help.tv_usec < 0) {
        help.tv_usec += 1000000;
        help.tv_sec--;
    }
    
    return help;
}
