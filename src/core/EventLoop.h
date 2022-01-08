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

#include "core/EventDispatcher.h"
#include "core/TickStatus.h"
#include "utils/Timeval.h"

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

        static EventDispatcher eventDispatcher;

        static bool running;
        static bool stopped;
        static int stopsig;
        static bool initialized;

        static unsigned long tickCounter;

        friend class SNodeC;
    };

} // namespace core

#endif // CORE_EVENTLOOP_H
