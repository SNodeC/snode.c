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

#include "core/EventLoop.h" // for EventLoop

#include "core/DynamicLoader.h"
#include "core/system/select.h"
#include "core/system/signal.h"
#include "log/Logger.h" // for Logger

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <algorithm> // for min, max
#include <cstdlib>   // for exit
#include <string>    // for string, to_string

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#define MAX_READ_INACTIVITY 60
#define MAX_WRITE_INACTIVITY 60
#define MAX_OUTOFBAND_INACTIVITY 60

namespace core {

    static std::string getTickCounterAsString(const el::LogMessage*) {
        std::string tick = std::to_string(EventLoop::instance().getTickCounter());

        if (tick.length() < 10) {
            tick.insert(0, 10 - tick.length(), '0');
        }

        return tick;
    }

    bool EventLoop::initialized = false;
    bool EventLoop::running = false;
    bool EventLoop::stopped = true;
    int EventLoop::stopsig = 0;

    EventLoop& EventLoop::instance() {
        static EventLoop eventLoop;
        return eventLoop;
    }

    EventDispatcher& EventLoop::getReadEventDispatcher() {
        return readEventDispatcher;
    }

    EventDispatcher& EventLoop::getWriteEventDispatcher() {
        return writeEventDispatcher;
    }

    EventDispatcher& EventLoop::getExceptionalConditionEventDispatcher() {
        return exceptionalConditionEventDispatcher;
    }

    TimerEventDispatcher& EventLoop::getTimerEventDispatcher() {
        return timerEventDispatcher;
    }

    TickStatus EventLoop::_tick(struct timeval tickTimeOut) {
        TickStatus tickStatus = TickStatus::SUCCESS;
        tickCounter++;

        EventDispatcher::observeEnabledEvents();
        int maxFd = EventDispatcher::getMaxFd();

        struct timeval nextEventTimeout = EventDispatcher::getNextTimeout();
        struct timeval nextTimerTimeout = timerEventDispatcher.getNextTimeout();

        struct timeval nextTimeout = std::min(nextTimerTimeout, nextEventTimeout);

        if (maxFd >= 0 || (!timerEventDispatcher.empty() && !stopped)) {
            nextTimeout = std::max(nextTimeout, {0, 0}); // In case nextEventTimeout is negativ
            nextTimeout = std::min(nextTimeout, tickTimeOut);

            int ret = core::system::select(maxFd + 1,
                                           &readEventDispatcher.getFdSet(),
                                           &writeEventDispatcher.getFdSet(),
                                           &exceptionalConditionEventDispatcher.getFdSet(),
                                           &nextTimeout);
            if (ret >= 0) {
                timerEventDispatcher.dispatch();
                EventDispatcher::dispatchActiveEvents();
                EventDispatcher::unobserveDisabledEvents();

                DynamicLoader::execDlCloseDeleyed();
            } else {
                PLOG(ERROR) << "select";
                tickStatus = TickStatus::SELECT_ERROR;
            }
        } else {
            tickStatus = TickStatus::NO_OBSERVER;
        }

        return tickStatus;
    }

    // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays, hicpp-avoid-c-arrays, modernize-avoid-c-arrays)
    void EventLoop::init(int argc, char* argv[]) {
        logger::Logger::init(argc, argv);

        logger::Logger::setCustomFormatSpec("%tick", core::getTickCounterAsString);

        EventLoop::initialized = true;
    }

    int EventLoop::start(struct timeval timeOut) {
        if (!initialized) {
            PLOG(ERROR) << "snode.c not initialized. Use SNodeC::init(argc, argv) before SNodeC::start().";
            exit(1);
        }

        sighandler_t oldSigPipeHandler = core::system::signal(SIGPIPE, SIG_IGN);
        sighandler_t oldSigQuitHandler = core::system::signal(SIGQUIT, EventLoop::stoponsig);
        sighandler_t oldSigHubHandler = core::system::signal(SIGHUP, EventLoop::stoponsig);
        sighandler_t oldSigIntHandler = core::system::signal(SIGINT, EventLoop::stoponsig);
        sighandler_t oldSigTermHandler = core::system::signal(SIGTERM, EventLoop::stoponsig);
        sighandler_t oldSigAbrtHandler = core::system::signal(SIGABRT, EventLoop::stoponsig);

        if (!running) {
            running = true;

            stopped = false;

            core::TickStatus tickStatus = TickStatus::SUCCESS;
            do {
                tickStatus = EventLoop::instance()._tick(timeOut);
            } while (tickStatus == TickStatus::SUCCESS && !stopped);

            free();

            running = false;
        }

        core::system::signal(SIGPIPE, oldSigPipeHandler);
        core::system::signal(SIGQUIT, oldSigQuitHandler);
        core::system::signal(SIGHUP, oldSigHubHandler);
        core::system::signal(SIGINT, oldSigIntHandler);
        core::system::signal(SIGTERM, oldSigTermHandler);
        core::system::signal(SIGABRT, oldSigAbrtHandler);

        int returnReason = 0;

        if (stopsig != 0) {
            returnReason = -1;
        }

        return returnReason;
    }

    TickStatus EventLoop::tick(struct timeval timeOut) {
        if (!initialized) {
            PLOG(ERROR) << "snode.c not initialized. Use SNodeC::init(argc, argv) before SNodeC::tick().";
            exit(1);
        }

        sighandler_t oldSigPipeHandler = core::system::signal(SIGPIPE, SIG_IGN);

        TickStatus tickStatus = EventLoop::instance()._tick(timeOut);

        core::system::signal(SIGPIPE, oldSigPipeHandler);

        return tickStatus;
    }

    void EventLoop::free() {
        core::TickStatus tickStatus = TickStatus::SUCCESS;

        do {
            EventDispatcher::terminateObservedEvents();
            tickStatus = EventLoop::tick({0, 0});
        } while (tickStatus == TickStatus::SUCCESS);

        do {
            EventLoop::instance().timerEventDispatcher.cancelAll();
            tickStatus = EventLoop::tick({0, 0});
        } while (tickStatus == TickStatus::SUCCESS);
    }

    void EventLoop::stoponsig(int sig) {
        stopsig = sig;
        stopped = true;
    }

    unsigned long EventLoop::getEventCounter() {
        return readEventDispatcher.getEventCounter() + writeEventDispatcher.getEventCounter() +
               exceptionalConditionEventDispatcher.getEventCounter();
    }

    unsigned long EventLoop::getTickCounter() {
        return tickCounter;
    }

} // namespace core
