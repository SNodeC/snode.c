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

#include "core/select/EventDispatcher.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "core/system/select.h"

#include <algorithm> // for min, find
#include <cerrno>
#include <climits>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

core::EventDispatcher& EventDispatcher() {
    static core::select::EventDispatcher eventDispatcher;

    return eventDispatcher;
}

namespace core::select {

    core::TimerEventDispatcher& EventDispatcher::getTimerEventDispatcher() {
        return timerEventDispatcher;
    }

    core::DescriptorEventDispatcher& EventDispatcher::getDescriptorEventDispatcher(core::EventDispatcher::DISP_TYPE dispType) {
        return eventDispatcher[dispType];
    }

    int EventDispatcher::getInterestCount() {
        int maxFd = -1;

        for (const DescriptorEventDispatcher& eventDispatcher : eventDispatcher) {
            maxFd = std::max(eventDispatcher.getInterestCount(), maxFd);
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

    void EventDispatcher::unobserveDisabledEvents(const utils::Timeval& currentTime) {
        for (DescriptorEventDispatcher& eventDispatcher : eventDispatcher) {
            eventDispatcher.unobserveDisabledEvents(currentTime);
        }
    }

    void EventDispatcher::dispatchActiveEvents(const utils::Timeval& currentTime) {
        for (DescriptorEventDispatcher& eventDispatcher : eventDispatcher) {
            eventDispatcher.dispatchActiveEvents(currentTime);
        }

        for (DescriptorEventDispatcher& eventDispatcher : eventDispatcher) {
            eventDispatcher.dispatchImmediateEvents(currentTime);
        }
    }

    TickStatus EventDispatcher::dispatch(const utils::Timeval& tickTimeOut, bool stopped) {
        TickStatus tickStatus = TickStatus::SUCCESS;

        EventDispatcher::observeEnabledEvents();

        int interestCount = EventDispatcher::getInterestCount();

        utils::Timeval currentTime = utils::Timeval::currentTime();

        utils::Timeval nextEventTimeout = EventDispatcher::getNextTimeout(currentTime);
        utils::Timeval nextTimerTimeout = timerEventDispatcher.getNextTimeout(currentTime);

        if (interestCount >= 0 || (!timerEventDispatcher.empty() && !stopped)) {
            utils::Timeval nextTimeout = stopped ? nextEventTimeout : std::min(nextTimerTimeout, nextEventTimeout);

            nextTimeout = std::min(nextTimeout, tickTimeOut);
            nextTimeout = std::max(nextTimeout, utils::Timeval()); // In case nextEventTimeout is negativ

            int ret = core::system::select(interestCount + 1,
                                           &eventDispatcher[RD].getFdSet(),
                                           &eventDispatcher[WR].getFdSet(),
                                           &eventDispatcher[EX].getFdSet(),
                                           &nextTimeout);

            if (ret >= 0) {
                currentTime = utils::Timeval::currentTime();

                timerEventDispatcher.dispatchActiveEvents(currentTime);
                EventDispatcher::dispatchActiveEvents(currentTime);
                EventDispatcher::unobserveDisabledEvents(currentTime);
            } else {
                if (errno != EINTR) {
                    tickStatus = TickStatus::ERROR;
                }
            }
        } else {
            tickStatus = TickStatus::NO_OBSERVER;
        }

        return tickStatus;
    }

    void EventDispatcher::stop() {
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

} // namespace core::select
