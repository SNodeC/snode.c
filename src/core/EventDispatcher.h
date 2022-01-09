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

#include "core/DescriptorEventDispatcher.h"
#include "core/TickStatus.h"
#include "core/TimerEventDispatcher.h"

namespace core {
    class EventLoop;
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core {

    class EventDispatcher {
        EventDispatcher(const EventDispatcher&) = delete;
        EventDispatcher& operator=(const EventDispatcher&) = delete;

    private:
        EventDispatcher() = default;
        ~EventDispatcher() = default;

    public:
        enum DISP_TYPE { RD = 0, WR = 1, EX = 2, TI = 3 };

        DescriptorEventDispatcher& getDescriptorEventDispatcher(DISP_TYPE dispType);
        TimerEventDispatcher& getTimerEventDispatcher();

    private:
        int getMaxFd();
        utils::Timeval getNextTimeout(const utils::Timeval& currentTime);

        void observeEnabledEvents();
        void dispatchActiveEvents(const utils::Timeval& currentTime);
        void unobserveDisabledEvents(const utils::Timeval& currentTime);

        TickStatus dispatch(const utils::Timeval& tickTimeOut, bool stopped);

        void stop();

        DescriptorEventDispatcher eventDispatcher[4];
        TimerEventDispatcher timerEventDispatcher;

        friend class core::EventLoop;
    };

} // namespace core

#endif // CORE_EVENTDISPATCHER_H
