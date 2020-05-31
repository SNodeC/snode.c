#ifndef TIMERMANAGER_H
#define TIMERMANAGER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <list>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "timer/Timer.h"


class ManagedTimer {
public:
    ManagedTimer();

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
            return *t1 < *t2;
        }
    };

    bool timerListDirty;
};

#endif // TIMERMANAGER_H
