/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025
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

/*
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "core/multiplexer/poll/EventMultiplexer.h"

#include "core/DescriptorEventReceiver.h"
#include "core/multiplexer/poll/DescriptorEventPublisher.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"
#include "utils/Timeval.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <ctime>
#include <utility>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

core::EventMultiplexer& EventMultiplexer() {
    static core::multiplexer::poll::EventMultiplexer eventMultiplexer;

    return eventMultiplexer;
}

namespace core::multiplexer::poll {

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

    void PollFdsManager::muxOn(const DescriptorEventReceiver* eventReceiver, short event) {
        pollfds[pollFdIndices.find(eventReceiver->getRegisteredFd())->second.index].events |= event;
    }

    void PollFdsManager::muxOff(const DescriptorEventReceiver* eventReceiver, short event) {
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
        : core::EventMultiplexer(new core::multiplexer::poll::DescriptorEventPublisher("READ", //
                                                                                       pollFdsManager,
                                                                                       POLLIN,
                                                                                       POLLIN | POLLHUP | POLLRDHUP | POLLERR),
                                 new core::multiplexer::poll::DescriptorEventPublisher("WRITE", //
                                                                                       pollFdsManager,
                                                                                       POLLOUT,
                                                                                       POLLOUT),
                                 new core::multiplexer::poll::DescriptorEventPublisher("EXCEPT", //
                                                                                       pollFdsManager,
                                                                                       POLLPRI,
                                                                                       POLLPRI)) {
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

} // namespace core::multiplexer::poll
