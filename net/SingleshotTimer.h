#ifndef SINGLESHOTTIMER_H
#define SINGLESHOTTIMER_H

#include "Timer.h"


class SingleshotTimer : public Timer
{
public:
    SingleshotTimer(std::function<void (const void* arg)> processor, const struct timeval& timeout, const void* arg) : Timer(processor, timeout, arg) {}
    
private:
    SingleshotTimer(const SingleshotTimer& timer) {}
    
    SingleshotTimer& operator=(const SingleshotTimer& timer) {
        return *this;
    }
    
    virtual ~SingleshotTimer() = default;
};

#endif // SINGLESHOTTIMER_H
