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

#include "DescriptorEventPublisher.h"

#include "EventMultiplexer.h"
#include "core/DescriptorEventReceiver.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#include <unordered_map> // for unordered_map, _Map_base<>::mapped_type
#include <utility>       // for tuple_element<>::type

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

    void DescriptorEventPublisher::muxOff(int fd) {
        pollFds.muxOff(fd, events);
    }

    int DescriptorEventPublisher::publishActiveEvents() {
        int count = 0;

        pollfd* pollfds = pollFds.getEvents();

        const std::unordered_map<int, PollFdsManager::PollFdIndex>& pollFdsIndices = pollFds.getPollFdIndices();

        for (auto& [fd, eventReceivers] : observedEventReceivers) { // cppcheck-suppress unassignedVariable
            pollfd& pollFd = pollfds[pollFdsIndices.find(fd)->second.index];

            if ((pollFd.events & events) != 0 && (pollFd.revents & revents) != 0) {
                core::DescriptorEventReceiver* eventReceiver = eventReceivers.front();
                VLOG(0) << "** DEP: Publish to " << eventReceiver->getName() << " -- fd = " << eventReceiver->getRegisteredFd();
                eventCounter++;
                eventReceiver->publish();
                count++;
            } else {
                VLOG(0) << "** DEP: Not published: condition not fullfilled -- fd = " << eventReceivers.front()->getRegisteredFd();
            }
        }

        return count;
    }

} // namespace core::poll
