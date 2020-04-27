#ifndef CONTINOUSTIMER_H
#define CONTINOUSTIMER_H

#include "Timer.h"


class ContinousTimer : public Timer
{
public:
    ContinousTimer(std::function<void (const void* arg)> processor, const struct timeval& timeout, const void* arg) : Timer(processor, timeout, arg) {}
    
private:
    ContinousTimer(const ContinousTimer& timer) {}
    
    ContinousTimer& operator=(const ContinousTimer& timer) {
        return *this;
    }
    
    virtual ~ContinousTimer() = default;
};

#endif // CONTINOUSTIMER_H
