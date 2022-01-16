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

#include "core/poll/DescriptorEventDispatcher.h"

#include "core/EventReceiver.h"
#include "core/poll/EventDispatcher.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <utility> // for tuple_element<>::type

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::poll {

    DescriptorEventDispatcher::DescriptorEventDispatcher(PollFds& pollFds, short events)
        : pollFds(pollFds)
        , events(events) {
    }

    void DescriptorEventDispatcher::modAdd(EventReceiver* eventReceiver) {
        pollFds.add(eventReceiver, events);
    }

    void DescriptorEventDispatcher::modDel(EventReceiver* eventReceiver) {
        pollFds.del(eventReceiver, events);
    }

    void DescriptorEventDispatcher::modOn(EventReceiver* eventReceiver) {
        pollFds.modOn(eventReceiver, events);
    }

    void DescriptorEventDispatcher::modOff(EventReceiver* eventReceiver) {
        pollFds.modOff(eventReceiver, events);
    }

    int DescriptorEventDispatcher::getInterestCount() const {
        return static_cast<int>(observedEventReceiver.size());
    }

    void DescriptorEventDispatcher::unobserveDisabledEvents(const utils::Timeval& currentTime) {
        bool doCompress = false;

        for (const auto& [fd, eventReceivers] : disabledEventReceiver) {
            for (core::EventReceiver* eventReceiver : eventReceivers) {
                observedEventReceiver[fd].remove(eventReceiver);
                if (observedEventReceiver[fd].empty()) {
                    modDel(eventReceiver);
                    observedEventReceiver.erase(fd);
                } else if (!observedEventReceiver[fd].front()->isSuspended()) {
                    modOn(observedEventReceiver[fd].front());
                    observedEventReceiver[fd].front()->triggered(currentTime);
                } else {
                    modOff(observedEventReceiver[fd].front());
                }
                eventReceiver->disabled();
                if (eventReceiver->getObservationCounter() == 0) {
                    eventReceiver->unobservedEvent();
                    doCompress = true;
                }
            }
        }

        disabledEventReceiver.clear();

        if (doCompress) {
            pollFds.compress();
        }
    }

} // namespace core::poll
