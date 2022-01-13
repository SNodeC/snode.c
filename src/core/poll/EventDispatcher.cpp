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

#ifdef NOBLOCK

/* Event types that can be polled for.  These bits may be set in `events'
   to indicate the interesting event types; they will appear in `revents'
   to indicate the status of the file descriptor.  */
#define POLLIN 0x001  /* There is data to read.  */
#define POLLPRI 0x002 /* There is urgent data to read.  */
#define POLLOUT 0x004 /* Writing now will not block.  */

#if defined __USE_XOPEN || defined __USE_XOPEN2K8
/* These values are defined in XPG4.2.  */
#define POLLRDNORM 0x040 /* Normal data may be read.  */
#define POLLRDBAND 0x080 /* Priority data may be read.  */
#define POLLWRNORM 0x100 /* Writing now will not block.  */
#define POLLWRBAND 0x200 /* Priority data may be written.  */
#endif

#ifdef __USE_GNU
/* These are extensions for Linux.  */
#define POLLMSG 0x400
#define POLLREMOVE 0x1000
#define POLLRDHUP 0x2000
#endif

/* Event types always implicitly polled for.  These bits need not be set in
   `events', but they will appear in `revents' to indicate the status of
   the file descriptor.  */
#define POLLERR 0x008  /* Error condition.  */
#define POLLHUP 0x010  /* Hung up.  */
#define POLLNVAL 0x020 /* Invalid polling request.  */

#endif

namespace core::poll {

    PollFds::PollEvent::PollEvent(pollfds_size_type fds)
        : fds(fds) {
    }

    void PollFds::PollEvent::set(short event, EventReceiver* eventReceiver) {
        const auto [it, success] = eventReceivers.insert({event, eventReceiver});

        if (!success) {
            it->second = eventReceiver;
        }
    }

