#ifndef CONTINOUSTIMER_H
#define CONTINOUSTIMER_H

#include "Timer.h"


class ContinousTimer : public Timer
{
public:
<<<<<<< HEAD
    ContinousTimer(std::function<void (void* arg)> processor, const struct timeval& timeout, void* arg)
    : Timer(processor, timeout, arg) {}
    
protected:
    virtual ~ContinousTimer() {}
=======
	ContinousTimer (std::function<void (const void *arg)> dispatcher, const struct timeval &timeout, const void *arg) : Timer(dispatcher, timeout,
	                                                                                                                          arg)
	{}
	
	virtual ~ContinousTimer () = default;

private:
	ContinousTimer &operator= (const ContinousTimer &timer)
	{
		return *this;
	}
>>>>>>> MatthiasKoettritsch
};

#endif // CONTINOUSTIMER_H
