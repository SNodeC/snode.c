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

#ifndef CORE_EVENTMULTIPLEXER_H
#define CORE_EVENTMULTIPLEXER_H

#include "TickStatus.h" // IWYU pragma: export

// IWYU pragma: no_include "core/TickStatus.h"

namespace core {
    class Event;
    class DescriptorEventPublisher;
    class TimerEventPublisher;
} // namespace core

namespace utils {
    class Timeval;
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <array> // IWYU pragma: export
#include <set>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core {

    class EventMultiplexer {
        EventMultiplexer(const EventMultiplexer&) = delete;
        EventMultiplexer& operator=(const EventMultiplexer&) = delete;

    protected:
        EventMultiplexer(DescriptorEventPublisher* const readDescriptorEventDispatcher,
                         DescriptorEventPublisher* const writeDescriptorEventDispatcher,
                         DescriptorEventPublisher* const exceptionDescriptorEventDispatcher);

        ~EventMultiplexer();

    private:
        class EventQueue {
        public:
            EventQueue();
            ~EventQueue();

            void insert(const Event* event);
            void remove(const Event* event);
            void execute(const utils::Timeval& currentTime);
            bool empty() const;

        private:
            std::set<const Event*>* executeQueue;
            std::set<const Event*>* publishQueue;
        };

    public:
#define DISP_COUNT 3
        enum DISP_TYPE { RD = 0, WR = 1, EX = 2 };

        DescriptorEventPublisher& getDescriptorEventPublisher(core::EventMultiplexer::DISP_TYPE dispType);
        TimerEventPublisher& getTimerEventPublisher();

        void publish(const core::Event* event);
        void unPublish(const Event* event);

        TickStatus tick(const utils::Timeval& tickTimeOut, bool stopped);
        void stop();
        void stopDescriptorEvents();
        void stopTimerEvents();

    protected:
        int getObservedEventReceiverCount();
        int getMaxFd();

    private:
        void checkTimedOutEvents(const utils::Timeval& currentTime);

        utils::Timeval getNextTimeout(const utils::Timeval& currentTime);

        void observeEnabledEvents(const utils::Timeval& currentTime);
        virtual int multiplex(utils::Timeval& tickTimeOut) = 0;
        void dispatchActiveEvents(int count, const utils::Timeval &currentTime);
        virtual void dispatchActiveEvents(int count) = 0;
        void unobserveDisabledEvents(const utils::Timeval& currentTime);

    protected:
        std::array<DescriptorEventPublisher*, DISP_COUNT> descriptorEventPublisher;

    private:
        EventQueue eventQueue;

        core::TimerEventPublisher* const timerEventPublisher;
    };

} // namespace core

#endif // CORE_EVENTMULTIPLEXER_H
