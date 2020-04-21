#ifndef TIMERMANAGER_H
#define TIMERMANAGER_H

#include <list>

#include "Timer.h"

class TimerManager
{
public:
    TimerManager();
    
    struct timeval getNextTimeout();
    
    int process();
    
    void remove(Timer* timer);
    void add(Timer* timer);
    
protected:
    
    std::list<Timer*> timerList;
    std::list<Timer*> addedList;
    std::list<Timer*> removedList;
    
private:
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
