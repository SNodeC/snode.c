#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <algorithm>
#include <sys/time.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "ManagedTimer.h"
#include "timer/ContinousTimer.h"
#include "timer/SingleshotTimer.h"
#include "timer/Timer.h"


ManagedTimer::ManagedTimer()
    : timerListDirty(false) {
}


struct timeval ManagedTimer::getNextTimeout() {
    struct timeval tv;

    tv.tv_sec = 20L;
    tv.tv_usec = 0L;

    for (Timer* timer : addedList) {
        timerList.push_back(timer);
        timerListDirty = true;
    }

    addedList.clear();

    for (Timer* timer : removedList) {
        timerList.remove(timer);
        timer->destroy();
        timerListDirty = true;
    }

    removedList.clear();

    if (!timerList.empty()) {
        if (timerListDirty) {
            timerList.sort(timernode_lt());
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

    for (Timer* timer : timerList) {
        if (timer->timeout() <= currentTime) {
            timer->dispatch();
            if (dynamic_cast<SingleshotTimer*>(timer)) {
                remove(timer);
            } else if (dynamic_cast<ContinousTimer*>(timer)) {
                timer->update();
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
