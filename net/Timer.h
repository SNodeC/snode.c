#ifndef TIMER_H
#define TIMER_H

#include <time.h>
#include <sys/time.h>

#include <functional>


class SingleshotTimer;
class ContinousTimer;

class Timer {
protected:
    Timer(std::function<void (const void* arg)> processor, const struct timeval& timeout, const void* arg);

protected:
    Timer() {}
    
    Timer(const Timer& timer) {
        *this = timer;
    }
    
    virtual ~Timer() = default;
    
    Timer& operator=(const Timer& timer) {
        return *this;
    }
    
public:
    static ContinousTimer& continousTimer(std::function<void (const void* arg)> processor, const struct timeval& timeout, const void* arg);
    static SingleshotTimer& singleshotTimer(std::function<void (const void* arg)> processor, const struct timeval& timeout, const void* arg);
    
    struct timeval& timeout();
    
    void dispatch();
    void update();
    void cancel();
    void destroy();
    
    operator struct timeval() const;
    
private:
    std::function<void (const void* arg)> processor;
    struct timeval absoluteTimeout;
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
