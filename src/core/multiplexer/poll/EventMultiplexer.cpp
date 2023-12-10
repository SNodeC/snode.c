/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021, 2022, 2023 Volker Christian <me@vchrist.at>
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

#include "core/multiplexer/poll/EventMultiplexer.h"

#include "core/multiplexer/poll/DescriptorEventPublisher.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"
#include "utils/Timeval.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <memory>
#include <utility>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

core::EventMultiplexer& EventMultiplexer() {
    static core::poll::EventMultiplexer eventMultiplexer;

    return eventMultiplexer;
}

namespace core::poll {

    PollFdsManager::PollFdsManager() {
        pollfds.resize(1, {-1, 0, 0});
        pollFdIndices.reserve(1);
    }

    void PollFdsManager::muxAdd(core::DescriptorEventReceiver* eventReceiver, short event) {
        const int fd = eventReceiver->getRegisteredFd();

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

    void PollFdsManager::muxDel(int fd, short event) {
        const std::unordered_map<int, PollFdIndex>::iterator itPollFdIndex = pollFdIndices.find(fd);

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

    void PollFdsManager::muxOn(core::DescriptorEventReceiver* eventReceiver, short event) {
        const int fd = eventReceiver->getRegisteredFd();

        pollfds[pollFdIndices.find(fd)->second.index].events |= event;
    }

    void PollFdsManager::muxOff(DescriptorEventReceiver* eventReceiver, short event) {
        pollfds[pollFdIndices.find(eventReceiver->getRegisteredFd())->second.index].events &=
            static_cast<short>(~event); // Tilde promotes to int
    }

    void PollFdsManager::compress() {
        (void) std::remove_if(pollfds.begin(), pollfds.end(), [](const pollfd& pollFd) -> bool {
            return pollFd.fd < 0;
        });

        pollfds.resize(pollFdIndices.size() + 1, {-1, 0, 0});

        pollFdIndices.reserve(pollFdIndices.size() + 1);

        for (uint32_t i = 0; i < pollFdIndices.size(); i++) {
            pollFdIndices[pollfds[i].fd].index = i;
        }

        nextIndex = pollFdIndices.size();
    }

    pollfd* PollFdsManager::getEvents() {
        return pollfds.data();
    }

    const std::unordered_map<int, PollFdsManager::PollFdIndex>& PollFdsManager::getPollFdIndices() const {
        return pollFdIndices;
    }

    nfds_t PollFdsManager::getCurrentSize() const {
        return nextIndex;
    }

    EventMultiplexer::EventMultiplexer()
        : core::EventMultiplexer(
              new core::poll::DescriptorEventPublisher("READ", pollFdsManager, POLLIN, POLLIN | POLLHUP | POLLRDHUP | POLLERR),
              new core::poll::DescriptorEventPublisher("WRITE", pollFdsManager, POLLOUT, POLLOUT),
              new core::poll::DescriptorEventPublisher("EXCEPT", pollFdsManager, POLLPRI, POLLPRI)) {
        LOG(DEBUG) << "Core::multiplexer: poll";
    }

    int EventMultiplexer::monitorDescriptors(utils::Timeval& tickTimeOut, const sigset_t& sigMask) {
        const timespec timeSpec = tickTimeOut.getTimespec();

        return core::system::ppoll(pollFdsManager.getEvents(), pollFdsManager.getCurrentSize(), &timeSpec, &sigMask);
    }

    void EventMultiplexer::spanActiveEvents(int activeDescriptorCount) {
        if (activeDescriptorCount > 0) {
            for (core::DescriptorEventPublisher* const descriptorEventPublisher : descriptorEventPublishers) {
                descriptorEventPublisher->spanActiveEvents();
            }
        }
    }

} // namespace core::poll
