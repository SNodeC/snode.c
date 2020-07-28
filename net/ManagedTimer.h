#ifndef MANAGEDTIMER_H
#define MANAGEDTIMER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <list>
#include <sys/time.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "timer/Timer.h"


class ManagedTimer {
public:
    ManagedTimer() = default;

    struct timeval getNextTimeout();

    void dispatch();

    void remove(Timer* timer);
    void add(Timer* timer);

private:
    std::list<Timer*> timerList;
    std::list<Timer*> addedList;
    std::list<Timer*> removedList;

    class timernode_lt {
    public:
        bool operator()(const Timer* t1, const Timer* t2) const {
            return static_cast<timeval>(*t1) < static_cast<timeval>(*t2);
        }
    };

    bool timerListDirty = false;
};

#endif // MANAGEDTIMER_H
