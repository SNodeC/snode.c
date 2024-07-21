/*
 * SNode.C - a slim toolkit for network communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024
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

#include <array>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

core::EventMultiplexer& EventMultiplexer() {
    static core::multiplexer::select::EventMultiplexer eventMultiplexer;

    return eventMultiplexer;
}

namespace core::multiplexer::select {

    EventMultiplexer::EventMultiplexer()
        : core::EventMultiplexer(new core::multiplexer::select::DescriptorEventPublisher("READ", //
                                                                                 fdSets[core::EventMultiplexer::DISP_TYPE::RD]),
                                 new core::multiplexer::select::DescriptorEventPublisher("WRITE", //
                                                                                 fdSets[core::EventMultiplexer::DISP_TYPE::WR]),
                                 new core::multiplexer::select::DescriptorEventPublisher("EXCEPT", //
                                                                                 fdSets[core::EventMultiplexer::DISP_TYPE::EX])) {
        LOG(DEBUG) << "Core::multiplexer: select";
    }

    int EventMultiplexer::monitorDescriptors(utils::Timeval& tickTimeOut, const sigset_t& sigMask) {
        const timespec timeSpec = tickTimeOut.getTimespec();

        return core::system::pselect(maxFd() + 1,
                                     &fdSets[core::EventMultiplexer::DISP_TYPE::RD].get(),
                                     &fdSets[core::EventMultiplexer::DISP_TYPE::WR].get(),
                                     &fdSets[core::EventMultiplexer::DISP_TYPE::EX].get(),
                                     &timeSpec,
                                     &sigMask);
    }

    void EventMultiplexer::spanActiveEvents(int activeDescriptorCount) {
        if (activeDescriptorCount > 0) {
            for (core::DescriptorEventPublisher* const descriptorEventPublisher : descriptorEventPublishers) {
                descriptorEventPublisher->spanActiveEvents();
            }
        }
    }

} // namespace core::multiplexer::select
