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

#ifndef CORE_EVENTDISPATCHER_H
#define CORE_EVENTDISPATCHER_H

#include "core/TickStatus.h"

namespace core {
    class TimerEventDispatcher;
    class DescriptorEventDispatcher;
} // namespace core

namespace utils {
    class Timeval;
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "core/system/select.h" // IWYU pragma: keep

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core {

    class EventDispatcher {
    private:
        EventDispatcher(const EventDispatcher&) = delete;
        EventDispatcher& operator=(const EventDispatcher&) = delete;

        EventDispatcher() = default;

    public:
        static int getMaxFd();
        static utils::Timeval getNextTimeout(const utils::Timeval& currentTime);

        static void observeEnabledEvents();
        static void dispatchActiveEvents(const utils::Timeval& currentTime);
        static void unobserveDisabledEvents(const utils::Timeval& currentTime);
        static void terminateObservedEvents();
        static void terminateTimerEvents();

        static TickStatus dispatch(const utils::Timeval& tickTimeOut, bool stopped);

        static DescriptorEventDispatcher& getReadEventDispatcher();
        static DescriptorEventDispatcher& getWriteEventDispatcher();
        static DescriptorEventDispatcher& getExceptionalConditionEventDispatcher();

        static TimerEventDispatcher& getTimerEventDispatcher();

    private:
        enum DISP_TYPE { RD = 0, WR = 1, EX = 2, TI = 3 };

        static DescriptorEventDispatcher eventDispatcher[4];

        static TimerEventDispatcher timerEventDispatcher;

    public:
        class FdSet {
        public:
            FdSet();

            void set(int fd);
            void clr(int fd);
            int isSet(int fd) const;
            void zero();
            fd_set& get();

        protected:
            fd_set registered;
            fd_set active;
        };
    };

} // namespace core

#endif // CORE_EVENTDISPATCHER_H
