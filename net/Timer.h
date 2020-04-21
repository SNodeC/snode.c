#ifndef TIMER_H
#define TIMER_H

#include <iostream>
#include <time.h>
#include <sys/time.h>

#include <functional>

class SingleshotTimer;
class ContinousTimer;

class Timer {
public:
    Timer(std::function<void (void* arg)> processor, const struct timeval& timeout, void* arg);
    virtual ~Timer() = default;
    
    void update();
    struct timeval& absolutTimeout();
    operator struct timeval() const;
    void dispatch();
    
    static SingleshotTimer& singleshotTimer(std::function<void (void* arg)> processor, const struct timeval& timeout, void* arg);
    
    static ContinousTimer& continousTimer(std::function<void (void* arg)> processor, const struct timeval& timeout, void* arg);
    
    static void cancel(Timer* timer);
    
protected:
    struct timeval absoluteTimeout;
    std::function<void (void* arg)> processor;
    struct timeval delay;
    void* arg;
};

bool operator<(const struct timeval &tv1, const struct timeval &tv2);
bool operator>(const struct timeval &tv1, const struct timeval &tv2);
bool operator<=(const struct timeval &tv1, const struct timeval &tv2);
bool operator>=(const struct timeval &tv1, const struct timeval &tv2);
bool operator==(const struct timeval &tv1, const struct timeval &tv2);

struct timeval operator+(const struct timeval &tv1, const struct timeval &tv2);
struct timeval operator-(const struct timeval &tv1, const struct timeval &tv2);


#endif // TIMER_H
