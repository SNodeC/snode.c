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

#include "EventMultiplexer.h"

#include "DescriptorEventPublisher.h"
#include "core/DescriptorEventReceiver.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "utils/Timeval.h" // IWYU pragma: keep

#include <compare> // for operator<, __synth3way_t, operator>=
#include <memory>  // for allocator_traits<>::value_type
#include <utility> // for pair

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

core::EventMultiplexer& EventDispatcher() {
    static core::poll::EventMultiplexer eventMultiplexer;

    return eventMultiplexer;
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

    void PollFds::modAdd(core::DescriptorEventReceiver* eventReceiver, short event) {
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
            PollFdIndex& pollFdIndex = itPollFdIndex->second;

            pollfds[pollFdIndex.index].events |= event;
            pollFdIndex.events |= event;
        }
    }

    void PollFds::modDel(core::DescriptorEventReceiver* eventReceiver, short event) {
        int fd = eventReceiver->getRegisteredFd();

        std::unordered_map<int, PollFdIndex>::iterator itPollFdIndex = pollFdIndices.find(fd);

        PollFdIndex& pollFdIndex = itPollFdIndex->second;

        pollfds[pollFdIndex.index].events &= static_cast<short>(~event); // tilde promotes to int
        pollFdIndex.events &= static_cast<short>(~event);                // tilde promotes to int

        if (pollFdIndex.events == 0) {
            pollfds[pollFdIndex.index].fd = -1; // Compress will keep track of that descriptor

            pollFdIndices.erase(itPollFdIndex);

            interestCount--;

            //            if (pollfds.size() > (interestCount * 2) + 1) { // Do not know why this is not working
            compress();
            //            }
        }
    }

    void PollFds::modOn(core::DescriptorEventReceiver* eventReceiver, short event) {
        int fd = eventReceiver->getRegisteredFd();

        pollfds[pollFdIndices.find(fd)->second.index].events |= event;
    }

    void PollFds::modOff(core::DescriptorEventReceiver* eventReceiver, short event) {
        int fd = eventReceiver->getRegisteredFd();

        pollfds[pollFdIndices.find(fd)->second.index].events &= static_cast<short>(~event); // Tilde promotes to int
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

    const std::unordered_map<int, PollFds::PollFdIndex>& PollFds::getPollFdIndices() const {
        return pollFdIndices;
    }

    nfds_t PollFds::getInterestCount() const {
        return interestCount;
    }

    nfds_t PollFds::getSize() const {
        return pollfds.size();
    }

    EventMultiplexer::EventMultiplexer()
        : core::EventMultiplexer(new core::poll::DescriptorEventPublisher(pollFds, POLLIN, POLLIN | POLLHUP | POLLRDHUP | POLLERR),
                                 new core::poll::DescriptorEventPublisher(pollFds, POLLOUT, POLLOUT),
                                 new core::poll::DescriptorEventPublisher(pollFds, POLLPRI, POLLPRI)) {
    }

    int EventMultiplexer::multiplex(utils::Timeval& tickTimeOut) {
        return core::system::poll(pollFds.getEvents(), pollFds.getSize(), tickTimeOut.ms());
    }

    void EventMultiplexer::dispatchActiveEvents(int count) {
        if (count > 0) {
            for (core::DescriptorEventPublisher* const eventMultiplexer : descriptorEventPublisher) {
                eventMultiplexer->dispatchActiveEvents();
            }
        }
    }

} // namespace core::poll
