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

#include "core/mux/poll/DescriptorEventDispatcher.h"

#include "core/DescriptorEventReceiver.h"
#include "core/mux/poll/EventDispatcher.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <unordered_map> // for unordered_map, _Map_base<>::mapped_type
#include <utility>       // for tuple_element<>::type

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::poll {

    DescriptorEventDispatcher::DescriptorEventDispatcher(PollFds& pollFds, short events, short revents)
        : pollFds(pollFds)
        , events(events)
        , revents(revents) {
    }

    void DescriptorEventDispatcher::modAdd(core::DescriptorEventReceiver* eventReceiver) {
        pollFds.modAdd(eventReceiver, events);
    }

    void DescriptorEventDispatcher::modDel(core::DescriptorEventReceiver* eventReceiver) {
        pollFds.modDel(eventReceiver, events);
    }

    void DescriptorEventDispatcher::modOn(core::DescriptorEventReceiver* eventReceiver) {
        pollFds.modOn(eventReceiver, events);
    }

    void DescriptorEventDispatcher::modOff(core::DescriptorEventReceiver* eventReceiver) {
        pollFds.modOff(eventReceiver, events);
    }

    void DescriptorEventDispatcher::dispatchActiveEvents() {
        pollfd* pollfds = pollFds.getEvents();

        std::unordered_map<int, PollFds::PollFdIndex> pollFdsIndices = pollFds.getPollFdIndices();

        for (auto& [fd, eventReceivers] : observedEventReceiver) { // cppcheck-suppress unassignedVariable
            pollfd& pollFd = pollfds[pollFdsIndices[fd].index];

            if ((pollFd.events & events) != 0 && (pollFd.revents & this->revents) != 0) {
                core::DescriptorEventReceiver* eventReceiver = eventReceivers.front();
                if (!eventReceiver->isSuspended()) {
                    eventCounter++;
                    eventReceiver->publish();
                }
            }
        }
    }

    void core::poll::DescriptorEventDispatcher::finishTick() {
        pollFds.compress();
    }

} // namespace core::poll
