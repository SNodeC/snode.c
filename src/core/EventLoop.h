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

#ifndef NET_EVENTLOOP_H
#define NET_EVENTLOOP_H

#include "core/EventDispatcher.h"
#include "core/TickStatus.h"
#include "core/TimerEventDispatcher.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <climits>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core {

    class EventLoop {
        EventLoop(const EventLoop& eventLoop) = delete;
        EventLoop& operator=(const EventLoop& eventLoop) = delete;

    private:
        EventLoop() = default;

        ~EventLoop() = default;

    public:
        static EventLoop& instance();

        EventDispatcher& getReadEventDispatcher();
        EventDispatcher& getWriteEventDispatcher();
        EventDispatcher& getExceptionalConditionEventDispatcher();
        TimerEventDispatcher& getTimerEventDispatcher();

        unsigned long getEventCounter();
        unsigned long getTickCounter();

    protected:
        // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays, hicpp-avoid-c-arrays, modernize-avoid-c-arrays)
        static void init(int argc, char* argv[]);
        static int start(struct timeval timeOut);
        static TickStatus tick(struct timeval timeOut);
        static void stop();
        static void free();

    private:
        static void stoponsig(int sig);

        TickStatus _tick(struct timeval timeOut);
        void _free();

        static EventLoop eventLoop;

        EventDispatcher readEventDispatcher;
        EventDispatcher writeEventDispatcher;
        EventDispatcher exceptionalConditionEventDispatcher;

        TimerEventDispatcher timerEventDispatcher;

        struct timeval nextEventTimeout = {LONG_MAX, 0};

        static bool running;
        static bool stopped;
        static int stopsig;
        static bool initialized;

        unsigned long tickCounter = 0;

        friend class SNodeC;
    };

} // namespace core

#endif // NET_EVENTLOOP_H
