/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020  Volker Christian <me@vchrist.at>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <sys/time.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "TimerEventDispatcher.h"

struct timeval TimerEventDispatcher::getNextTimeout() {
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

void TimerEventDispatcher::dispatch() {
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

void TimerEventDispatcher::remove(Timer* timer) {
    removedList.push_back(timer);
}

void TimerEventDispatcher::add(Timer* timer) {
    addedList.push_back(timer);
}

bool TimerEventDispatcher::empty() {
    return timerList.empty();
}

void TimerEventDispatcher::cancelAll() {
    getNextTimeout();

    for (Timer* timer : timerList) {
        removedList.push_back(timer);
    }

    getNextTimeout();
}
