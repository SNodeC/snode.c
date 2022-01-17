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

#include "core/poll/EventDispatcher.h"

#include "core/EventReceiver.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <algorithm> // for min, max
#include <cerrno>
#include <compare>  // for operator<, __synth3way_t, operator>=
#include <iterator> // for distance
#include <memory>   // for allocator_traits<>::value_type
#include <numeric>
#include <utility> // for pair

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

core::EventDispatcher& EventDispatcher() {
    static core::poll::EventDispatcher eventDispatcher;

    return eventDispatcher;
}

namespace core::poll {

    PollFds::PollFds()
        : interestCount(0) {
        pollfd pollFd;

        pollFd.fd = -1;
        pollFd.events = 0;
        pollFd.revents = 0;

        pollfds.resize(1, pollFd);
    }

    void PollFds::add(std::unordered_map<int, EventReceiver*, std::hash<int>>& pollEvents, EventReceiver* eventReceiver, short event) {
        int fd = eventReceiver->getRegisteredFd();

        std::vector<pollfd>::iterator itPollFd = std::find_if(pollfds.begin(), pollfds.end(), [fd](const pollfd& pollFd) {
            return fd == pollFd.fd;
        });

        if (itPollFd == pollfds.end()) {
            pollfds[interestCount].events = event;
            pollfds[interestCount].fd = fd;

            pollEvents.insert({fd, eventReceiver});
            pollFdIndices[fd].index = interestCount;
            pollFdIndices[fd].refCount++;

            interestCount++;

            if (interestCount == pollfds.size()) {
                pollfd pollFd;

                pollFd.fd = -1;
                pollFd.events = 0;
                pollFd.revents = 0;

                pollfds.resize(pollfds.size() * 2, pollFd);
            }
        } else {
            itPollFd->events |= event;
            itPollFd->fd = fd;

            std::unordered_map<int, EventReceiver*>::iterator itPollEvent = pollEvents.find(fd);

            if (itPollEvent == pollEvents.end()) {
                pollEvents.insert({fd, eventReceiver});
                pollFdIndices[fd].index = static_cast<PollFdIndex::pollfds_size_type>(std::distance(pollfds.begin(), itPollFd));
                pollFdIndices[fd].refCount++;
            } else {
                if (itPollEvent->second == nullptr) {
                    pollFdIndices[fd].refCount++;
                }
                itPollEvent->second = eventReceiver;
            }
        }
    }

    // #define DEBUG_DEL

    void PollFds::del(std::unordered_map<int, EventReceiver*, std::hash<int>>& pollEvents, EventReceiver* eventReceiver, short event) {
        int fd = eventReceiver->getRegisteredFd();

        std::unordered_map<int, EventReceiver*>::iterator it = pollEvents.find(fd);

        if (it != pollEvents.end()) {
            pollfd& pollFd = pollfds[pollFdIndices[fd].index];
            pollFd.events &= static_cast<short>(~event); // tilde promotes to int

            it->second = nullptr;
            pollFdIndices[fd].refCount--;

            pollEvents.erase(fd);

            if (pollFdIndices[fd].refCount == 0) {
                pollFdIndices.erase(fd);
                pollFd.fd = -1; // Compress will keep track of that descriptor
                interestCount--;
            }
        }
    }

    void PollFds::modOn(std::unordered_map<int, EventReceiver*, std::hash<int>>& pollEvents, EventReceiver* eventReceiver, short event) {
        int fd = eventReceiver->getRegisteredFd();

        std::unordered_map<int, EventReceiver*>::iterator it = pollEvents.find(fd);

        if (it != pollEvents.end()) {
            it->second = eventReceiver;

            pollfd& pollFd = pollfds[pollFdIndices[fd].index];
            pollFd.events |= event;
        }
    }

    void PollFds::modOff(std::unordered_map<int, EventReceiver*, std::hash<int>>& pollEvents, EventReceiver* eventReceiver, short event) {
        int fd = eventReceiver->getRegisteredFd();

        std::unordered_map<int, EventReceiver*>::iterator it = pollEvents.find(fd);

        if (it != pollEvents.end()) {
            pollfd& pollFd = pollfds[pollFdIndices[fd].index];
            pollFd.events &= static_cast<short>(~event);
            pollFd.revents = 0;
        }
    }
    // #define DEBUG_COMPRESS

    void PollFds::compress() {
        std::vector<pollfd>::iterator it = pollfds.begin();
        std::vector<pollfd>::iterator rit = pollfds.begin() + static_cast<long>(pollfds.size()) - 1;

        while (it < rit) {
            while (it != pollfds.end() && it->fd != -1) {
                ++it;
            }

            while (rit >= it && rit->fd == -1) {
                --rit;
            }

            while (it < rit && it->fd == -1 && rit->fd != -1) {
                pollfd tPollFd = *it;
                *it = *rit;
                *rit = tPollFd;
                ++it;
                --rit;
            }
        }

        while (pollfds.size() > (interestCount * 2) + 1) {
            pollfds.resize(pollfds.size() / 2);
        }

        for (uint32_t i = 0; i < interestCount; i++) {
            if (pollfds[i].fd >= 0) {
                pollFdIndices[pollfds[i].fd].index = i;
            }
        }
    }

