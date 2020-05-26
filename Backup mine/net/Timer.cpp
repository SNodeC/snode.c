#include "Timer.h"
#include "SocketMultiplexer.h"
#include "SingleShotTimer.h"
#include "ContinousTimer.h"


Timer::Timer(std::function<void (void* arguments)> processor, const struct timeval& timeout, void* arguments) : processor(processor), argument(arguments), delay(timeout)
{
    gettimeofday(&absoluteTimeoutValue, NULL);
    this->update();
}

SingleShotTimer & Timer::singleshotTimer(std::function<void (void* arguments)> processor, const struct timeval& timeout, void* arguments)
{
    SingleShotTimer* timer = new SingleShotTimer(processor, timeout, arguments);
    
    SocketMultiplexer::instance().getTimerManager().add(timer);
    
    return *timer;
}

ContinousTimer & Timer::continousTimer(std::function<void (void* arguments)> processor, const struct timeval& timeout, void* arguments)
{
    ContinousTimer* timer = new ContinousTimer(processor, timeout, arguments);
    
    SocketMultiplexer::instance().getTimerManager().add(timer);
    
    return *timer;
}



struct timeval& Timer::absoluteTimeout()
{
    return absoluteTimeoutValue;
}



void Timer::cancel()
{
    SocketMultiplexer::instance().getTimerManager().remove(this);
}

void Timer::dispatch()
{
    processor(argument);
}








/**
 * 
 * Operators
 * 
 */

Timer::operator struct timeval() const
{
    return absoluteTimeoutValue;
}




bool operator < (const struct timeval& timeValue1, const struct timeval& timeValue2)
{
    return (timeValue1.tv_sec < timeValue2.tv_sec) || ((timeValue1.tv_sec == timeValue2.tv_sec) && (timeValue1.tv_usec < timeValue2.tv_usec));
}

bool operator <= (const struct timeval& timeValue1, const struct timeval& timeValue2)
{
    return timeValue1 < timeValue2 || timeValue1 == timeValue2;
}

bool operator == (const struct timeval& timeValue1, const struct timeval& timeValue2)
{
    return (timeValue1.tv_sec == timeValue2.tv_sec) && (timeValue1.tv_usec == timeValue2.tv_usec);
}

struct timeval operator-(const timeval& timeValue1, const struct timeval& timeValue2)
{
    struct timeval temp;
    
    temp.tv_sec = timeValue1.tv_sec - timeValue2.tv_sec;
    temp.tv_usec = timeValue1.tv_usec - timeValue2.tv_usec;
    
    if (temp.tv_usec < 0)
    {
        temp.tv_sec -= 1;
        temp.tv_usec = 1000000 - temp.tv_usec;
    }
    
    return temp;
}
