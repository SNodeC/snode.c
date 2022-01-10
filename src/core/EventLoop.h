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

#ifndef CORE_EVENTLOOP_H
#define CORE_EVENTLOOP_H

#include "core/TickStatus.h"
#include "utils/Timeval.h"

namespace core {
    class EventDispatcher;
#if defined(USE_EPOLL)
    namespace epoll {
        class EventDispatcher;
    }
#elif defined(USE_POLL)
    namespace poll {
        class EventDispatcher;
    }
#elif defined(USE_SELECT)
    namespace select {
        class EventDispatcher;
    }
#endif
} // namespace core

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core {

    class EventLoop {
        EventLoop() = delete;

        EventLoop(const EventLoop& eventLoop) = delete;
        EventLoop& operator=(const EventLoop& eventLoop) = delete;

        ~EventLoop() = delete;

    public:
        static unsigned long getTickCounter();
        static EventDispatcher& getEventDispatcher();

    private:
        // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays, hicpp-avoid-c-arrays, modernize-avoid-c-arrays)
        static void init(int argc, char* argv[]);
        static TickStatus _tick(const utils::Timeval& timeOut, bool stopped);
        static TickStatus tick(const utils::Timeval& timeOut = 0);
        static int start(const utils::Timeval& timeOut);
        static void free();

        static void stoponsig(int sig);

#if defined(USE_EPOLL)
        static core::epoll::EventDispatcher eventDispatcher;
#elif defined(USE_POLL)
        static core::poll::EventDispatcher eventDispatcher;
#elif defined(USE_SELECT)
        static core::select::EventDispatcher eventDispatcher;
#endif

        static bool running;
        static bool stopped;
        static int stopsig;
        static bool initialized;

        static unsigned long tickCounter;

        friend class SNodeC;
    };

} // namespace core

#endif // CORE_EVENTLOOP_H
