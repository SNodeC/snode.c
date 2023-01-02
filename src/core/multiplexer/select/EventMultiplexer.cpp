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

#include "core/multiplexer/select/EventMultiplexer.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "core/system/select.h"

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

core::EventMultiplexer& EventMultiplexer() {
    static core::select::EventMultiplexer eventMultiplexer;

    return eventMultiplexer;
}

namespace core::select {

    EventMultiplexer::EventMultiplexer()
        : core::EventMultiplexer(
              new core::select::DescriptorEventPublisher("READ", fdSets[core::DescriptorEventReceiver::DISP_TYPE::RD]),
              new core::select::DescriptorEventPublisher("WRITE", fdSets[core::DescriptorEventReceiver::DISP_TYPE::WR]),
              new core::select::DescriptorEventPublisher("EXCEPT", fdSets[core::DescriptorEventReceiver::DISP_TYPE::EX])) {
    }

    int EventMultiplexer::multiplex(utils::Timeval& tickTimeOut) {
        return core::system::select(getMaxFd() + 1,
                                    &fdSets[core::DescriptorEventReceiver::DISP_TYPE::RD].get(),
                                    &fdSets[core::DescriptorEventReceiver::DISP_TYPE::WR].get(),
                                    &fdSets[core::DescriptorEventReceiver::DISP_TYPE::EX].get(),
                                    &tickTimeOut);
    }

    void EventMultiplexer::publishActiveEvents() {
        if (activeEventCount > 0) {
            for (core::DescriptorEventPublisher* const descriptorEventPublisher : descriptorEventPublishers) {
                descriptorEventPublisher->publishActiveEvents();
            }
        }
    }

} // namespace core::select
