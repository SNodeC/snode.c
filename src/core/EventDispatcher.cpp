/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021 Volker Christian <me@vchrist.at>
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

#include "core/EventDispatcher.h"

#include "core/DescriptorEventDispatcher.h"
#include "core/TimerEventDispatcher.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <algorithm> // for min, find
#include <climits>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core {

    DescriptorEventDispatcher EventDispatcher::eventDispatcher[];
    TimerEventDispatcher EventDispatcher::timerEventDispatcher;

    DescriptorEventDispatcher& EventDispatcher::getReadEventDispatcher() {
        return eventDispatcher[RD];
    }

    DescriptorEventDispatcher& EventDispatcher::getWriteEventDispatcher() {
        return eventDispatcher[WR];
    }

    DescriptorEventDispatcher& EventDispatcher::getExceptionalConditionEventDispatcher() {
        return eventDispatcher[EX];
    }

    TimerEventDispatcher& EventDispatcher::getTimerEventDispatcher() {
        return timerEventDispatcher;
    }

    DescriptorEventDispatcher& EventDispatcher::getDescriptorEventDispatcher(DISP_TYPE dispType) {
        return eventDispatcher[dispType];
    }

    EventDispatcher::FdSet::FdSet() {
        zero();
    }

    void EventDispatcher::FdSet::set(int fd) {
        FD_SET(fd, &registered);
    }

    void EventDispatcher::FdSet::clr(int fd) {
        FD_CLR(fd, &registered);
        FD_CLR(fd, &active);
    }

    int EventDispatcher::FdSet::isSet(int fd) const {
        return FD_ISSET(fd, &active);
    }

    void EventDispatcher::FdSet::zero() {
        FD_ZERO(&registered);
        FD_ZERO(&active);
    }

    fd_set& EventDispatcher::FdSet::get() {
        active = registered;
        return active;
    }

    int EventDispatcher::getMaxFd() {
        int maxFd = -1;

        for (const DescriptorEventDispatcher& eventDispatcher : eventDispatcher) {
            maxFd = std::max(eventDispatcher.getMaxFd(), maxFd);
        }

        return maxFd;
    }

    utils::Timeval EventDispatcher::getNextTimeout(const utils::Timeval& currentTime) {
        utils::Timeval nextTimeout = {LONG_MAX, 0};

        for (const DescriptorEventDispatcher& eventDispatcher : eventDispatcher) {
            nextTimeout = std::min(eventDispatcher.getNextTimeout(currentTime), nextTimeout);
        }

        return nextTimeout;
    }

    void EventDispatcher::observeEnabledEvents() {
        for (DescriptorEventDispatcher& eventDispatcher : eventDispatcher) {
            eventDispatcher.observeEnabledEvents();
        }
    }

    void EventDispatcher::dispatchActiveEvents(const utils::Timeval& currentTime) {
        for (DescriptorEventDispatcher& eventDispatcher : eventDispatcher) {
            eventDispatcher.dispatchActiveEvents(currentTime);
        }
    }

    void EventDispatcher::unobserveDisabledEvents(const utils::Timeval& currentTime) {
        for (DescriptorEventDispatcher& eventDispatcher : eventDispatcher) {
            eventDispatcher.unobserveDisabledEvents(currentTime);
        }
    }

    TickStatus EventDispatcher::dispatch(const utils::Timeval& tickTimeOut, bool stopped) {
        TickStatus tickStatus = TickStatus::SUCCESS;

        EventDispatcher::observeEnabledEvents();

        int maxFd = EventDispatcher::getMaxFd();

        utils::Timeval currentTime = utils::Timeval::currentTime();

        utils::Timeval nextEventTimeout = EventDispatcher::getNextTimeout(currentTime);
        utils::Timeval nextTimerTimeout = timerEventDispatcher.getNextTimeout(currentTime);

        utils::Timeval nextTimeout = std::min(nextTimerTimeout, nextEventTimeout);

        if (maxFd >= 0 || (!timerEventDispatcher.empty() && !stopped)) {
            nextTimeout = std::min(nextTimeout, tickTimeOut);
            nextTimeout = std::max(nextTimeout, utils::Timeval()); // In case nextEventTimeout is negativ

            int ret = core::system::select(
                maxFd + 1, &eventDispatcher[RD].getFdSet(), &eventDispatcher[WR].getFdSet(), &eventDispatcher[EX].getFdSet(), &nextTimeout);
            if (ret >= 0) {
                currentTime = utils::Timeval::currentTime();

                timerEventDispatcher.dispatchActiveEvents(currentTime);
                EventDispatcher::dispatchActiveEvents(currentTime);
                EventDispatcher::unobserveDisabledEvents(currentTime);
            } else {
                tickStatus = TickStatus::ERROR;
            }
        } else {
            tickStatus = TickStatus::NO_OBSERVER;
        }

        return tickStatus;
    }

    void EventDispatcher::stopDescriptorEvents() {
        core::TickStatus tickStatus;

        do {
            for (DescriptorEventDispatcher& eventDispatcher : eventDispatcher) {
                eventDispatcher.stop();
            }
            tickStatus = dispatch(2, true);
        } while (tickStatus == TickStatus::SUCCESS);
    }

    void EventDispatcher::stopTimerEvents() {
        core::TickStatus tickStatus;

        do {
            timerEventDispatcher.stop();
            tickStatus = dispatch(0, false);
        } while (tickStatus == TickStatus::SUCCESS);
    }

    void EventDispatcher::stop() {
        stopDescriptorEvents();
        stopTimerEvents();
    }

} // namespace core
