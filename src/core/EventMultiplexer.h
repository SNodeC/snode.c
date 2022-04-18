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
#include <list>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core {

    class EventMultiplexer {
        EventMultiplexer(const EventMultiplexer&) = delete;
        EventMultiplexer& operator=(const EventMultiplexer&) = delete;

    protected:
        EventMultiplexer(DescriptorEventPublisher* const readDescriptorEventPublisher,
                         DescriptorEventPublisher* const writeDescriptorEventPublisher,
                         DescriptorEventPublisher* const exceptionDescriptorEventPublisher);

        virtual ~EventMultiplexer();

    private:
        class EventQueue {
        public:
            EventQueue();
            ~EventQueue();

            void insert(Event* event);
            void remove(Event* event);
            void execute(const utils::Timeval& currentTime);
            bool empty() const;

        private:
            std::list<Event*>* executeQueue;
            std::list<Event*>* publishQueue;
        };

    public:
#define DISP_COUNT 3
        enum DISP_TYPE { RD = 0, WR = 1, EX = 2 };

        DescriptorEventPublisher& getDescriptorEventPublisher(core::EventMultiplexer::DISP_TYPE dispType);
        TimerEventPublisher& getTimerEventPublisher();

        void publish(core::Event* event);
        void unPublish(core::Event* event);

        TickStatus tick(const utils::Timeval& tickTimeOut);
        void stop();

    protected:
        int getObservedEventReceiverCount();
        int getMaxFd();

    private:
        void checkTimedOutEvents(const utils::Timeval& currentTime);

        utils::Timeval getNextTimeout(const utils::Timeval& currentTime);

        virtual int multiplex(utils::Timeval& tickTimeOut) = 0;
        void publishActiveEvents(int count, const utils::Timeval& currentTime);
        virtual void publishActiveEvents(int count) = 0;
        void unobserveDisabledEvents(const utils::Timeval& currentTime);

    protected:
        std::array<DescriptorEventPublisher*, DISP_COUNT> descriptorEventPublishers;
        core::TimerEventPublisher* const timerEventPublisher;

    private:
        EventQueue eventQueue;

        friend class DescriptorEventPublisher;
    };

} // namespace core

#endif // CORE_EVENTMULTIPLEXER_H
