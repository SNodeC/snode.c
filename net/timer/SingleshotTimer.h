#ifndef SINGLESHOTTIMER_H
#define SINGLESHOTTIMER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "Timer.h"


class SingleshotTimer : public Timer {
public:
    SingleshotTimer(const std::function<void(const void* arg)>& dispatcher, const struct timeval& timeout, const void* arg)
        : Timer(timeout, arg)
        , dispatcher(dispatcher) {
    }

    ~SingleshotTimer() override = default;

    bool dispatch() override {
        dispatcher(arg);
        cancel();
        return false;
    }

    SingleshotTimer& operator=(const SingleshotTimer& timer) = delete;

private:
    std::function<void(const void* arg)> dispatcher;
};

#endif // SINGLESHOTTIMER_H