    pollfd* PollFds::getEvents() {
        return pollfds.data();
    }

    std::unordered_map<int, PollFdIndex>& PollFds::getPollFdIndices() {
        return pollFdIndices;
    }

    nfds_t PollFds::getInterestCount() const {
        return interestCount;
    }

    EventDispatcher::EventDispatcher()
        : eventDispatcher{core::poll::DescriptorEventDispatcher(pollFds, POLLIN, POLLIN | POLLHUP | POLLRDHUP | POLLERR),
                          core::poll::DescriptorEventDispatcher(pollFds, POLLOUT, POLLOUT),
                          core::poll::DescriptorEventDispatcher(pollFds, POLLPRI, POLLPRI)} {
    }

    core::DescriptorEventDispatcher& EventDispatcher::getDescriptorEventDispatcher(core::EventDispatcher::DISP_TYPE dispType) {
        return eventDispatcher[dispType];
    }

    core::TimerEventDispatcher& EventDispatcher::getTimerEventDispatcher() {
        return timerEventDispatcher;
    }

    int EventDispatcher::getInterestCount() {
        int receiverCount = std::accumulate(
            eventDispatcher, eventDispatcher + 3, 0, [](int count, DescriptorEventDispatcher& descriptorEventDispatcher) -> int {
                return count + descriptorEventDispatcher.getInterestCount();
            });

        return receiverCount;
    }

    utils::Timeval EventDispatcher::getNextTimeout(const utils::Timeval& currentTime) {
        utils::Timeval nextTimeout = core::EventReceiver::TIMEOUT::MAX;

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

    void EventDispatcher::dispatchActiveEvents(int count, const utils::Timeval& currentTime) {
        if (count > 0) {
            for (DescriptorEventDispatcher& eventDispatcher : eventDispatcher) {
                eventDispatcher.dispatchActiveEvents(currentTime);
            }
        }

        for (DescriptorEventDispatcher& eventDispatcher : eventDispatcher) {
            eventDispatcher.dispatchImmediateEvents(currentTime);
        }
    }

    TickStatus EventDispatcher::dispatch(const utils::Timeval& tickTimeOut, bool stopped) {
        TickStatus tickStatus = TickStatus::SUCCESS;

        EventDispatcher::observeEnabledEvents();

        int receiverCount = EventDispatcher::getInterestCount();

        utils::Timeval currentTime = utils::Timeval::currentTime();

        utils::Timeval nextEventTimeout = EventDispatcher::getNextTimeout(currentTime);
        utils::Timeval nextTimerTimeout = timerEventDispatcher.getNextTimeout(currentTime);

        if (receiverCount > 0 || (!timerEventDispatcher.empty() && !stopped)) {
            utils::Timeval nextTimeout = stopped ? nextEventTimeout : std::min(nextTimerTimeout, nextEventTimeout);

            nextTimeout = std::min(nextTimeout, tickTimeOut);
            nextTimeout = std::max(nextTimeout, utils::Timeval()); // In case nextEventTimeout is negativ

            int ret = ::poll(pollFds.getEvents(), pollFds.getInterestCount(), nextTimeout.ms());

            if (ret >= 0) {
                currentTime = utils::Timeval::currentTime();

                timerEventDispatcher.dispatchActiveEvents(currentTime);

                EventDispatcher::dispatchActiveEvents(ret, currentTime);
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

    /*
        void PollFds::printStats(const std::string& what) {
            std::cout << "-----------------------------------------------------------------------------" << std::endl;
            std::cout << "Current Status for: " << what << std::endl;
            std::cout << "  PollFds: Vector size = " << pollfds.size() << ", map size = " << pollEvents.size()
                      << ", interrest count = " << interestCount << std::endl;

            std::string pollFdsString = "  PollFds: Fd = ";
            for (auto& pollfd : pollfds) {
                pollFdsString += std::to_string(pollfd.fd) + ", ";
            }
            std::cout << pollFdsString << std::endl;

            for (auto& [fd, pollEvent] : this->pollEvents) {
                std::unordered_map<int, PollEvent>::iterator it = pollEvents.find(fd);

                short events = 0;
                short rEvents = 0;
                if (it != pollEvents.end()) {
                    events = pollfds[it->second.fds].events;
                    rEvents = pollfds[it->second.fds].revents;
                } else {
                    events = -1;
                    rEvents = -1;
                }

                std::string sEvents = "[";
                if ((events & POLLIN) != 0) {
                    sEvents += "POLLIN";
                }
                if ((events & POLLOUT) != 0) {
                    sEvents += "|POLLOUT";
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
                }
            }

            std::cout << "-----------------------------------------------------------------------------" << std::endl;
        }
    */

} // namespace core::poll
