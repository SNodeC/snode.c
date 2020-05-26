#ifndef TIMERMANAGER_H
#define TIMERMANAGER_H

#include <list>

#include "Timer.h"
/**
 * @todo write docs
 */
class TimerManager
{
private:
    bool timerListDirty;
    
    class LessThanTimerNodeComparator
    {
    public:
        bool operator() (const Timer* timer1, const Timer* timer2) const;
    };
    
protected:
    std::list<Timer*> timerList;
    std::list<Timer*> addedList;
    std::list<Timer*> removedList;
    
public:
    TimerManager();
    
    struct timeval getNextTimeout();
    void process();
    
    void add(Timer* timer);
    void remove(Timer* timer);
};

#endif // TIMERMANAGER_H
