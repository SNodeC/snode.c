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

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <algorithm>       // for min, max
#include <cassert>         // for assert
#include <cerrno>          // for EINTR, errno
#include <csignal>         // for signal, SIGABRT, SIGHUP, SIGINT, SIGPIPE
#include <cstdlib>         // for exit
#include <ctime>           // for time, time_t
#include <easylogging++.h> // for PErrorWriter, CERROR, PLOG, Writer

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "EventLoop.h"   // for EventLoop
#include "Logger.h"      // for Logger
#include "timer/Timer.h" // for operator<

#define MAX_READ_INACTIVITY 60
#define MAX_ACCEPT_INACTIVITY LONG_MAX
#define MAX_WRITE_INACTIVITY 60
#define MAX_OUTOFBAND_INACTIVITY 60

namespace net {

    EventLoop EventLoop::eventLoop;

    bool EventLoop::running = false;
    bool EventLoop::stopped = true;
    bool EventLoop::initialized = false;

    EventLoop::EventLoop()
        : readEventDispatcher(readfds, MAX_READ_INACTIVITY)
        , acceptEventDispatcher(readfds, MAX_ACCEPT_INACTIVITY)
        , writeEventDispatcher(writefds, MAX_WRITE_INACTIVITY)
        , outOfBandEventDispatcher(exceptfds, MAX_OUTOFBAND_INACTIVITY) {
    }

    void EventLoop::tick() {
        long nextTimeout = readEventDispatcher.observeEnabledEvents();
        nextInactivityTimeout.tv_sec = std::min(nextTimeout, nextInactivityTimeout.tv_sec);

        nextTimeout = writeEventDispatcher.observeEnabledEvents();
        nextInactivityTimeout.tv_sec = std::min(nextTimeout, nextInactivityTimeout.tv_sec);

        nextTimeout = acceptEventDispatcher.observeEnabledEvents();
        nextInactivityTimeout.tv_sec = std::min(nextTimeout, nextInactivityTimeout.tv_sec);

        nextTimeout = outOfBandEventDispatcher.observeEnabledEvents();
        nextInactivityTimeout.tv_sec = std::min(nextTimeout, nextInactivityTimeout.tv_sec);

        int maxFd = readEventDispatcher.getLargestFd();
        maxFd = std::max(writeEventDispatcher.getLargestFd(), maxFd);
        maxFd = std::max(acceptEventDispatcher.getLargestFd(), maxFd);
        maxFd = std::max(outOfBandEventDispatcher.getLargestFd(), maxFd);

        fd_set _exceptfds = exceptfds;
        fd_set _writefds = writefds;
        fd_set _readfds = readfds;

        struct timeval tv = timerEventDispatcher.getNextTimeout();

        if (nextInactivityTimeout.tv_sec >= 0) {
            tv = std::min(tv, nextInactivityTimeout);
        }

        if (maxFd >= 0 || !timerEventDispatcher.empty()) {
            int counter = select(maxFd + 1, &_readfds, &_writefds, &_exceptfds, &tv);

            if (counter >= 0) {
                timerEventDispatcher.dispatch();

                time_t currentTime = time(nullptr);
                nextInactivityTimeout = {LONG_MAX, 0};

                nextTimeout = readEventDispatcher.dispatchActiveEvents(_readfds, counter, currentTime);
                nextInactivityTimeout.tv_sec = std::min(nextTimeout, nextInactivityTimeout.tv_sec);

                nextTimeout = writeEventDispatcher.dispatchActiveEvents(_writefds, counter, currentTime);
                nextInactivityTimeout.tv_sec = std::min(nextTimeout, nextInactivityTimeout.tv_sec);

                nextTimeout = acceptEventDispatcher.dispatchActiveEvents(_readfds, counter, currentTime);
                nextInactivityTimeout.tv_sec = std::min(nextTimeout, nextInactivityTimeout.tv_sec);

                nextTimeout = outOfBandEventDispatcher.dispatchActiveEvents(_exceptfds, counter, currentTime);
                nextInactivityTimeout.tv_sec = std::min(nextTimeout, nextInactivityTimeout.tv_sec);

                assert(counter == 0);
            } else if (errno != EINTR) {
                PLOG(ERROR) << "select";
                stop();
            }
        } else {
            stop();
        }

        readEventDispatcher.unobserveDisabledEvents();
        writeEventDispatcher.unobserveDisabledEvents();
        acceptEventDispatcher.unobserveDisabledEvents();
        outOfBandEventDispatcher.unobserveDisabledEvents();
    }

    // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays, hicpp-avoid-c-arrays, modernize-avoid-c-arrays)
    void EventLoop::init(int argc, char* argv[]) {
        signal(SIGPIPE, SIG_IGN);
        signal(SIGQUIT, EventLoop::stoponsig);
        signal(SIGHUP, EventLoop::stoponsig);
        signal(SIGINT, EventLoop::stoponsig);
        signal(SIGTERM, EventLoop::stoponsig);
        signal(SIGABRT, EventLoop::stoponsig);

        Logger::init(argc, argv);

        EventLoop::initialized = true;
    }

    void EventLoop::start() {
        if (!initialized) {
            PLOG(ERROR) << "snode.c not initialized. Use Multiplexer::init(argc, argv) before Multiplexer::start().";
            exit(1);
        }

        stopped = false;

        if (!running) {
            running = true;

            while (!stopped) {
                eventLoop.tick();
            };

            eventLoop.readEventDispatcher.observeEnabledEvents();
            eventLoop.writeEventDispatcher.observeEnabledEvents();
            eventLoop.acceptEventDispatcher.observeEnabledEvents();
            eventLoop.outOfBandEventDispatcher.observeEnabledEvents();

            eventLoop.readEventDispatcher.disableObservedEvents();
            eventLoop.writeEventDispatcher.disableObservedEvents();
            eventLoop.acceptEventDispatcher.disableObservedEvents();
            eventLoop.outOfBandEventDispatcher.disableObservedEvents();

            eventLoop.readEventDispatcher.unobserveDisabledEvents();
            eventLoop.writeEventDispatcher.unobserveDisabledEvents();
            eventLoop.acceptEventDispatcher.unobserveDisabledEvents();
            eventLoop.outOfBandEventDispatcher.unobserveDisabledEvents();

            eventLoop.timerEventDispatcher.cancelAll();

            running = false;
        }
    }

    void EventLoop::stop() {
        stopped = true;
    }

    void EventLoop::stoponsig(int sig) {
        VLOG(1) << "Stopped due to signal " << sig;
        stop();
    }

} // namespace net
