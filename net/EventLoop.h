/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020  Volker Christian <me@vchrist.at>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef EVENTLOOP_H
#define EVENTLOOP_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <sys/select.h> // for fd_set

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "AcceptEventDispatcher.h"
#include "OutOfBandEventDispatcher.h"
#include "ReadEventDispatcher.h"
#include "TimerEventDispatcher.h"
#include "WriteEventDispatcher.h"

class EventLoop {
private:
    EventLoop();

public:
    static EventLoop& instance() {
        return eventLoop;
    }

    ReadEventDispatcher& getReadEventDispatcher() {
        return readEventDispatcher;
    }

    AcceptEventDispatcher& getAcceptEventDispatcher() {
        return acceptEventDispatcher;
    }

    WriteEventDispatcher& getWriteEventDispatcher() {
        return writeEventDispatcher;
    }

    OutOfBandEventDispatcher& getOutOfBandEventDispatcher() {
        return outOfBandEventDispatcher;
    }

    TimerEventDispatcher& getTimerEventDispatcher() {
        return timerEventDispatcher;
    }

    static void init(int argc, char* argv[]); // NOLINT(cppcoreguidelines-avoid-c-arrays, hicpp-avoid-c-arrays, modernize-avoid-c-arrays)
    static void start();
    static void stop();

private:
    static void stoponsig(int sig);

    inline void tick();

    static EventLoop eventLoop;

    fd_set readfds{0};
    fd_set writefds{0};
    fd_set exceptfds{0};

    ReadEventDispatcher readEventDispatcher;
    AcceptEventDispatcher acceptEventDispatcher;
    WriteEventDispatcher writeEventDispatcher;
    OutOfBandEventDispatcher outOfBandEventDispatcher;
    TimerEventDispatcher timerEventDispatcher;

    static bool running;
    static bool stopped;
    static bool initialized;
};

#endif // EVENTLOOP_H
