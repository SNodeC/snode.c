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
#include "core/poll/DescriptorEventDispatcher.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "utils/Timeval.h" // IWYU pragma: keep

#include <compare> // for operator<, __synth3way_t, operator>=
#include <memory>  // for allocator_traits<>::value_type
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

    void PollFds::add(EventReceiver* eventReceiver, short event) {
        int fd = eventReceiver->getRegisteredFd();

        std::unordered_map<int, PollFdIndex>::iterator itPollFdIndex = pollFdIndices.find(fd);

        if (itPollFdIndex == pollFdIndices.end()) {
            pollfds[interestCount].events = event;
            pollfds[interestCount].fd = fd;

            pollFdIndices[fd].index = interestCount;
            pollFdIndices[fd].events = event;

            interestCount++;

            if (interestCount == pollfds.size()) {
                pollfd pollFd;

                pollFd.fd = -1;
                pollFd.events = 0;
                pollFd.revents = 0;

                pollfds.resize(pollfds.size() * 2, pollFd);
            }
        } else {
            pollfds[itPollFdIndex->second.index].events |= event;

            itPollFdIndex->second.events |= event;
        }
    }

    void PollFds::del(EventReceiver* eventReceiver, short event) {
        int fd = eventReceiver->getRegisteredFd();

        std::unordered_map<int, PollFdIndex>::iterator itPollFdIndex = pollFdIndices.find(fd);

        if (itPollFdIndex != pollFdIndices.end()) {
            unsigned long index = itPollFdIndex->second.index;

            pollfds[index].events &= static_cast<short>(~event); // tilde promotes to int

            itPollFdIndex->second.events &= static_cast<short>(~event); // tilde promotes to int

            if (itPollFdIndex->second.events == 0) {
                pollfds[index].fd = -1; // Compress will keep track of that descriptor

                pollFdIndices.erase(itPollFdIndex);

                interestCount--;
            }
        }
    }

    void PollFds::modOn(EventReceiver* eventReceiver, short event) {
        int fd = eventReceiver->getRegisteredFd();

        std::unordered_map<int, PollFdIndex>::iterator itPollFdIndex = pollFdIndices.find(fd);

        if (itPollFdIndex != pollFdIndices.end()) {
            pollfds[itPollFdIndex->second.index].events |= event;
        }
    }

    void PollFds::modOff(EventReceiver* eventReceiver, short event) {
        int fd = eventReceiver->getRegisteredFd();

        std::unordered_map<int, PollFdIndex>::iterator itPollFdIndex = pollFdIndices.find(fd);

        if (itPollFdIndex != pollFdIndices.end()) {
            pollfd& pollFd = pollfds[itPollFdIndex->second.index];

            pollFd.events &= static_cast<short>(~event);
            pollFd.revents = 0;
        }
    }

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
        : core::EventDispatcher(new core::poll::DescriptorEventDispatcher(pollFds, POLLIN, POLLIN | POLLHUP | POLLRDHUP | POLLERR),
                                new core::poll::DescriptorEventDispatcher(pollFds, POLLOUT, POLLOUT),
                                new core::poll::DescriptorEventDispatcher(pollFds, POLLPRI, POLLPRI)) {
    }

    int EventDispatcher::multiplex(utils::Timeval& tickTimeOut) {
        return ::poll(pollFds.getEvents(), pollFds.getInterestCount(), tickTimeOut.ms());
    }

    void EventDispatcher::dispatchActiveEvents(int count, const utils::Timeval& currentTime) {
        if (count > 0) {
            for (core::DescriptorEventDispatcher* const eventDispatcher : descriptorEventDispatcher) {
                eventDispatcher->dispatchActiveEvents(currentTime);
            }
        }

        for (core::DescriptorEventDispatcher* const eventDispatcher : descriptorEventDispatcher) {
            eventDispatcher->dispatchImmediateEvents(currentTime);
        }
    }

} // namespace core::poll
