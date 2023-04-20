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

#include "core/multiplexer/poll/DescriptorEventPublisher.h"

#include "core/multiplexer/poll/EventMultiplexer.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#include <unordered_map>
#include <utility>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::poll {

    DescriptorEventPublisher::DescriptorEventPublisher(const std::string& name, PollFdsManager& pollFds, short events, short revents)
        : core::DescriptorEventPublisher(name)
        , pollFds(pollFds)
        , events(events)
        , revents(revents) {
    }

    void DescriptorEventPublisher::muxAdd(core::DescriptorEventReceiver* eventReceiver) {
        pollFds.muxAdd(eventReceiver, events);
    }

    void DescriptorEventPublisher::muxDel(int fd) {
        pollFds.muxDel(fd, events);
    }

    void DescriptorEventPublisher::muxOn(core::DescriptorEventReceiver* eventReceiver) {
        pollFds.muxOn(eventReceiver, events);
    }

    void DescriptorEventPublisher::muxOff(core::DescriptorEventReceiver* eventReceiver) {
        pollFds.muxOff(eventReceiver, events);
    }

    int DescriptorEventPublisher::spanActiveEvents() {
        int count = 0;

        pollfd* pollfds = pollFds.getEvents();

        const std::unordered_map<int, PollFdsManager::PollFdIndex>& pollFdsIndices = pollFds.getPollFdIndices();

        for (auto& [fd, eventReceivers] : observedEventReceivers) {
            pollfd& pollFd = pollfds[pollFdsIndices.find(fd)->second.index];

            if ((pollFd.events == events) != 0 && (pollFd.revents & revents) != 0) {
                LOG(TRACE) << "POLL " << getName() << " DEP fired";
                core::DescriptorEventReceiver* eventReceiver = eventReceivers.front();
                eventCounter++;
                eventReceiver->span();
                count++;
            }
        }

        return count;
    }

} // namespace core::poll
