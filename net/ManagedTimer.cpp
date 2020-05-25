#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <sys/time.h>

#include <algorithm>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "ManagedTimer.h"
#include "Timer.h"
#include "SingleshotTimer.h"
#include "ContinousTimer.h"


ManagedTimer::ManagedTimer() : timerListDirty(false) {
}


struct timeval ManagedTimer::getNextTimeout() {
    struct timeval tv;

    tv.tv_sec = 20L;
    tv.tv_usec = 0L;

    for_each(addedList.begin(), addedList.end(),
        [this] (Timer* timer) {
            timerList.push_back(timer);
            timerListDirty = true;
        }
    );
    addedList.clear();

    for_each(addedList.begin(), addedList.end(),
        [this] (Timer* timer) {
            timerList.remove(timer);
            timer->destroy();
            timerListDirty = true;
        }
    );
    
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


void ManagedTimer::dispatch() {
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


void ManagedTimer::remove(Timer* timer) {
    removedList.push_back(timer);
}


void ManagedTimer::add(Timer* timer) {
    addedList.push_back(timer);
}


bool ManagedTimer::lttimernode::operator()(const Timer* t1, const Timer* t2) const
{
    return *t1 < *t2;
}


bool ManagedTimer::timernode_equality::operator()(const Timer* timer) const {
    return *timer == *this->timer;
}
