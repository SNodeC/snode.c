#ifndef CONTINOUSTIMER_H
#define CONTINOUSTIMER_H

#include "Timer.h"

class ContinousTimer : public Timer
{
public:
    ContinousTimer(std::function<void (const void* arg)> processor, const struct timeval& timeout, const void* arg) : Timer(processor, timeout, arg) {}
    virtual ~ContinousTimer() = default;
};

#endif // CONTINOUSTIMER_H
