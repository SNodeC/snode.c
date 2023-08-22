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
#include "log/Logger.h"
#include "utils/Timeval.h"

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

core::EventMultiplexer& EventMultiplexer() {
    static core::select::EventMultiplexer eventMultiplexer;

    return eventMultiplexer;
}

namespace core::select {

    EventMultiplexer::EventMultiplexer()
        : core::EventMultiplexer(new core::select::DescriptorEventPublisher("READ", fdSets[core::EventMultiplexer::DISP_TYPE::RD]),
                                 new core::select::DescriptorEventPublisher("WRITE", fdSets[core::EventMultiplexer::DISP_TYPE::WR]),
                                 new core::select::DescriptorEventPublisher("EXCEPT", fdSets[core::EventMultiplexer::DISP_TYPE::EX])) {
        LOG(TRACE) << "IO-Multiplexer: select";
    }

    int EventMultiplexer::monitorDescriptors(utils::Timeval& tickTimeOut, const sigset_t& sigMask) {
        timespec timeSpec = tickTimeOut.getTimespec();

        return core::system::pselect(getMaxFd() + 1,
                                     &fdSets[core::EventMultiplexer::DISP_TYPE::RD].get(),
                                     &fdSets[core::EventMultiplexer::DISP_TYPE::WR].get(),
                                     &fdSets[core::EventMultiplexer::DISP_TYPE::EX].get(),
                                     &timeSpec,
                                     &sigMask);
    }

    void EventMultiplexer::spanActiveEvents() {
        if (activeEventCount > 0) {
            for (core::DescriptorEventPublisher* const descriptorEventPublisher : descriptorEventPublishers) {
                descriptorEventPublisher->spanActiveEvents();
            }
        }
    }

} // namespace core::select
