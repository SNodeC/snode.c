#ifndef TIMER_H
#define TIMER_H

#include <sys/time.h>
#include <time.h>
#include <functional>

class SingleShotTimer;
class ContinousTimer;

class Timer
{
private:
    struct timeval absoluteTimeoutValue;
    struct timeval delay;
    
    void* argument;
    std::function<void (void* arguments)> processor;
    
public:
    
    Timer(std::function<void (void* arguments)> processor, const struct timeval& timeout, void* arguments);
    virtual ~Timer() { }
    
    struct timeval& absoluteTimeout();
    
    void dispatch();
    void update();
    void cancel();
    
    static SingleShotTimer& singleshotTimer(std::function<void (void* arguments)> processor, const struct timeval& timeout, void* arguments);
    static ContinousTimer& continousTimer(std::function<void (void* arguments)> processor, const struct timeval& timeout, void* arguments);
    
    operator struct timeval() const;
};

bool operator < (const struct timeval& timeValue1, const struct timeval& timeValue2);
bool operator <= (const struct timeval& timeValue1, const struct timeval& timeValue2);
bool operator == (const struct timeval& timeValue1, const struct timeval& timeValue2);
struct timeval operator - (const timeval& timeValue1, const struct timeval& timeValue2);


#endif // TIMER_H

