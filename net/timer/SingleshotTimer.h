#ifndef SINGLESHOTTIMER_H
#define SINGLESHOTTIMER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "Timer.h"


class SingleshotTimer : public Timer {
public:
    SingleshotTimer(const std::function<void(const void* arg)>& processor, const struct timeval& timeout, const void* arg)
        : Timer(processor, timeout, arg) {
    }

    virtual ~SingleshotTimer() = default;

private:
    SingleshotTimer& operator=(const SingleshotTimer& timer) {
        return *this;
    }
};

#endif // SINGLESHOTTIMER_H
