#ifndef SINGLESHOTTIMER_H
#define SINGLESHOTTIMER_H

#include "Timer.h"

<<<<<<< HEAD
class SingleshotTimer : public Timer
{
public:
    SingleshotTimer(std::function<void (void* arg)> processor, const struct timeval& timeout, void* arg)
    : Timer(processor, timeout, arg) {}
    
protected:
    virtual ~SingleshotTimer() {}
=======

class SingleshotTimer : public Timer
{
public:
	SingleshotTimer (std::function<void (const void *arg)> processor, const struct timeval &timeout, const void *arg) : Timer(processor, timeout, arg)
	{}
	
	virtual ~SingleshotTimer () = default;

private:
	SingleshotTimer &operator= (const SingleshotTimer &timer)
	{
		return *this;
	}
>>>>>>> MatthiasKoettritsch
};

#endif // SINGLESHOTTIMER_H
