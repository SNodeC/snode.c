#ifndef CONTINOUSTIMER_H
#define CONTINOUSTIMER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "Timer.h"


class ContinousTimer : public Timer {
public:
    ContinousTimer(const std::function<void(const void* arg, const std::function<void()>& stop)>& dispatcher, const struct timeval& timeout,
                   const void* arg)
        : Timer(timeout, arg)
        , dispatcherS(dispatcher) {
    }

    ContinousTimer(const std::function<void(const void* arg)>& dispatcher, const struct timeval& timeout, const void* arg)
        : Timer(timeout, arg)
        , dispatcherC(dispatcher) {
    }

    ~ContinousTimer() override = default;

    ContinousTimer& operator=(const ContinousTimer& timer) = delete;

    bool dispatch() override {
        bool stop = false;

        if (dispatcherS) {
            dispatcherS(arg, [&stop]() -> void {
                stop = true;
            });
            if (stop) {
                cancel();
            } else {
                update();
            }
        } else if (dispatcherC) {
            dispatcherC(arg);
            update();
        }

        return !stop;
    }

private:
    std::function<void(const void* arg, const std::function<void()>& stop)> dispatcherS = nullptr;
    std::function<void(const void* arg)> dispatcherC = nullptr;
};

#endif // CONTINOUSTIMER_H
