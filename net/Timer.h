#ifndef TIMER_H
#define TIMER_H

#include <functional>

#include <time.h>
#include <sys/time.h>

class SingleshotTimer;
class ContinousTimer;

class Timer
{
public:
    Timer(std::function<void (void* arg)> processor, const struct timeval& timeout, void* arg);
    virtual ~Timer() {}
    
    struct timeval& absolutTimeout();
    
    operator struct timeval() const;
    
    void dispatch();
    void update();
    static void cancel(Timer* timer);
    
    static SingleshotTimer& singleshotTimer(std::function<void (void* arg)> processor, const struct timeval& timeout, void* arg);
    static ContinousTimer& continousTimer(std::function<void (void* arg)> processor, const struct timeval& timeout, void* arg);
    
    
protected:
    struct timeval absoluteTimeout;
    struct timeval delay;
    void* arg;
    std::function<void (void* arg)> processor;
};

bool operator<(const struct timeval& tv1, const struct timeval& tv2);
bool operator<=(const struct timeval& tv1, const struct timeval& tv2);

struct timeval operator+(const struct timeval& tv1, const struct timeval& tv2);
struct timeval operator-(const struct timeval& tv1, const struct timeval& tv2);

#endif // TIMER_H
