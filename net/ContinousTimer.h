#ifndef CONTINOUSTIMER_H
#define CONTINOUSTIMER_H

#include "Timer.h"


class ContinousTimer : public Timer
{
public:
    ContinousTimer(std::function<void (void* arg)> processor, const struct timeval& timeout, void* arg)
    : Timer(processor, timeout, arg) {}
    
protected:
    virtual ~ContinousTimer() {}
};

#endif // CONTINOUSTIMER_H
