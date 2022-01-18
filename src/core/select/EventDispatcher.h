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

#ifndef CORE_SELECT_EVENTDISPATCHER_H
#define CORE_SELECT_EVENTDISPATCHER_H

#include "core/EventDispatcher.h"
#include "core/select/DescriptorEventDispatcher.h"

namespace utils {
    class Timeval;
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::select {

    class EventDispatcher : public core::EventDispatcher {
        EventDispatcher(const EventDispatcher&) = delete;
        EventDispatcher& operator=(const EventDispatcher&) = delete;

    public:
        EventDispatcher();
        ~EventDispatcher() = default;

    private:
        int multiplex(utils::Timeval& tickTimeOut) override;
        void dispatchActiveEvents(int count, const utils::Timeval& currentTime) override;

        FdSet fdSets[3];
    };

} // namespace core::select

#endif // CORE_SELECT_EVENTDISPATCHER_H
