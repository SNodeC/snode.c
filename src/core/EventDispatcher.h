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

#include "core/TickStatus.h" // IWYU pragma: export

namespace core {
    //    class EventLoop;
    class DescriptorEventDispatcher;
    class TimerEventDispatcher;
} // namespace core

namespace utils {
    class Timeval;
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core {

    class EventDispatcher {
        EventDispatcher(const EventDispatcher&) = delete;
        EventDispatcher& operator=(const EventDispatcher&) = delete;

    protected:
        EventDispatcher() = default;
        ~EventDispatcher() = default;

    public:
        enum DISP_TYPE { RD = 0, WR = 1, EX = 2 };

        virtual DescriptorEventDispatcher& getDescriptorEventDispatcher(core::EventDispatcher::DISP_TYPE dispType) = 0;
        virtual TimerEventDispatcher& getTimerEventDispatcher() = 0;

        virtual TickStatus dispatch(const utils::Timeval& tickTimeOut, bool stopped) = 0;
        virtual void stop() = 0;

    protected:
        void observeEnabledEvents();
        void unobserveDisabledEvents(const utils::Timeval& currentTime);

        utils::Timeval getNextTimeout(const utils::Timeval& currentTime);
        /*
        core::DescriptorEventDispatcher& eventDispatcherrr[];
        core::TimerEventDispatcher& timerEventDispatcherrr;
        */
    };

} // namespace core

#endif // CORE_EVENTDISPATCHER_H
