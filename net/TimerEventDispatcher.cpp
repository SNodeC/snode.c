/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020 Volker Christian <me@vchrist.at>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <algorithm> // for copy
#include <climits>
#include <iterator>   // for back_insert_iterator, back_inserter
#include <sys/time.h> // IWYU pragma: export

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "net/TimerEventDispatcher.h"
#include "net/TimerEventReceiver.h" // for TimerEventReceiver

namespace net {

    struct timeval TimerEventDispatcher::getNextTimeout() {
        struct timeval tv {
            LONG_MAX, 0
        };

        for (TimerEventReceiver* timer : addedList) {
            timerList.push_back(timer);
            timerListDirty = true;
        }
        addedList.clear();

        for (TimerEventReceiver* timer : removedList) {
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

        for (TimerEventReceiver* timer : timerList) {
            if (timer->timeout() <= currentTime) {
                timerListDirty = timer->dispatch();
            } else {
                break;
            }
        }
    }

    void TimerEventDispatcher::remove(TimerEventReceiver* timer) {
        removedList.push_back(timer);
    }

    void TimerEventDispatcher::add(TimerEventReceiver* timer) {
        addedList.push_back(timer);
    }

    bool TimerEventDispatcher::empty() {
        return timerList.empty();
    }

    void TimerEventDispatcher::cancelAll() {
        getNextTimeout();

        std::copy(timerList.begin(), timerList.end(), std::back_inserter(removedList));

        getNextTimeout();
    }

} // namespace net
