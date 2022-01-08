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
    class EventLoop;
    class EventReceiver;

    namespace timer {
        class Timer;
    }
} // namespace core

namespace utils {
    class Timeval;
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "core/system/select.h"

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core {

    class EventDispatcher {
        EventDispatcher() = default;

        EventDispatcher(const EventDispatcher&) = delete;
        EventDispatcher& operator=(const EventDispatcher&) = delete;

        ~EventDispatcher() = default;

    public:
        enum DISP_TYPE { RD = 0, WR = 1, EX = 2, TI = 3 };

    private:
        DescriptorEventDispatcher& getDescriptorEventDispatcher(DISP_TYPE dispType);
        TimerEventDispatcher& getTimerEventDispatcher();

        int getMaxFd();
        utils::Timeval getNextTimeout(const utils::Timeval& currentTime);

        void observeEnabledEvents();
        void dispatchActiveEvents(const utils::Timeval& currentTime);
        void unobserveDisabledEvents(const utils::Timeval& currentTime);

        TickStatus dispatch(const utils::Timeval& tickTimeOut, bool stopped);

        void stop();

        static DescriptorEventDispatcher eventDispatcher[4];
        static TimerEventDispatcher timerEventDispatcher;

        friend class core::EventLoop;
        friend class core::EventReceiver;
        friend class core::timer::Timer;
    };

} // namespace core

#endif // CORE_EVENTDISPATCHER_H
