﻿/*
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

#ifndef CORE_SELECT_EVENTDISPATCHER_H
#define CORE_SELECT_EVENTDISPATCHER_H

#include "core/EventMultiplexer.h"
#include "core/multiplexer/select/DescriptorEventPublisher.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

namespace utils {
    class Timeval;
}

#include <csignal>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::multiplexer::select {

    class EventMultiplexer : public core::EventMultiplexer {
    public:
        EventMultiplexer();
        ~EventMultiplexer() override = default;

    private:
        int monitorDescriptors(utils::Timeval& tickTimeOut, const sigset_t& sigMask) override;
        void spanActiveEvents(int activeDescriptorCount) override;

        FdSet fdSets[3];
    };

} // namespace core::multiplexer::select

#endif // CORE_SELECT_EVENTDISPATCHER_H
