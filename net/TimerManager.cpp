#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <sys/time.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "TimerManager.h"
#include "Timer.h"
#include "SingleshotTimer.h"
#include "ContinousTimer.h"


TimerManager::TimerManager() : timerListDirty(false) {
}


struct timeval TimerManager::getNextTimeout() {
    struct timeval tv;
    
    tv.tv_sec = 20L;
    tv.tv_usec = 0L;
    
    for(std::list<Timer*>::iterator it = addedList.begin(); it != addedList.end(); ++it) {
        timerList.push_back(*it);
        timerListDirty = true;
    }
    addedList.clear();
    
    for(std::list<Timer*>::iterator it = removedList.begin(); it != removedList.end(); ++it) {
        timerList.remove(*it);
        (*it)->destroy();
        timerListDirty = true;
    }
    removedList.clear();
    
    if (!timerList.empty()) {
        if (timerListDirty) {
            timerList.sort(lttimernode());
            timerListDirty = false;
        }
        
        tv = (*(timerList.begin()))->timeout();
        
        struct timeval currentTime;
        gettimeofday(&currentTime, NULL);
        
        if (tv < currentTime) {
            tv.tv_sec = 0;
            tv.tv_usec = 0;
        } else {
            tv = tv - currentTime;
        }
    }
    
    return tv;
}


void TimerManager::dispatch() {
    struct timeval currentTime;
    gettimeofday(&currentTime, NULL);
    
    for (std::list<Timer*>::iterator it = timerList.begin(); it != timerList.end(); ++it) {
        if ((*it)->timeout() <= currentTime) {
            (*it)->dispatch();
            if (dynamic_cast<SingleshotTimer*>(*it)) {
                remove(*it);
            } else if (dynamic_cast<ContinousTimer*>(*it)) {
                (*it)->update();
                timerListDirty = true;
            }
        } else {
            break;
        }
    }
}


void TimerManager::remove(Timer* timer) {
    removedList.push_back(timer);
}


void TimerManager::add(Timer* timer) {
    addedList.push_back(timer);
}


bool TimerManager::lttimernode::operator()(const Timer* t1, const Timer* t2) const
{
    return *t1 < *t2;
}


bool TimerManager::timernode_equality::operator()(const Timer* timer) const {
    return *timer == *this->timer;
}
