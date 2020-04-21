#ifndef TIMER_H
#define TIMER_H

#include <time.h>
#include <sys/time.h>

class Timer
{
public:
    struct timeval& absolutTimeout();
    
    operator struct timeval() const;
    
protected:
    struct timeval absoluteTimeout;
};

bool operator<(const struct timeval& tv1, const struct timeval& tv2);

struct timeval operator-(const struct timeval& tv1, const struct timeval& tv2);

#endif // TIMER_H
