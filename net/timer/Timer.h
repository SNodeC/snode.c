#ifndef TIMER_H
#define TIMER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>
#include <sys/time.h>
#include <time.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */


class SingleshotTimer;
class ContinousTimer;

class Timer {
protected:
    Timer(const std::function<void(const void* arg)>& dispatcher, const struct timeval& timeout, const void* arg);

    Timer(const Timer& timer)
        : dispatcher(0) {
        *this = timer;
    }

    virtual ~Timer() = default;

    Timer& operator=(const Timer& timer) {
        return *this;
    }

public:
    static ContinousTimer& continousTimer(const std::function<void(const void* arg)>& dispatcher, const struct timeval& timeout,
                                          const void* arg);
    static SingleshotTimer& singleshotTimer(const std::function<void(const void* arg)>& processor, const struct timeval& timeout,
                                            const void* arg);

    struct timeval& timeout();

    void dispatch();
    void update();
    void cancel();
    void destroy();

    operator struct timeval() const;

protected:
    struct timeval absoluteTimeout;
    struct timeval delay;

private:
    std::function<void(const void* arg)> dispatcher;
    const void* arg;
};

bool operator<(const struct timeval& tv1, const struct timeval& tv2);
bool operator>(const struct timeval& tv1, const struct timeval& tv2);
bool operator<=(const struct timeval& tv1, const struct timeval& tv2);
bool operator>=(const struct timeval& tv1, const struct timeval& tv2);
bool operator==(const struct timeval& tv1, const struct timeval& tv2);

struct timeval operator+(const struct timeval& tv1, const struct timeval& tv2);
struct timeval operator-(const struct timeval& tv1, const struct timeval& tv2);


#endif // TIMER_H
