#ifndef TIMER_H
#define TIMER_H

<<<<<<< HEAD
#include <functional>

#include <time.h>
#include <sys/time.h>

class SingleshotTimer;
=======
#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <time.h>

#include <functional>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */


class SingleshotTimer;

>>>>>>> MatthiasKoettritsch
class ContinousTimer;

class Timer
{
<<<<<<< HEAD
public:
    Timer(std::function<void (void* arg)> processor, const struct timeval& timeout, void* arg);
    virtual ~Timer() {}
    
    struct timeval& absolutTimeout();
    
    operator struct timeval() const;
    
    void dispatch();
    void update();
    static void cancel(Timer* timer);
    
    static SingleshotTimer& singleshotTimer(std::function<void (void* arg)> processor, const struct timeval& timeout, void* arg);
    static ContinousTimer& continousTimer(std::function<void (void* arg)> processor, const struct timeval& timeout, void* arg);
    
    
protected:
    struct timeval absoluteTimeout;
    struct timeval delay;
    void* arg;
    std::function<void (void* arg)> processor;
};

bool operator<(const struct timeval& tv1, const struct timeval& tv2);
bool operator<=(const struct timeval& tv1, const struct timeval& tv2);

struct timeval operator+(const struct timeval& tv1, const struct timeval& tv2);
struct timeval operator-(const struct timeval& tv1, const struct timeval& tv2);
=======
protected:
	Timer (const std::function<void (const void *arg)> &dispatcher, const struct timeval &timeout, const void *arg);
	
	Timer (const Timer &timer) : dispatcher(0)
	{
		*this = timer;
	}
	
	virtual ~Timer () = default;
	
	Timer &operator= (const Timer &timer)
	{
		return *this;
	}

public:
	static ContinousTimer &continousTimer (const std::function<void (const void *arg)> &dispatcher, const struct timeval &timeout, const void *arg);
	
	static SingleshotTimer &singleShotTimer (const std::function<void (const void *arg)> &processor, const struct timeval &timeout, const void *arg);
	
	struct timeval &timeout ();
	
	void dispatch ();
	
	virtual void update ();
	
	void cancel ();
	
	void destroy ();
	
	operator struct timeval () const;

protected:
	struct timeval absoluteTimeout;
	struct timeval delay;

private:
	std::function<void (const void *arg)> dispatcher;
	const void *arg;
};

bool operator< (const struct timeval &tv1, const struct timeval &tv2);

bool operator> (const struct timeval &tv1, const struct timeval &tv2);

bool operator<= (const struct timeval &tv1, const struct timeval &tv2);

bool operator>= (const struct timeval &tv1, const struct timeval &tv2);

bool operator== (const struct timeval &tv1, const struct timeval &tv2);

struct timeval operator+ (const struct timeval &tv1, const struct timeval &tv2);

struct timeval operator- (const struct timeval &tv1, const struct timeval &tv2);

>>>>>>> MatthiasKoettritsch

#endif // TIMER_H
