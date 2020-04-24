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
    Timer(std::function<void (const void* arg)> processor, const struct timeval& timeout, const void* arg);
    virtual ~Timer() = default;
    
private:
    Timer(const Timer& timer) {
        *this = timer;
    }
    
    Timer& operator=(const Timer& timer) {
        return *this;
    }
    
public:
    void update();
    struct timeval& absolutTimeout();
    operator struct timeval() const;
    void dispatch();
    
    static SingleshotTimer& singleshotTimer(std::function<void (const void* arg)> processor, const struct timeval& timeout, const void* arg);
    
    static ContinousTimer& continousTimer(std::function<void (const void* arg)> processor, const struct timeval& timeout, const void* arg);
    
    void cancel();
    
protected:
    struct timeval absoluteTimeout;
    std::function<void (const void* arg)> processor;
    struct timeval delay;
    const void* arg;
};

bool operator<(const struct timeval &tv1, const struct timeval &tv2);
bool operator>(const struct timeval &tv1, const struct timeval &tv2);
bool operator<=(const struct timeval &tv1, const struct timeval &tv2);
bool operator>=(const struct timeval &tv1, const struct timeval &tv2);
bool operator==(const struct timeval &tv1, const struct timeval &tv2);

struct timeval operator+(const struct timeval &tv1, const struct timeval &tv2);
struct timeval operator-(const struct timeval &tv1, const struct timeval &tv2);


#endif // TIMER_H
