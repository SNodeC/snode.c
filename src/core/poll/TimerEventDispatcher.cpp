/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021, 2022 Volker Christian <me@vchrist.at>
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

#include "core/poll/TimerEventDispatcher.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <algorithm>
#include <climits>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::poll {

    utils::Timeval TimerEventDispatcher::getNextTimeout(const utils::Timeval& currentTime) {
        utils::Timeval nextTimeout({LONG_MAX, 0});

        for (TimerEventReceiver* timer : addedList) {
            timerList.push_back(timer);
            timerListDirty = true;
        }
        addedList.clear();

        for (TimerEventReceiver* timer : removedList) {
            timerList.remove(timer);
            timer->unobservedEvent();
            timerListDirty = true;
        }
        removedList.clear();

        if (!timerList.empty()) {
            if (timerListDirty) {
                timerList.sort(core::TimerEventDispatcher::timernode_lt());
                timerListDirty = false;
            }

            nextTimeout = (*(timerList.begin()))->getTimeout();

            if (nextTimeout < currentTime) {
                nextTimeout = 0;
            } else {
                nextTimeout -= currentTime;
            }
        }

        return nextTimeout;
    }

    void TimerEventDispatcher::dispatchActiveEvents(const utils::Timeval& currentTime) {
        for (TimerEventReceiver* timer : timerList) {
            if (timer->getTimeout() <= currentTime) {
                timerListDirty = timer->trigger();
            } else {
                break;
            }
        }
    }

    void TimerEventDispatcher::remove(TimerEventReceiver* timer) {
        if (std::find(timerList.begin(), timerList.end(), timer) != timerList.end() &&
            std::find(removedList.begin(), removedList.end(), timer) == removedList.end()) {
            removedList.push_back(timer);
        }
    }

    void TimerEventDispatcher::add(TimerEventReceiver* timer) {
        addedList.push_back(timer);
    }

    bool TimerEventDispatcher::empty() {
        return timerList.empty();
    }

    void TimerEventDispatcher::stop() {
        utils::Timeval currentTime = utils::Timeval::currentTime();

        getNextTimeout(currentTime);

        for (TimerEventReceiver* timer : timerList) {
            remove(timer);
        }

        getNextTimeout(currentTime);
    }

} // namespace core::poll
