#ifndef TIMERMANAGER_H
#define TIMERMANAGER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <list>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "Timer.h"


class ManagedTimer
{
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

    class lttimernode
    {
    public:
        bool operator()(const Timer* t1, const Timer* t2) const;
    };


    class timernode_equality {
    public:
        timernode_equality(Timer* timer) : timer(timer) {}

        bool operator()(const Timer* timer) const;

    private:
        Timer* timer;
    };

    bool timerListDirty;
};

#endif // TIMERMANAGER_H
