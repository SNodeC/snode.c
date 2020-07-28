#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <sys/time.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "ManagedTimer.h"
#include "timer/ContinousTimer.h"
#include "timer/SingleshotTimer.h"


struct timeval ManagedTimer::getNextTimeout() {
    struct timeval tv {
        0, 0
    };

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

        struct timeval currentTime {
            0, 0
        };
        gettimeofday(&currentTime, nullptr);

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
    struct timeval currentTime {
        0, 0
    };
    gettimeofday(&currentTime, nullptr);

    for (Timer* timer : timerList) {
        if (timer->timeout() <= currentTime) {
            timerListDirty = timer->dispatch();
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
