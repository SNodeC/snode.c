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

#include "core/EventDispatchers.h"

#include "core/DescriptorEventDispatcher.h"
#include "core/TimerEventDispatcher.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <algorithm> // for min, find
#include <climits>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core {

    DescriptorEventDispatcher EventDispatchers::eventDispatcher[];
    TimerEventDispatcher EventDispatchers::timerEventDispatcher;

    TimerEventDispatcher& EventDispatchers::getTimerEventDispatcher() {
        return timerEventDispatcher;
    }

    DescriptorEventDispatcher& EventDispatchers::getDescriptorEventDispatcher(DISP_TYPE dispType) {
        return eventDispatcher[dispType];
    }

    EventDispatchers::FdSet::FdSet() {
        zero();
    }

    void EventDispatchers::FdSet::set(int fd) {
        FD_SET(fd, &registered);
    }

    void EventDispatchers::FdSet::clr(int fd) {
        FD_CLR(fd, &registered);
        FD_CLR(fd, &active);
    }

    int EventDispatchers::FdSet::isSet(int fd) const {
        return FD_ISSET(fd, &active);
    }

    void EventDispatchers::FdSet::zero() {
        FD_ZERO(&registered);
        FD_ZERO(&active);
    }

    fd_set& EventDispatchers::FdSet::get() {
        active = registered;
        return active;
    }

    int EventDispatchers::getMaxFd() {
        int maxFd = -1;

        for (const DescriptorEventDispatcher& eventDispatcher : eventDispatcher) {
            maxFd = std::max(eventDispatcher.getMaxFd(), maxFd);
        }

        return maxFd;
    }

    utils::Timeval EventDispatchers::getNextTimeout(const utils::Timeval& currentTime) {
        utils::Timeval nextTimeout = {LONG_MAX, 0};

        for (const DescriptorEventDispatcher& eventDispatcher : eventDispatcher) {
            nextTimeout = std::min(eventDispatcher.getNextTimeout(currentTime), nextTimeout);
        }

        return nextTimeout;
    }

    void EventDispatchers::observeEnabledEvents() {
        for (DescriptorEventDispatcher& eventDispatcher : eventDispatcher) {
            eventDispatcher.observeEnabledEvents();
        }
    }

    void EventDispatchers::dispatchActiveEvents(const utils::Timeval& currentTime) {
        for (DescriptorEventDispatcher& eventDispatcher : eventDispatcher) {
            eventDispatcher.dispatchActiveEvents(currentTime);
        }
    }

    void EventDispatchers::unobserveDisabledEvents(const utils::Timeval& currentTime) {
        for (DescriptorEventDispatcher& eventDispatcher : eventDispatcher) {
            eventDispatcher.unobserveDisabledEvents(currentTime);
        }
    }

    TickStatus EventDispatchers::dispatch(const utils::Timeval& tickTimeOut, bool stopped) {
        TickStatus tickStatus = TickStatus::SUCCESS;

        EventDispatchers::observeEnabledEvents();

        int maxFd = EventDispatchers::getMaxFd();

        utils::Timeval currentTime = utils::Timeval::currentTime();

        utils::Timeval nextEventTimeout = EventDispatchers::getNextTimeout(currentTime);
        utils::Timeval nextTimerTimeout = timerEventDispatcher.getNextTimeout(currentTime);

        if (maxFd >= 0 || (!timerEventDispatcher.empty() && !stopped)) {
            utils::Timeval nextTimeout = stopped ? nextEventTimeout : std::min(nextTimerTimeout, nextEventTimeout);

            nextTimeout = std::min(nextTimeout, tickTimeOut);
            nextTimeout = std::max(nextTimeout, utils::Timeval()); // In case nextEventTimeout is negativ

            int ret = core::system::select(
                maxFd + 1, &eventDispatcher[RD].getFdSet(), &eventDispatcher[WR].getFdSet(), &eventDispatcher[EX].getFdSet(), &nextTimeout);
            if (ret >= 0) {
                currentTime = utils::Timeval::currentTime();

                timerEventDispatcher.dispatchActiveEvents(currentTime);
                EventDispatchers::dispatchActiveEvents(currentTime);
                EventDispatchers::unobserveDisabledEvents(currentTime);
            } else {
                tickStatus = TickStatus::ERROR;
            }
        } else {
            tickStatus = TickStatus::NO_OBSERVER;
        }

        return tickStatus;
    }

    void EventDispatchers::stop() {
        core::TickStatus tickStatus;

        do {
            for (DescriptorEventDispatcher& eventDispatcher : eventDispatcher) {
                eventDispatcher.stop();
            }
            tickStatus = dispatch(2, true);
        } while (tickStatus == TickStatus::SUCCESS);

        do {
            timerEventDispatcher.stop();
            tickStatus = dispatch(0, false);
        } while (tickStatus == TickStatus::SUCCESS);
    }

} // namespace core
