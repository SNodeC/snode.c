#ifndef TIMERMANAGER_H
#define TIMERMANAGER_H

#include <list>

#include <Timer.h>

class TimerManager {
public:
    TimerManager();
    struct timeval getNextTimeout();
    void process();
    
    void add(Timer* timer);
    void remove(Timer* timer);
    
protected:
    std::list<Timer*> timerList;
    std::list<Timer*> addedList;
    std::list<Timer*> removedList;
    
private:
    bool timerListDirty;
    
    class lttimernodecomparator {
    public:
        bool operator() (const Timer* t1, const Timer* t2) const;
    };
};

#endif // TIMERMANAGER_H
