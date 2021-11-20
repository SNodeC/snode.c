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

#ifndef NET_TIMEREVENTDISPATCHER_H
#define NET_TIMEREVENTDISPATCHER_H

#include "core/TimerEventReceiver.h" // IWYU pragma: export

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <list>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core {

    class TimerEventDispatcher {
    public:
        TimerEventDispatcher() = default;

        struct timeval getNextTimeout();

        void dispatch();

        void remove(TimerEventReceiver* timer);
        void add(TimerEventReceiver* timer);

        bool empty();

        void cancelAll();

    private:
        std::list<TimerEventReceiver*> timerList;
        std::list<TimerEventReceiver*> addedList;
        std::list<TimerEventReceiver*> removedList;

        class timernode_lt {
        public:
            bool operator()(const TimerEventReceiver* t1, const TimerEventReceiver* t2) const {
                return static_cast<struct timeval>(*t1) < static_cast<struct timeval>(*t2);
            }
        };

        bool timerListDirty = false;
    };

} // namespace net

#endif // NET_TIMEREVENTDISPATCHER_H
