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

#ifndef CORE_TIMEREVENTPUBLISHER_H
#define CORE_TIMEREVENTPUBLISHER_H

namespace core {
    class TimerEventReceiver;
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "utils/Timeval.h"

#include <list>
#include <set>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core {

    class TimerEventPublisher {
    public:
        TimerEventPublisher() = default;

        utils::Timeval getNextTimeout(const utils::Timeval& currentTime);

        void publishActiveEvents(const utils::Timeval& currentTime);
        void unobsereDisableEvents();

        void remove(TimerEventReceiver* timer);

        void erase(TimerEventReceiver* timer);
        void insert(TimerEventReceiver* timer);

        bool empty();

        void stop();

    private:
        class timernode_lt {
        public:
            bool operator()(const TimerEventReceiver* t1, const TimerEventReceiver* t2) const;
        };

        std::set<TimerEventReceiver*, timernode_lt> timerList;
        std::list<TimerEventReceiver*> removedList;
    };

} // namespace core

#endif // CORE_TIMEREVENTPUBLISHER_H
