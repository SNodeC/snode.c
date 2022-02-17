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
#include <cstdint>
#include <memory>  // for allocator_traits<>::value_type
#include <utility> // for pair

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

core::EventMultiplexer& EventDispatcher() {
    static core::poll::EventMultiplexer eventMultiplexer;

    return eventMultiplexer;
}

namespace core::poll {

    PollFds::PollFds() {
        pollfd pollFd;

        pollFd.fd = -1;
        pollFd.events = 0;
        pollFd.revents = 0;

        pollfds.resize(1, pollFd);
        pollFdIndices.reserve(1);
    }

    void PollFds::muxAdd(core::DescriptorEventReceiver* eventReceiver, short event) {
        int fd = eventReceiver->getRegisteredFd();

        if (!pollFdIndices.contains(fd)) {
            pollfds[nextIndex].events = event;
            pollfds[nextIndex].fd = fd;

            pollFdIndices[fd].index = nextIndex;
            pollFdIndices[fd].events = event;

            ++nextIndex;

            if (nextIndex == pollfds.size()) {
                pollfds.resize(pollfds.size() * 2, {-1, 0, 0});
                pollFdIndices.reserve(pollfds.size());
            }
        } else {
            PollFdIndex& pollFdIndex = pollFdIndices[fd];

            pollfds[pollFdIndex.index].events |= event;
            pollFdIndex.events |= event;
        }
    }

    void PollFds::muxDel(int fd, short event) {
        std::unordered_map<int, PollFdIndex>::iterator itPollFdIndex = pollFdIndices.find(fd);

        PollFdIndex& pollFdIndex = itPollFdIndex->second;

        pollfds[pollFdIndex.index].events &= static_cast<short>(~event); // tilde promotes to int
        pollFdIndex.events &= static_cast<short>(~event);                // tilde promotes to int

        if (pollFdIndex.events == 0) {
            pollfds[pollFdIndex.index].fd = -1; // Compress will keep track of that descriptor
            pollFdIndices.erase(fd);

            if (pollfds.size() > (pollFdIndices.size() * 2) + 1) {
                compress();
            }
        }
    }

    void PollFds::muxOn(core::DescriptorEventReceiver* eventReceiver, short event) {
        int fd = eventReceiver->getRegisteredFd();

        pollfds[pollFdIndices.find(fd)->second.index].events |= event;
    }

    void PollFds::muxOff(int fd, short event) {
        pollfds[pollFdIndices.find(fd)->second.index].events &= static_cast<short>(~event); // Tilde promotes to int
    }

    void PollFds::compress() {
        remove_if(pollfds.begin(), pollfds.end(), [](const pollfd& pollFd) -> bool {
            return pollFd.fd < 0;
        });

        pollfds.resize(pollFdIndices.size() + 1, {-1, 0, 0});

        pollFdIndices.reserve(pollFdIndices.size() + 1);

        for (uint32_t i = 0; i < pollFdIndices.size(); i++) {
            if (pollfds[i].fd >= 0) {
                pollFdIndices[pollfds[i].fd].index = i;
            }
        }

        nextIndex = pollFdIndices.size();
    }

    pollfd* PollFds::getEvents() {
        return pollfds.data();
    }

    const std::unordered_map<int, PollFds::PollFdIndex>& PollFds::getPollFdIndices() const {
        return pollFdIndices;
    }

    nfds_t PollFds::getCurrentIndex() const {
        return nextIndex;
    }

    EventMultiplexer::EventMultiplexer()
        : core::EventMultiplexer(new core::poll::DescriptorEventPublisher(pollFds, POLLIN, POLLIN | POLLHUP | POLLRDHUP | POLLERR),
                                 new core::poll::DescriptorEventPublisher(pollFds, POLLOUT, POLLOUT),
                                 new core::poll::DescriptorEventPublisher(pollFds, POLLPRI, POLLPRI)) {
    }

    int EventMultiplexer::multiplex(utils::Timeval& tickTimeOut) {
        return core::system::poll(pollFds.getEvents(), pollFds.getCurrentIndex(), tickTimeOut.ms());
    }

    void EventMultiplexer::dispatchActiveEvents(int count) {
        if (count > 0) {
            for (core::DescriptorEventPublisher* const descriptorEventPublisher : descriptorEventPublisher) {
                descriptorEventPublisher->dispatchActiveEvents();
            }
        }
    }

} // namespace core::poll
