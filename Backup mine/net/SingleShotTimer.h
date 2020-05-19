#ifndef SINGLESHOTTIMER_H
#define SINGLESHOTTIMER_H

#include "Timer.h"

class SingleShotTimer : public Timer
{
public:
    SingleShotTimer(std::function<void (void* arguments)> processor, const struct timeval& timeout, void* arguments) : Timer(processor, timeout, arguments) { }
    
    virtual ~SingleShotTimer() { }
};

#endif // SINGLESHOTTIMER_H
