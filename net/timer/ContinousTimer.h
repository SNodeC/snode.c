#ifndef CONTINOUSTIMER_H
#define CONTINOUSTIMER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "Timer.h"


class ContinousTimer : public Timer {
public:
    ContinousTimer(const std::function<bool(const void* arg)>& dispatcher, const struct timeval& timeout, const void* arg)
        : Timer(dispatcher, timeout, arg) {
    }

    ~ContinousTimer() override = default;

    ContinousTimer& operator=(const ContinousTimer& timer) = delete;
};

#endif // CONTINOUSTIMER_H
