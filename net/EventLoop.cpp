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

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <algorithm>
#include <cassert>
#include <cerrno> // for EINTR, errno
#include <csignal>
#include <cstdlib>
#include <ctime>
#include <easylogging++.h>
#include <sys/time.h> // for timeval
#include <tuple>      // for tie, tuple

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "EventLoop.h"
#include "Logger.h"
#include "timer/Timer.h" // for operator<

namespace net {

    EventLoop EventLoop::eventLoop;

    bool EventLoop::running = false;
    bool EventLoop::stopped = true;
    bool EventLoop::initialized = false;

    EventLoop::EventLoop()
        : readEventDispatcher(readfds)
        , acceptEventDispatcher(readfds)
        , writeEventDispatcher(writefds)
        , outOfBandEventDispatcher(exceptfds) {
    }

    void EventLoop::tick() {
        readEventDispatcher.observeEnabledEvents();
        writeEventDispatcher.observeEnabledEvents();
        acceptEventDispatcher.observeEnabledEvents();
        outOfBandEventDispatcher.observeEnabledEvents();

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
                time_t timeout;
                nextInactivityTimeout = {-1, 0};
                std::tie(counter, timeout) = readEventDispatcher.dispatch(_readfds, counter);
                if (timeout >= 0) {
                    nextInactivityTimeout.tv_sec = timeout;
                }
                std::tie(counter, timeout) = writeEventDispatcher.dispatch(_writefds, counter);
                if (timeout >= 0) {
                    nextInactivityTimeout.tv_sec = std::min(timeout, nextInactivityTimeout.tv_sec);
                }
                std::tie(counter, timeout) = acceptEventDispatcher.dispatch(_readfds, counter);
                if (timeout >= 0) {
                    nextInactivityTimeout.tv_sec = std::min(timeout, nextInactivityTimeout.tv_sec);
                }
                std::tie(counter, timeout) = outOfBandEventDispatcher.dispatch(_exceptfds, counter);
                if (timeout >= 0) {
                    nextInactivityTimeout.tv_sec = std::min(timeout, nextInactivityTimeout.tv_sec);
                }
                assert(counter == 0);
            } else if (errno != EINTR) {
                PLOG(ERROR) << "select";
                stop();
            }
        } else {
            EventLoop::stopped = true;
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

            eventLoop.readEventDispatcher.unobserveDisabledEvents();
            eventLoop.writeEventDispatcher.unobserveDisabledEvents();
            eventLoop.acceptEventDispatcher.unobserveDisabledEvents();
            eventLoop.outOfBandEventDispatcher.unobserveDisabledEvents();

            eventLoop.readEventDispatcher.unobserveObservedEvents();
            eventLoop.writeEventDispatcher.unobserveObservedEvents();
            eventLoop.acceptEventDispatcher.unobserveObservedEvents();
            eventLoop.outOfBandEventDispatcher.unobserveObservedEvents();

            eventLoop.timerEventDispatcher.cancelAll();

            running = false;
        }
    }

    void EventLoop::stop() {
        stopped = true;
    }

    void EventLoop::stoponsig([[maybe_unused]] int sig) {
        stop();
    }

} // namespace net
