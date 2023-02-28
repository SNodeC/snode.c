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

#ifndef CORE_SELECT_EVENTDISPATCHER_H
#define CORE_SELECT_EVENTDISPATCHER_H

#include "core/EventMultiplexer.h"
#include "core/multiplexer/select/DescriptorEventPublisher.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

namespace utils {
    class Timeval;
}

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::select {

    class EventMultiplexer : public core::EventMultiplexer {
    public:
        EventMultiplexer(const EventMultiplexer&) = delete;

        EventMultiplexer& operator=(const EventMultiplexer&) = delete;

        EventMultiplexer();
        ~EventMultiplexer() override = default;

    private:
        int monitorDescriptors(utils::Timeval& tickTimeOut) override;
        void spanActiveEvents() override;

        FdSet fdSets[3];
    };

} // namespace core::select

#endif // CORE_SELECT_EVENTDISPATCHER_H
