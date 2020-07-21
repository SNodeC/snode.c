#ifndef TIMER_H
#define TIMER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>
#include <sys/time.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */


class SingleshotTimer;
class ContinousTimer;

class Timer {
protected:
    Timer(const std::function<void(const void* arg)>& dispatcher, const struct timeval& timeout, const void* arg);

    virtual ~Timer() = default;

public:
    Timer(const Timer&) = delete;
    Timer& operator=(const Timer& timer) = delete;

    static ContinousTimer& continousTimer(const std::function<void(const void* arg)>& dispatcher, const struct timeval& timeout,
                                          const void* arg);
    static SingleshotTimer& singleshotTimer(const std::function<void(const void* arg)>& dispatcher, const struct timeval& timeout,
                                            const void* arg);

    struct timeval& timeout();

    void dispatch();
    void update();
    void cancel();
    void destroy();

    explicit operator struct timeval() const;

private:
    struct timeval absoluteTimeout {};
    struct timeval delay;

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
