/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
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

/*
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef CORE_EVENTMULTIPLEXER_H
#define CORE_EVENTMULTIPLEXER_H

#include "core/TickStatus.h"

namespace core {
    class Event;
    class DescriptorEventPublisher;
    class TimerEventPublisher;
} // namespace core

namespace utils {
    class Timeval;
} // namespace utils

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "utils/system/signal.h"

#include <array>
#include <list>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core {

    class EventMultiplexer {
    public:
        EventMultiplexer(DescriptorEventPublisher* readDescriptorEventPublisher,
                         DescriptorEventPublisher* writeDescriptorEventPublisher,
                         DescriptorEventPublisher* exceptionDescriptorEventPublisher);
        virtual ~EventMultiplexer();

        EventMultiplexer(const EventMultiplexer&) = delete;

        EventMultiplexer& operator=(const EventMultiplexer&) = delete;

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
            void clear();

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

        void signal(int sigNum);
        void terminate();
        void clearEventQueue();

        TickStatus tick(const utils::Timeval& tickTimeOut, const sigset_t& sigMask);

    private:
        TickStatus waitForEvents(const utils::Timeval& tickTimeOut,
                                 const utils::Timeval& currentTime,
                                 const sigset_t& sigMask,
                                 int& activeDescriptorCount);

        void spanActiveEvents(const utils::Timeval& currentTime, int activeDescriptorCount);
        void executeEventQueue(const utils::Timeval& currentTime);
        void checkTimedOutEvents(const utils::Timeval& currentTime);
        void releaseExpiredResources(const utils::Timeval& currentTime);

        utils::Timeval getNextTimeout(const utils::Timeval& currentTime);

        virtual void spanActiveEvents(int activeDescriptorCount) = 0;

        virtual int monitorDescriptors(utils::Timeval& tickTimeOut, const sigset_t& sigMask) = 0;

    protected:
        int maxFd();

        std::array<DescriptorEventPublisher*, DISP_COUNT> descriptorEventPublishers;

    private:
        int observedEventReceiverCount();

        core::TimerEventPublisher* const timerEventPublisher;

        EventQueue eventQueue;
    };

} // namespace core

core::EventMultiplexer& EventMultiplexer();

#endif // CORE_EVENTMULTIPLEXER_H
