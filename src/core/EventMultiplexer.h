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

#include "core/DescriptorEventReceiver.h" // IWYU pragma: export
#include "core/TickStatus.h"              // IWYU pragma: export

namespace core {
    class Event;
    class DescriptorEventPublisher;
    class TimerEventPublisher;
} // namespace core

namespace utils {
    class Timeval; // IWYU pragma: keep
} // namespace utils

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "core/system/signal.h"

#include <array> // IWYU pragma: export
#include <list>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core {

    class EventMultiplexer {
    public:
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

            sigset_t newSet{};
            sigset_t oldSet{};
        };

    public:
        DescriptorEventPublisher& getDescriptorEventPublisher(core::DescriptorEventReceiver::DISP_TYPE dispType);
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
        void publishActiveEvents(const utils::Timeval& currentTime);
        virtual void publishActiveEvents() = 0;
        void releaseExpiredResources(const utils::Timeval& currentTime);
        void executeEventQueue(const utils::Timeval& currentTime);

    protected:
#define DISP_COUNT 3

        std::array<DescriptorEventPublisher*, DISP_COUNT> descriptorEventPublishers;

        int activeEventCount = 0;

    private:
        core::TimerEventPublisher* const timerEventPublisher;

        EventQueue eventQueue;

        friend class DescriptorEventPublisher;
    };

} // namespace core

core::EventMultiplexer& EventMultiplexer();

#endif // CORE_EVENTMULTIPLEXER_H
