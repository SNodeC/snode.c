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

#include "core/poll/EventDispatcher.h"

#include "core/EventReceiver.h"
#include "log/Logger.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <algorithm> // for min, find
#include <climits>
#include <sys/poll.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::poll {

    EventDispatcher::PollFds::PollFds() {
        size = 0;
        pollfd pollFd;
        pollFd.fd = 0;
        pollFd.events = 0;
        pollFds.resize(1, pollFd);
    }

    void EventDispatcher::PollFds::add(EventReceiver* eventReceiver, short events) {
        int fd = eventReceiver->getRegisteredFd();

        std::vector<pollfd>::iterator pollFd = std::find_if(pollFds.begin(), pollFds.begin() + size, [&fd](pollfd pollFd) -> bool {
            return pollFd.fd == fd;
        });

        if (pollFd != (pollFds.begin() + size)) {
            pollFd->events |= events;
        } else {
            pollFds[size].events = events;
            pollFds[size].fd = eventReceiver->getRegisteredFd();

            size++;
            if (size >= pollFds.size()) {
                pollfd pollFd;
                pollFd.fd = 0;
                pollFd.events = 0;
                pollFds.resize(pollFds.size() * 2, pollFd);
            }
        }
    }

    void EventDispatcher::PollFds::del(EventReceiver* eventReceiver, short events) {
        int fd = eventReceiver->getRegisteredFd();

        std::vector<pollfd>::iterator pollFd = std::find_if(pollFds.begin(), pollFds.begin() + size, [&fd](pollfd pollFd) -> bool {
            return pollFd.fd == fd;
        });

        if (pollFd != (pollFds.begin() + size)) {
            pollFd->events &= ~events;
            if (pollFd->events == 0) {
                pollFd->fd = -1;
                size--;
            }
        }
    }

    pollfd* EventDispatcher::PollFds::getEvents() {
        return pollFds.data();
    }

    int EventDispatcher::PollFds::getMaxEvents() const {
        return static_cast<int>(size);
    }

    void EventDispatcher::PollFds::compress() {
        if (pollFds.size() > (size * 2) + 1) {
            std::vector<pollfd>::iterator it = pollFds.begin();
            std::vector<pollfd>::iterator rit = pollFds.end() - 1;

            while (it < rit) {
                while (it != pollFds.end() && it->fd != -1) {
                    ++it;
                }
                //                while (rit >= it && rit->fd == -1) {
                while (rit != pollFds.begin() && rit->fd == -1) {
                    --rit;
                }

                while (it->fd == -1 && rit->fd != -1 && it < rit) {
                    pollfd tPollFd = *it;
                    *it = *rit;
                    *rit = tPollFd;
                    ++it;
                    --rit;
                }
            }

            while (pollFds.size() > (size * 2) + 1) {
                pollFds.resize(pollFds.size() / 2);
            }
        }
    }
    /*
    std::vector<int> v{-1, -1, 3, 3, 5, -1, 4, 3, 2, 1};

    std::vector<int>::iterator it = v.begin();
    std::vector<int>::iterator rit = v.end() - 1;

    int k = 0;
    while (it < rit) {
        for (; it != v.end() && *it != -1; ++it)
            ;
        for (; rit >= it && *rit == -1; --rit)
            ;

        while (*it == -1 && *rit != -1 && it < rit) {
            int tmp = *it;
            *it = *rit;
            *rit = tmp;
            ++it;
            --rit;
        }
    }
    */

    void EventDispatcher::PollFds::printStats() {
        VLOG(0) << "PollFds stats: Vector size = " << pollFds.size() << ", interrest count = " << size;
    }

    /*
        core::TimerEventDispatcher& EventDispatcher::getTimerEventDispatcher() {
            return timerEventDispatcher;
        }

        EventDispatcher::EventDispatcher()
            : eventDispatcher{core::poll::DescriptorEventDispatcher(POLLIN),
                              core::poll::DescriptorEventDispatcher(POLLOUT),
                              core::poll::DescriptorEventDispatcher(POLLPRI)} {
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

        void EventDispatcher::dispatchActiveEvents(int count, const utils::Timeval& currentTime) {
            if (count > 0) {
                for (int i = 0; i < count; i++) {
                    static_cast<DescriptorEventDispatcher*>(ePollEvents[i].data.ptr)->dispatchActiveEvents(currentTime);
                }
            }

            for (DescriptorEventDispatcher& eventDispatcher : eventDispatcher) {
                eventDispatcher.dispatchImmediateEvents(currentTime);
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

                int ret = epoll_wait(epfd, ePollEvents, 3, nextTimeout.ms());

                if (ret >= 0) {
                    currentTime = utils::Timeval::currentTime();

                    timerEventDispatcher.dispatchActiveEvents(currentTime);
                    EventDispatcher::dispatchActiveEvents(ret, currentTime);
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
        */

} // namespace core::poll
