/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021, 2022, 2023 Volker Christian <me@vchrist.at>
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

#include "utils/system/signal.h"

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

            EventQueue(const EventQueue&) = delete;
            EventQueue(EventQueue&&) = delete;

            EventQueue& operator=(const EventQueue&) = delete;
            EventQueue& operator=(EventQueue&&) = delete;

            void insert(Event* event);
            void remove(Event* event);
            void execute(const utils::Timeval& currentTime);
            bool empty() const;
            void deleteEvents();

        private:
            std::list<Event*>* executeQueue;
            std::list<Event*>* publishQueue;
        };

    public:
#define DISP_COUNT 3

        enum DISP_TYPE { RD = 0, WR = 1, EX = 2 };

        DescriptorEventPublisher& getDescriptorEventPublisher(DISP_TYPE dispType);
        TimerEventPublisher& getTimerEventPublisher();

        void span(core::Event* event);
        void relax(core::Event* event);

        TickStatus tick(const utils::Timeval& tickTimeOut, const sigset_t& sigMask);
        void exit();
        void stop();
        void deletePublishedEvents();

    protected:
        int getObservedEventReceiverCount();
        int getMaxFd();

    private:
        void spanActiveEvents(const utils::Timeval& currentTime);
        void executeEventQueue(const utils::Timeval& currentTime);
        void checkTimedOutEvents(const utils::Timeval& currentTime);
        void releaseExpiredResources(const utils::Timeval& currentTime);
        TickStatus waitForEvents(const utils::Timeval& tickTimeOut, const utils::Timeval& currentTime, const sigset_t& sigMask);

        utils::Timeval getNextTimeout(const utils::Timeval& currentTime);

        virtual void spanActiveEvents() = 0;

        virtual int monitorDescriptors(utils::Timeval& tickTimeOut, const sigset_t& sigMask) = 0;

    protected:
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
