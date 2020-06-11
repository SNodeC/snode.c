#ifndef CONTINOUSTIMER_H
#define CONTINOUSTIMER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "Timer.h"


class ContinousTimer : public Timer {
public:
    ContinousTimer(std::function<void(const void* arg)> dispatcher, const struct timeval& timeout, const void* arg)
        : Timer(dispatcher, timeout, arg) {
    }

    virtual ~ContinousTimer() = default;

private:
    ContinousTimer& operator=(const ContinousTimer& timer) {
        return *this;
    }
};

#endif // CONTINOUSTIMER_H
