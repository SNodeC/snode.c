/*
 * SNode.C - a slim toolkit for network communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025
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

#include "core/State.h" // IWYU pragma: export
#include "core/TickStatus.h"

namespace core {
    class EventMultiplexer;
} // namespace core

#ifndef DOXYGEN_SHOULD_SKIP_THIS

namespace utils {
    class Timeval;
}

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core {

    class EventLoop {
    public:
        EventLoop(const EventLoop& eventLoop) = delete;

        EventLoop& operator=(const EventLoop& eventLoop) = delete;

    private:
        EventLoop();
        ~EventLoop() = default;

    public:
        static EventLoop& instance();

        static unsigned long getTickCounter();
        EventMultiplexer& getEventMultiplexer();

        static core::State getEventLoopState();

    private:
        // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays, hicpp-avoid-c-arrays, modernize-avoid-c-arrays)
        static bool init(int argc, char* argv[]);
        TickStatus _tick(const utils::Timeval& timeOut);
        static TickStatus tick(const utils::Timeval& timeOut);
        static int start(const utils::Timeval& timeOut);
        static void stop();
        static void free();

        static void stoponsig(int sig);

        core::EventMultiplexer& eventMultiplexer;

        static int stopsig;

        static unsigned long tickCounter;

        static core::State eventLoopState;

        friend class SNodeC;
    };

} // namespace core

#endif // CORE_EVENTLOOP_H
