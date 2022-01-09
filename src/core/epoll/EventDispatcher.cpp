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

#include "core/epoll/EventDispatcher.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <algorithm> // for min, find
#include <climits>
#include <sys/epoll.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::epoll {

    core::TimerEventDispatcher& EventDispatcher::getTimerEventDispatcher() {
        return timerEventDispatcher;
    }

    EventDispatcher::EventDispatcher()
        : eventDispatcher{core::epoll::DescriptorEventDispatcher(EPOLLIN),
                          core::epoll::DescriptorEventDispatcher(EPOLLOUT),
                          core::epoll::DescriptorEventDispatcher(EPOLLPRI),
                          core::epoll::DescriptorEventDispatcher(EPOLLIN)} {
        epfd = epoll_create1(EPOLL_CLOEXEC);

        epoll_event event;
        event.events = EPOLLIN;

        event.data.ptr = &eventDispatcher[0];
        epoll_ctl(epfd, EPOLL_CTL_ADD, eventDispatcher[0].getEPFd(), &event);

        event.data.ptr = &eventDispatcher[1];
        epoll_ctl(epfd, EPOLL_CTL_ADD, eventDispatcher[1].getEPFd(), &event);

        event.data.ptr = &eventDispatcher[2];
        epoll_ctl(epfd, EPOLL_CTL_ADD, eventDispatcher[2].getEPFd(), &event);
    }

    core::DescriptorEventDispatcher& EventDispatcher::getDescriptorEventDispatcher(core::EventDispatcher::DISP_TYPE dispType) {
        return eventDispatcher[dispType];
    }

    int EventDispatcher::getReceiverCount() {
        int receiverCount = 0;

        for (const DescriptorEventDispatcher& eventDispatcher : eventDispatcher) {
            receiverCount += eventDispatcher.getReceiverCount();
        }

        return receiverCount;
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

    void EventDispatcher::dispatchActiveEvents(int count, const utils::Timeval& currentTime, const utils::Timeval& timeOut) {
        if (count > 0) {
            for (int i = 0; i < count; i++) {
                static_cast<DescriptorEventDispatcher*>(ePollEvents[i].data.ptr)->dispatchActiveEvents(currentTime, timeOut);
            }
        }

        if (timeOut == 0) {
            for (DescriptorEventDispatcher& eventDispatcher : eventDispatcher) {
                eventDispatcher.doContinueImmediately(currentTime);
            }
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

        int receiverCount = EventDispatcher::getReceiverCount();

        utils::Timeval currentTime = utils::Timeval::currentTime();

        utils::Timeval nextEventTimeout = EventDispatcher::getNextTimeout(currentTime);
        utils::Timeval nextTimerTimeout = timerEventDispatcher.getNextTimeout(currentTime);

        if (receiverCount > 0 || (!timerEventDispatcher.empty() && !stopped)) {
            utils::Timeval nextTimeout = stopped ? nextEventTimeout : std::min(nextTimerTimeout, nextEventTimeout);

            nextTimeout = std::min(nextTimeout, tickTimeOut);
            nextTimeout = std::max(nextTimeout, utils::Timeval()); // In case nextEventTimeout is negativ

            int ret = 0;

            ret = epoll_wait(epfd, ePollEvents, 3, nextTimeout.ms());
            if (ret >= 0) {
                currentTime = utils::Timeval::currentTime();

                timerEventDispatcher.dispatchActiveEvents(currentTime);
                EventDispatcher::dispatchActiveEvents(ret, currentTime, nextTimeout);
                EventDispatcher::unobserveDisabledEvents(currentTime);
            } else {
                tickStatus = TickStatus::ERROR;
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

} // namespace core::epoll
