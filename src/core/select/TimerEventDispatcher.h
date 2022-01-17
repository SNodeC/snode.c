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

#ifndef CORE_SELECT_TIMEREVENTDISPATCHER_H
#define CORE_SELECT_TIMEREVENTDISPATCHER_H

#include "core/TimerEventDispatcher.h" // IWYU pragma: export

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <list>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::select {

    class TimerEventDispatcher : public core::TimerEventDispatcher {
    public:
        TimerEventDispatcher() = default;

        utils::Timeval getNextTimeout(const utils::Timeval& currentTime);

        void dispatchActiveEvents(const utils::Timeval& currentTime);

        void remove(core::TimerEventReceiver* timer);
        void add(core::TimerEventReceiver* timer);

        bool empty();

        void stop();

    private:
        std::list<core::TimerEventReceiver*> timerList;
        std::list<core::TimerEventReceiver*> addedList;
        std::list<core::TimerEventReceiver*> removedList;

        bool timerListDirty = false;
    };

} // namespace core::select

#endif // CORE_SELECT_TIMEREVENTDISPATCHER_H
