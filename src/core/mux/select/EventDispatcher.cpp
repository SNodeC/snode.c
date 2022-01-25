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

#include "core/mux/select/EventDispatcher.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "core/system/select.h"
#include "utils/Timeval.h" // for Timeval

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

core::EventDispatcher& EventDispatcher() {
    static core::select::EventDispatcher eventDispatcher;

    return eventDispatcher;
}

namespace core::select {

    EventDispatcher::EventDispatcher()
        : core::EventDispatcher(new core::select::DescriptorEventDispatcher(fdSets[RD]),
                                new core::select::DescriptorEventDispatcher(fdSets[WR]),
                                new core::select::DescriptorEventDispatcher(fdSets[EX])) {
    }

    int EventDispatcher::multiplex(utils::Timeval& tickTimeOut) {
        return core::system::select(getMaxFd() + 1, &fdSets[RD].get(), &fdSets[WR].get(), &fdSets[EX].get(), &tickTimeOut);
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

} // namespace core::select
