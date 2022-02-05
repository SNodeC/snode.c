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

#ifndef CORE_EVENTDISPATCHER_H
#define CORE_EVENTDISPATCHER_H

#include "core/TickStatus.h" // IWYU pragma: export

namespace core {
    class Event;
    class DescriptorEventDispatcher;
    class TimerEventDispatcher;
} // namespace core

namespace utils {
    class Timeval;
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <array> // IWYU pragma: export
#include <deque>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core {

    class EventDispatcher {
        EventDispatcher(const EventDispatcher&) = delete;
        EventDispatcher& operator=(const EventDispatcher&) = delete;

    protected:
        EventDispatcher(core::DescriptorEventDispatcher* const readDescriptorEventDispatcher,
                        core::DescriptorEventDispatcher* const writeDescriptorEventDispatcher,
                        core::DescriptorEventDispatcher* const exceptionDescriptorEventDispatcher);

        ~EventDispatcher();

    public:
#define DISP_COUNT 3
        enum DISP_TYPE { RD = 0, WR = 1, EX = 2 };

        DescriptorEventDispatcher& getDescriptorEventDispatcher(core::EventDispatcher::DISP_TYPE dispType);
        TimerEventDispatcher& getTimerEventDispatcher();

        void publish(core::Event* event);

        TickStatus dispatch(const utils::Timeval& tickTimeOut, bool stopped);
        void stop();
        void stopDescriptorEvents();
        void stopTimerEvents();

    protected:
        int getObservedEventReceiverCount();
        int getMaxFd();

    private:
        void executeEventQueue(const utils::Timeval& currentTime);
        void checkTimedOutEvents(const utils::Timeval& currentTime);

        utils::Timeval getNextTimeout(const utils::Timeval& currentTime);

        void observeEnabledEvents(const utils::Timeval& currentTime);
        virtual int multiplex(utils::Timeval& tickTimeOut) = 0;
        virtual void dispatchActiveEvents(int count) = 0;
        void dispatchImmediateEvents(const utils::Timeval& currentTime);
        void unobserveDisabledEvents(const utils::Timeval& currentTime);

    protected:
        std::array<core::DescriptorEventDispatcher*, DISP_COUNT> descriptorEventDispatcher;

    private:
        std::deque<Event*> eventQueue;

        core::TimerEventDispatcher* const timerEventDispatcher;
    };

} // namespace core

#endif // CORE_EVENTDISPATCHER_H