    PollFds::PollFds()
        : interestCount(0) {
        pollfd pollFd;

        pollFd.fd = -1;
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
            pollEvent.set(event, eventReceiver);

            pollEvents.insert({fd, pollEvent});
            interestCount++;

            if (interestCount >= pollFds.size()) {
                pollfd pollFd;

                pollFd.fd = -1;
                pollFd.events = 0;
                pollFd.revents = 0;

                pollFds.resize(pollFds.size() * 2, {-1, 0, 0});
            }

        } else {
            PollEvent& pollEvent = it->second;

            pollEvent.set(event, eventReceiver);

            VLOG(0) << "Event registered: fd = " << fd << ", fds = " << pollEvent.fds << ", event = " << std::hex << event << std::dec;

            pollFds[pollEvent.fds].events |= event;
            pollFds[pollEvent.fds].fd = fd;
        }
    }

    void PollFds::del(EventReceiver* eventReceiver, short event) {
        int fd = eventReceiver->getRegisteredFd();

        VLOG(0) << "Call del fd = " << fd << ", event = " << std::hex << event << std::dec << std::endl;

        std::map<int, PollEvent>::iterator it = pollEvents.find(fd);
        if (it != pollEvents.end()) {
            PollEvent& pollEvent = it->second;

            pollfd& pollFd = pollFds[pollEvent.fds];
            pollFd.events &= ~event;
            pollFd.fd = fd;

            //            if (pollFd.events == 0) {
            //                pollFd.fd = -1;
            //                interestCount--;
            //            }
        }
    }

    void PollFds::finish(EventReceiver* eventReceiver) {
        int fd = eventReceiver->getRegisteredFd();

        std::map<int, PollEvent>::iterator it = pollEvents.find(fd);
        if (it != pollEvents.end()) {
            PollEvent& pollEvent = it->second;

            pollfd& pollFd = pollFds[pollEvent.fds];

            std::cout << "Calling finished: pollFd.fd = " << pollFd.fd << " fds = " << pollEvent.fds << std::endl;

            if (pollFd.fd != fd && pollFd.fd != -2 && pollFd.fd != -1) {
                exit(0);
            }

            pollFd.fd = -1;
            interestCount--;
        }

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
            if (pollFd.events == 0) {
                //                pollFd.fd = -2;
            }
            // pollFd.revents &= ~event;
            pollFd.revents = 0;
        }
    }

    void PollFds::dispatch(const utils::Timeval& currentTime) {
        for (uint32_t i = 0; i < interestCount; i++) {
            pollfd& pollFd = pollFds[i];
            short revents = pollFd.revents;

            if (pollFd.revents != 0) {
                std::map<int, PollEvent>::iterator it = pollEvents.find(pollFd.fd);
                PollEvent& pollEvent = it->second;

                /*
                if ((revents & (POLLHUP | POLLRDHUP | POLLERR | POLLNVAL)) != 0) {
                    switch (revents) {
                        case POLLHUP:
                        case POLLRDHUP:
                        case POLLERR:
                            pollEvent.eventReceivers[POLLIN]->trigger(currentTime);
                            break;
                        case POLLNVAL:
                            PLOG(ERROR) << "This should never happen";
                            break;
                    }
                } else {
                */
                if ((revents & (POLLIN | POLLHUP | POLLRDHUP | POLLERR)) != 0) {
                    pollEvent.eventReceivers[POLLIN]->trigger(currentTime);
                }
                if ((revents & POLLOUT) != 0) {
                    pollEvent.eventReceivers[POLLOUT]->trigger(currentTime);
                }
                if ((revents & POLLPRI) != 0) {
                    pollEvent.eventReceivers[POLLPRI]->trigger(currentTime);
                }
                if ((revents & POLLNVAL) != 0) {
                    PLOG(ERROR) << "This should never happen fd = " << pollFd.fd << ", revents = " << std::hex << revents << std::dec
                                << std::endl;
                    exit(0);
                }
                //            }
            }
        }
    }

    void PollFds::dispatch(short event, const utils::Timeval& currentTime) {
        VLOG(0) << "===========================================";
        for (uint32_t i = 0; i < interestCount; i++) {
            pollfd& pollFd = pollFds[i];

            VLOG(0) << "********* 1 : fd = " << pollFd.fd << ", events = " << pollFd.revents << ", pevents = " << event;

            if ((pollFd.revents & (event | POLLNVAL)) != 0) {
                VLOG(0) << "********* 2 : " << pollFd.events;
                pollFd.revents &= ~event;
                std::map<int, PollEvent>::iterator it = pollEvents.find(pollFd.fd);
                if (it != pollEvents.end()) {
                    VLOG(0) << "********* 3 : " << pollFd.events;
                    if (it->second.eventReceivers.contains(event)) {
                        core::EventReceiver* eventReceiver = it->second.eventReceivers[event];
                        VLOG(0) << "********* 4 : " << pollFd.events << ", immediately: " << eventReceiver->continueImmediately()
                                << ", suspended: " << eventReceiver->isSuspended();
                        if (!eventReceiver->continueImmediately() && !eventReceiver->isSuspended()) {
                            VLOG(0) << "********* 5 : " << pollFd.events;
                            it->second.eventReceivers[event]->trigger(currentTime);
                        }
                    } else {
                        VLOG(0) << "Event not found";
                        exit(0);
                    }
                } else {
                    VLOG(0) << "Fd not found";
                    exit(0);
                }
            } else {
                VLOG(0) << "Not my event";
            }
        }
        VLOG(0) << "===========================================";
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

        for (uint32_t i = 0; i < interestCount; i++) {
            VLOG(0) << "Compress: fd = " << pollFds[i].fd << ", bevore fds = " << pollEvents.find(pollFds[i].fd)->second.fds
                    << ", new fds = " << i;

            if (pollFds[i].fd >= 0) {
                pollEvents.find(pollFds[i].fd)->second.fds = i;
            } else {
                exit(0);
            }
        }
    }

    void PollFds::printStats(const std::string& what) {
        std::cout << "-----------------------------------------------------------------------------" << std::endl;
        std::cout << "Current Status for: " << what << std::endl;
        std::cout << "  PollFds: Vector size = " << pollFds.size() << ", map size = " << pollEvents.size()
                  << ", interrest count = " << interestCount << std::endl;

        std::string s = "  PollFds: Fd = ";
        for (auto& pollfd : pollFds) {
            s += std::to_string(pollfd.fd) + ", ";
        }
        std::cout << s << std::endl;

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

            std::cout << "    PollEvent Structure for fd = " << fd << ", events = " << sEvents << ", revents = " << rSEvents << " ("
                      << std::hex << rEvents << std::dec << ")" << std::dec << std::endl;

            for (auto& [event, eventReceiver] : pollEvent.eventReceivers) {
                std::cout << "        Event = "
                          << ((event == POLLIN)    ? "POLLIN"
                              : (event == POLLOUT) ? "POLLOUT"
                                                   : "POLLPRI")
                          << ", EventReceiver = " << eventReceiver << std::endl;
            }

            if ((events & rEvents) == 0 && events == 0 && rEvents != 0) {
                std::cout << "        Error: Revents not in Event" << std::endl;
                exit(0);
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
        pollFds.printStats("Bevore observed");

        for (DescriptorEventDispatcher& eventDispatcher : eventDispatcher) {
            eventDispatcher.observeEnabledEvents();
        }

        pollFds.printStats("After observed");
    }

    void EventDispatcher::dispatchActiveEvents(int count, const utils::Timeval& currentTime) {
        pollFds.printStats("Bevore dispatchd");

        if (count > 0) {
            pollFds.dispatch(currentTime);
            /*for (core::poll::DescriptorEventDispatcher& ed : eventDispatcher) {
                ed.dispatchActiveEvents(currentTime);
            }
            */
        }

        pollFds.printStats("Midd dispatchd");

        for (DescriptorEventDispatcher& eventDispatcher : eventDispatcher) {
            eventDispatcher.dispatchImmediateEvents(currentTime);
        }

        pollFds.printStats("After dispatchd");
    }

    void EventDispatcher::unobserveDisabledEvents(const utils::Timeval& currentTime) {
        pollFds.printStats("Bevore unobserve");

        for (DescriptorEventDispatcher& eventDispatcher : eventDispatcher) {
            eventDispatcher.unobserveDisabledEvents(currentTime);
        }

        pollFds.printStats("After unobserve");
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
