#include <sys/time.h>
#include <list>

#include "TimerManager.h"
#include "SingleShotTimer.h"
#include "ContinousTimer.h"

TimerManager::TimerManager(): timerListDirty(false) { }


struct timeval TimerManager::getNextTimeout() {
    struct timeval tv;
    
    tv.tv_sec = 20L;
    tv.tv_usec = 0L;
    
    timerListDirty = timerListDirty || addedList.size() > 0 || removedList.size() > 0;
    
    for (std::list<Timer*>::iterator iterator = addedList.begin(); iterator != addedList.end(); iterator++)
    {
        timerList.push_back(*iterator);
    }
    addedList.clear();
    
    for (std::list<Timer*>::iterator iterator = removedList.begin(); iterator != removedList.end(); iterator++)
    {
        timerList.remove(*iterator);
        delete *iterator;
    }
    removedList.clear();
    
    
    if (!timerList.empty())
    {
        if (timerListDirty)
        {
            timerList.sort(LessThanTimerNodeComparator());
        }
        
        timerListDirty = false;
        
        tv = (*(timerList.begin()))->absoluteTimeout();
        
        struct timeval currentTime;
        gettimeofday(&currentTime, NULL);
        
        if (tv < currentTime)
        {
            tv.tv_sec = 0;
            tv.tv_usec = 0;
        }
        else
        {
            tv = tv - currentTime;
        }
    }
    
    return tv;
}

void TimerManager::process() {
        
    struct timeval currentTime;
    gettimeofday(&currentTime, NULL);
    
    for (std::list<Timer*>::iterator iterator = timerList.begin(); iterator != timerList.end(); iterator++)
    {
        if ((*iterator)->absoluteTimeout() <= currentTime)
        {
            (*iterator)->dispatch();
            
            if (dynamic_cast<SingleShotTimer*>(*iterator))
            {
                remove(*iterator);
            }
            else if (dynamic_cast<ContinousTimer*>(*iterator))
            {
                (*iterator)->update();
                timerListDirty = true;
            }
            else
            {
                break;
            }
        }
    }
}

void TimerManager::add(Timer* timer) {
    addedList.push_back(timer);
}

void TimerManager::remove(Timer* timer) {
    removedList.push_back(timer);
}

bool TimerManager::LessThanTimerNodeComparator::operator()(const Timer* timer1, const Timer* timer2) const
{
    return *timer1 < *timer2;
}



