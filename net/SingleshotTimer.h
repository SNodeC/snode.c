#ifndef SINGLESHOTTIMER_H
#define SINGLESHOTTIMER_H

#include "Timer.h"

class SingleshotTimer : public Timer
{
public:
    SingleshotTimer(std::function<void (void* arg)> processor, const struct timeval& timeout, void* arg)
    : Timer(processor, timeout, arg) {}
    
protected:
    virtual ~SingleshotTimer() {}
};

#endif // SINGLESHOTTIMER_H
