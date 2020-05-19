#ifndef CONTINOUSTIMER_H
#define CONTINOUSTIMER_H

#include "Timer.h"

class ContinousTimer : public Timer
{
public:
    ContinousTimer(std::function<void (void* arguments)> processor, const struct timeval& timeout, void* arguments) : Timer(processor, timeout, arguments) { }
    
    virtual ~ContinousTimer() { }
};

#endif // CONTINOUSTIMER_H
