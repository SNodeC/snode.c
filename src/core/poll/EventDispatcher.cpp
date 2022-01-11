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
#include <iostream>
#include <sys/poll.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::poll {

    PollFds::PollFds() {
        interestCount = 0;
        pollfd pollFd;
        pollFd.fd = 0;
        pollFd.events = 0;
        pollFd.revents = 0;
        pollFds.resize(1, pollFd);
    }

    void PollFds::add(EventReceiver* eventReceiver, short event) {
        int fd = eventReceiver->getRegisteredFd();

        std::map<int, PollEvent>::iterator it = pollEvents.find(fd);
        if (it == pollEvents.end()) {
            pollFds[interestCount].events = event;
            pollFds[interestCount].fd = fd;

            PollEvent pollEvent(interestCount);
            pollEvent.add(event, eventReceiver);
            pollEvents.insert({fd, pollEvent});

            interestCount++;
            if (interestCount >= pollFds.size()) {
                pollfd pollFd;
                pollFd.fd = -1;
                pollFd.events = 0;
                pollFd.revents = 0;
                pollFds.resize(pollFds.size() * 2, pollFd);
            }

        } else {
            PollEvent& pollEvent = it->second;

            pollEvent.add(event, eventReceiver);
            pollFds[pollEvent.fds].events |= event;
        }
    }

    void PollFds::del(EventReceiver* eventReceiver, short event) {
        int fd = eventReceiver->getRegisteredFd();

        std::map<int, PollEvent>::iterator it = pollEvents.find(fd);
        if (it != pollEvents.end()) {
            PollEvent& pollEvent = it->second;

            pollfd& pollFd = pollFds[pollEvent.fds];
            pollFd.events &= ~event;

            if (pollFd.events == 0) {
                pollFd.fd = -1;
                interestCount--;
                //                pollEvents.erase(fd);
            } else {
                //                pollEvent.del(event);
            }
        }
    }

    void PollFds::finish(EventReceiver* eventReceiver) {
        int fd = eventReceiver->getRegisteredFd();
        pollEvents.erase(fd);
    }

    void PollFds::modOn(EventReceiver* eventReceiver, short event) {
        int fd = eventReceiver->getRegisteredFd();

        std::map<int, PollEvent>::iterator it = pollEvents.find(fd);
        if (it != pollEvents.end()) {
            PollEvent& pollEvent = it->second;

            pollfd& pollFd = pollFds[pollEvent.fds];
            pollFd.events |= event;
        }
    }

    void PollFds::modOff(EventReceiver* eventReceiver, short event) {
        int fd = eventReceiver->getRegisteredFd();

        std::map<int, PollEvent>::iterator it = pollEvents.find(fd);
        if (it != pollEvents.end()) {
            PollEvent& pollEvent = it->second;

            pollfd& pollFd = pollFds[pollEvent.fds];
            pollFd.events &= ~event;
        }
    }

    void PollFds::dispatch(short event, const utils::Timeval& currentTime) {
        for (uint32_t i = 0; i < interestCount; i++) {
            pollfd& pollFd = pollFds[i];
            if ((pollFd.revents & event) != 0) {
                pollFd.revents &= ~event;
                std::map<int, PollEvent>::iterator it = pollEvents.find(pollFd.fd);
                if (it != pollEvents.end()) {
                    if (it->second.eventReceivers.contains(event)) {
                        core::EventReceiver* eventReceiver = it->second.eventReceivers[event];
                        if (!eventReceiver->continueImmediately() && !eventReceiver->isSuspended()) {
                            it->second.eventReceivers[event]->trigger(currentTime);
                        }
                    }
                }
            }
        }
    }

    pollfd* PollFds::getEvents() {
        return pollFds.data();
    }

    nfds_t PollFds::getMaxEvents() const {
        return interestCount;
    }

    void PollFds::compress() {
        std::vector<pollfd>::iterator it = pollFds.begin();
        std::vector<pollfd>::iterator rit = pollFds.begin() + static_cast<long>(pollFds.size()) - 1;

        while (it < rit) {
            while (it != pollFds.end() && it->fd != -1) {
                ++it;
            }

            while (rit >= it && rit->fd == -1) {
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

        while (pollFds.size() > (interestCount * 2) + 1) {
            pollFds.resize(pollFds.size() / 2);
        }
    }

    void PollFds::printStats(const std::string& what) {
        std::cout << "-----------------------------------------------------------------------------" << std::endl;
        std::cout << "Current Status for: " << what << std::endl;
        std::cout << "      PollFds: Vector size = " << pollFds.size() << ", map size = " << pollEvents.size()
                  << ", interrest count = " << interestCount << std::endl;

        for (auto& [fd, pollEvent] : this->pollEvents) {
            std::map<int, PollEvent>::iterator it = pollEvents.find(fd);

            short events = 0;
            short rEvents = 0;
            if (it != pollEvents.end()) {
                events = pollFds[it->second.fds].events;
                rEvents = pollFds[it->second.fds].revents;
            } else {
                events = -1;
                rEvents = -1;
            }

            std::string sEvents = "[";
            if ((events & POLLIN) != 0) {
                sEvents += "POLLIN";
            }
            if ((events & POLLOUT) != 0) {
                sEvents += "|POLOUT";
            }
            if ((events & POLLPRI) != 0) {
                sEvents += "|POLLPRI";
            }
            if (events == -1) {
                sEvents += "xxx";
            }
            sEvents += "]";

            std::string rSEvents = "[";
            if ((rEvents & POLLIN) != 0) {
                rSEvents += "POLLIN";
            }
            if ((rEvents & POLLOUT) != 0) {
                rSEvents += "|POLLOUT";
            }
            if ((rEvents & POLLPRI) != 0) {
                rSEvents += "|POLLPRI";
            }
            if (rEvents == -1) {
                rSEvents += "xxx";
            }
            rSEvents += "]";

            std::cout << "    PollEvent Structure for fd = " << fd << ", events = " << sEvents << ", revents = " << rSEvents
                      << ", size = " << pollEvents.size() << std::endl;

            for (auto& [event, eventReceiver] : pollEvent.eventReceivers) {
                std::cout << "        Event = "
                          << ((event == POLLIN)    ? "POLLIN"
                              : (event == POLLOUT) ? "POLLOUT"
                                                   : "POLLPRI")
                          << ", EventReceiver = " << eventReceiver << std::endl;
            }
        }
        std::cout << "-----------------------------------------------------------------------------" << std::endl;
    }

    core::TimerEventDispatcher& EventDispatcher::getTimerEventDispatcher() {
        return timerEventDispatcher;
    }

    EventDispatcher::EventDispatcher()
        : eventDispatcher{core::poll::DescriptorEventDispatcher(pollFds, POLLIN),
                          core::poll::DescriptorEventDispatcher(pollFds, POLLOUT),
                          core::poll::DescriptorEventDispatcher(pollFds, POLLPRI)} {
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
        //        pollFds.printStats("Observed");
    }

    void EventDispatcher::dispatchActiveEvents(int count, const utils::Timeval& currentTime) {
        //        pollFds.printStats("Dispatchd");

        if (count > 0) {
            for (core::poll::DescriptorEventDispatcher& ed : eventDispatcher) {
                ed.dispatchActiveEvents(currentTime);
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

            int ret = ::poll(pollFds.getEvents(), pollFds.getMaxEvents(), nextTimeout.ms());

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

} // namespace core::poll
