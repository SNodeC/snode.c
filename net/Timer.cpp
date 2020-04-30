#include "Multiplexer.h"
#include "SingleshotTimer.h"
#include "ContinousTimer.h"

#include "Timer.h"


Timer::Timer(const std::function<void (const void* arg)>& processor, const struct timeval& timeout, const void* arg) : processor(processor), arg(arg), delay(timeout) {
    gettimeofday(&absoluteTimeout, NULL);
    update();
}


SingleshotTimer& Timer::singleshotTimer(const std::function<void (const void* arg)>& processor, const struct timeval& timeout, const void* arg) {
    SingleshotTimer* st = new SingleshotTimer(processor, timeout, arg);
    
    Multiplexer::instance().getTimerManager().add(st);
    
    return *st;
}

ContinousTimer& Timer::continousTimer(const std::function<void (const void* arg)>& processor, const struct timeval& timeout, const void* arg) {
    ContinousTimer* ct = new ContinousTimer(processor, timeout, arg);
    
    Multiplexer::instance().getTimerManager().add(ct);
    
    return *ct;
}


void Timer::cancel() {
    Multiplexer::instance().getTimerManager().remove(this);
}


void Timer::update() {
    absoluteTimeout = absoluteTimeout + delay;
}


void Timer::dispatch() {
    processor(arg);
}


void Timer::destroy() {
    delete this;
}


struct timeval& Timer::timeout() {
    return absoluteTimeout;
}


Timer::operator struct timeval() const
{
    return absoluteTimeout;
}


bool operator<(const struct timeval &tv1, const struct timeval &tv2)
{
    return (tv1.tv_sec < tv2.tv_sec) || ((tv1.tv_sec == tv2.tv_sec) && (tv1.tv_usec < tv2.tv_usec));
}


bool operator>(const struct timeval &tv1, const struct timeval &tv2)
{
    return (tv2.tv_sec < tv1.tv_sec) || ((tv2.tv_sec == tv1.tv_sec) && (tv2.tv_usec < tv1.tv_usec));
}


bool operator<=(const struct timeval &tv1, const struct timeval &tv2)
{
    return !(tv1 > tv2);
}


bool operator>=(const struct timeval &tv1, const struct timeval &tv2)
{
    return !(tv1 < tv2);
}


bool operator==(const struct timeval &tv1, const struct timeval &tv2)
{
    return !(tv1 < tv2) && !(tv2 < tv1);
}


struct timeval operator+(const struct timeval &tv1, const struct timeval &tv2)
{
    struct timeval help;
    
    help.tv_sec = tv1.tv_sec + tv2.tv_sec;
    
    help.tv_usec = tv1.tv_usec + tv2.tv_usec;
    
    if (help.tv_usec > 999999) {
        help.tv_usec -= 1000000;
        help.tv_sec++;
    }
    
    return help;
}


struct timeval operator-(const struct timeval &tv1, const struct timeval &tv2)
{
    struct timeval help;
    
    help.tv_sec = tv1.tv_sec - tv2.tv_sec;
    help.tv_usec = tv1.tv_usec - tv2.tv_usec;
    
    if (help.tv_usec < 0) {
        help.tv_usec += 1000000;
        help.tv_sec--;
    }
    
    return help;
}
