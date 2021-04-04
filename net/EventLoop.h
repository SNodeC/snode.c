/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020 Volker Christian <me@vchrist.at>
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

#ifndef EVENTLOOP_H
#define EVENTLOOP_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <climits>
#include <sys/time.h> // for timeval

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "net/DescriptorEventDispatcher.h"
#include "net/TimerEventDispatcher.h"

namespace net {

    class EventLoop {
    private:
        EventLoop() = default;

        EventLoop(const EventLoop& eventLoop) = delete;

        EventLoop& operator=(const EventLoop& eventLoop) = delete;

        ~EventLoop() = default;

    public:
        static EventLoop& instance() {
            return eventLoop;
        }

        DescriptorEventDispatcher& getReadEventDispatcher() {
            return readEventDispatcher;
        }

        DescriptorEventDispatcher& getWriteEventDispatcher() {
            return writeEventDispatcher;
        }

        DescriptorEventDispatcher& getExceptionalConditionEventDispatcher() {
            return exceptionalConditionEventDispatcher;
        }

        TimerEventDispatcher& getTimerEventDispatcher() {
            return timerEventDispatcher;
        }

        unsigned long getEventCounter();

        unsigned long getTickCounter() {
            return tickCounter;
        }

    protected:
        // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays, hicpp-avoid-c-arrays, modernize-avoid-c-arrays)
        static void init(int argc, char* argv[]);
        static int start();
        static void stop();

    private:
        static void stoponsig(int sig);

        inline void tick();

        static EventLoop eventLoop;

        DescriptorEventDispatcher readEventDispatcher;
        DescriptorEventDispatcher writeEventDispatcher;
        DescriptorEventDispatcher exceptionalConditionEventDispatcher;

        TimerEventDispatcher timerEventDispatcher;

        struct timeval nextInactivityTimeout = {LONG_MAX, 0};

        static bool running;
        static bool stopped;
        static int stopsig;
        static bool initialized;

        unsigned long tickCounter = 0;

        friend class SNodeC;
    };

} // namespace net

#endif // EVENTLOOP_H
