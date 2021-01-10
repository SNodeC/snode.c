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

#include <algorithm>    // for min, max
#include <cerrno>       // for EINTR, errno
#include <csignal>      // for signal, SIGABRT, SIGHUP, SIGINT, SIGPIPE
#include <cstdlib>      // for exit
#include <ctime>        // for time, time_t
#include <string>       // for std::string std::to_string
#include <sys/select.h> // for select

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "EventDispatcher.h"
#include "EventLoop.h" // for EventLoop
#include "Logger.h"    // for Logger
#include "TimerEventDispatcher.h"
#include "Timeval.h" // for operator<

#define MAX_READ_INACTIVITY 60
#define MAX_WRITE_INACTIVITY 60
#define MAX_OUTOFBAND_INACTIVITY 60

namespace net {

    static std::string getTickCounterAsString([[maybe_unused]] const el::LogMessage* logMessage) {
        std::string tick = std::to_string(EventLoop::instance().getTickCounter());

        if (tick.length() < 10) {
            tick.insert(0, 10 - tick.length(), '0');
        }

        return tick;
    }

    EventLoop EventLoop::eventLoop;

    bool EventLoop::initialized = false;
    bool EventLoop::running = false;
    bool EventLoop::stopped = true;
    int EventLoop::stopsig = 0;

    EventLoop::EventLoop()
        : readEventDispatcher(new EventDispatcher(readFdSet, MAX_READ_INACTIVITY))
        , writeEventDispatcher(new EventDispatcher(writeFdSet, MAX_WRITE_INACTIVITY))
        , outOfBandEventDispatcher(new EventDispatcher(exceptFdSet, MAX_OUTOFBAND_INACTIVITY))
        , timerEventDispatcher(new TimerEventDispatcher()) {
    }

    EventLoop::~EventLoop() {
        delete readEventDispatcher;
        delete writeEventDispatcher;
        delete outOfBandEventDispatcher;
        delete timerEventDispatcher;
    }

    void EventLoop::tick() {
        struct timeval nextTimeout = readEventDispatcher->observeEnabledEvents();
        nextInactivityTimeout = std::min(nextTimeout, nextInactivityTimeout);
        nextTimeout = writeEventDispatcher->observeEnabledEvents();
        nextInactivityTimeout = std::min(nextTimeout, nextInactivityTimeout);
        nextTimeout = outOfBandEventDispatcher->observeEnabledEvents();
        nextInactivityTimeout = std::min(nextTimeout, nextInactivityTimeout);
        nextTimeout = timerEventDispatcher->getNextTimeout();
        nextInactivityTimeout = std::min(nextTimeout, nextInactivityTimeout);

        int maxFd = readEventDispatcher->getMaxFd();
        maxFd = std::max(writeEventDispatcher->getMaxFd(), maxFd);
        maxFd = std::max(outOfBandEventDispatcher->getMaxFd(), maxFd);

        if (maxFd >= 0 || !timerEventDispatcher->empty()) {
            if (nextInactivityTimeout < timeval({0, 0})) {
                nextInactivityTimeout = {0, 0};
            }

            int counter = select(maxFd + 1, &readFdSet.get(), &writeFdSet.get(), &exceptFdSet.get(), &nextInactivityTimeout);

            if (counter >= 0) {
                tickCounter++;
                timerEventDispatcher->dispatch();

                struct timeval currentTime = {time(nullptr), 0};
                nextInactivityTimeout = {LONG_MAX, 0};

                nextTimeout = readEventDispatcher->dispatchActiveEvents(currentTime);
                nextInactivityTimeout = std::min(nextTimeout, nextInactivityTimeout);
                nextTimeout = writeEventDispatcher->dispatchActiveEvents(currentTime);
                nextInactivityTimeout = std::min(nextTimeout, nextInactivityTimeout);
                nextTimeout = outOfBandEventDispatcher->dispatchActiveEvents(currentTime);
                nextInactivityTimeout = std::min(nextTimeout, nextInactivityTimeout);
            } else if (errno != EINTR) {
                PLOG(ERROR) << "select";
                stop();
            }
        } else {
            stop();
        }

        readEventDispatcher->unobserveDisabledEvents();
        writeEventDispatcher->unobserveDisabledEvents();
        outOfBandEventDispatcher->unobserveDisabledEvents();

        readEventDispatcher->releaseUnobservedEvents();
        writeEventDispatcher->releaseUnobservedEvents();
        outOfBandEventDispatcher->releaseUnobservedEvents();
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

        Logger::setCustomFormatSpec("%tick", net::getTickCounterAsString);

        EventLoop::initialized = true;
    }

    int EventLoop::start() {
        if (!initialized) {
            PLOG(ERROR) << "snode.c not initialized. Use EventLoop::init(argc, argv) before EventLoop::start().";
            exit(1);
        }

        stopped = false;

        if (!running) {
            running = true;

            while (!stopped) {
                eventLoop.tick();
            };

            eventLoop.readEventDispatcher->observeEnabledEvents();
            eventLoop.writeEventDispatcher->observeEnabledEvents();
            eventLoop.outOfBandEventDispatcher->observeEnabledEvents();

            eventLoop.readEventDispatcher->disableObservedEvents();
            eventLoop.writeEventDispatcher->disableObservedEvents();
            eventLoop.outOfBandEventDispatcher->disableObservedEvents();

            eventLoop.readEventDispatcher->unobserveDisabledEvents();
            eventLoop.writeEventDispatcher->unobserveDisabledEvents();
            eventLoop.outOfBandEventDispatcher->unobserveDisabledEvents();

            eventLoop.readEventDispatcher->releaseUnobservedEvents();
            eventLoop.writeEventDispatcher->releaseUnobservedEvents();
            eventLoop.outOfBandEventDispatcher->releaseUnobservedEvents();

            eventLoop.timerEventDispatcher->cancelAll();

            running = false;
        }

        int returnReason = 0;

        if (stopsig != 0) {
            returnReason = -1;
        }

        return returnReason;
    }

    void EventLoop::stop() {
        stopped = true;
    }

    void EventLoop::stoponsig(int sig) {
        stopsig = sig;
        stop();
    }

    unsigned long EventLoop::getEventCounter() {
        return readEventDispatcher->getEventCounter() + writeEventDispatcher->getEventCounter() +
               outOfBandEventDispatcher->getEventCounter();
    }

} // namespace net
