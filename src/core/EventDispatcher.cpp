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
#include "core/EventReceiver.h"
#include "core/TimerEventDispatcher.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <algorithm> // for min, max
#include <cerrno>    // for EINTR, errno
#include <numeric>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core {

    core::EventDispatcher::EventDispatcher(DescriptorEventDispatcher* const readDescriptorEventDispatcher,
                                           DescriptorEventDispatcher* const writeDescriptorEventDispatcher,
                                           DescriptorEventDispatcher* const exceptionDescriptorEventDispatcher)
        : descriptorEventDispatcher{readDescriptorEventDispatcher, writeDescriptorEventDispatcher, exceptionDescriptorEventDispatcher}
        , timerEventDispatcher(new core::TimerEventDispatcher()) {
    }

    EventDispatcher::~EventDispatcher() {
        for (int i = 0; i < 3; i++) {
            delete descriptorEventDispatcher[i];
        }

        delete timerEventDispatcher;
    }

    core::DescriptorEventDispatcher& EventDispatcher::getDescriptorEventDispatcher(core::EventDispatcher::DISP_TYPE dispType) {
        return *descriptorEventDispatcher[dispType];
    }

    core::TimerEventDispatcher& EventDispatcher::getTimerEventDispatcher() {
        return *timerEventDispatcher;
    }

    int EventDispatcher::getObservedEventReceiverCount() {
        return std::accumulate(descriptorEventDispatcher,
                               descriptorEventDispatcher + 3,
                               0,
                               [](int count, core::DescriptorEventDispatcher* descriptorEventDispatcher) -> int {
                                   return count + descriptorEventDispatcher->getObservedEventReceiverCount();
                               });
    }

    int EventDispatcher::getMaxFd() {
        return std::accumulate(descriptorEventDispatcher,
                               descriptorEventDispatcher + 3,
                               -1,
                               [](int count, core::DescriptorEventDispatcher* descriptorEventDispatcher) -> int {
                                   return std::max(descriptorEventDispatcher->getMaxFd(), count);
                               });
    }

    TickStatus EventDispatcher::dispatch(const utils::Timeval& tickTimeOut, bool stopped) {
        TickStatus tickStatus = TickStatus::SUCCESS;

        EventDispatcher::observeEnabledEvents();

        int observedEventReceiverCount = getObservedEventReceiverCount();

        utils::Timeval currentTime = utils::Timeval::currentTime();

        utils::Timeval nextEventTimeout = getNextTimeout(currentTime);
        utils::Timeval nextTimerTimeout = timerEventDispatcher->getNextTimeout(currentTime);

        if (observedEventReceiverCount > 0 || (!timerEventDispatcher->empty() && !stopped)) {
            utils::Timeval nextTimeout = stopped ? nextEventTimeout : std::min(nextTimerTimeout, nextEventTimeout);

            nextTimeout = std::min(nextTimeout, tickTimeOut);
            nextTimeout = std::max(nextTimeout, utils::Timeval()); // In case nextEventTimeout is negativ

            int ret = multiplex(nextTimeout);

            if (ret >= 0) {
                currentTime = utils::Timeval::currentTime();

                timerEventDispatcher->dispatchActiveEvents(currentTime);

                dispatchActiveEvents(ret, currentTime);
                unobserveDisabledEvents(currentTime);
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
            for (core::DescriptorEventDispatcher* const eventDispatcher : descriptorEventDispatcher) {
                eventDispatcher->stop();
            }
            tickStatus = dispatch(2, true);
        } while (tickStatus == TickStatus::SUCCESS);

        do {
            timerEventDispatcher->stop();
            tickStatus = dispatch(0, false);
        } while (tickStatus == TickStatus::SUCCESS);
    }

    utils::Timeval EventDispatcher::getNextTimeout(const utils::Timeval& currentTime) {
        utils::Timeval nextTimeout = core::EventReceiver::TIMEOUT::MAX;

        for (core::DescriptorEventDispatcher* const eventDispatcher : descriptorEventDispatcher) {
            nextTimeout = std::min(eventDispatcher->getNextTimeout(currentTime), nextTimeout);
        }
        return nextTimeout;
    }

    void EventDispatcher::observeEnabledEvents() {
        for (core::DescriptorEventDispatcher* const eventDispatcher : descriptorEventDispatcher) {
            eventDispatcher->observeEnabledEvents();
        }
    }

    void EventDispatcher::unobserveDisabledEvents(const utils::Timeval& currentTime) {
        for (core::DescriptorEventDispatcher* const eventDispatcher : descriptorEventDispatcher) {
            eventDispatcher->unobserveDisabledEvents(currentTime);
        }
    }

} // namespace core
