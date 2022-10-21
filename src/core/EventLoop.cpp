/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021, 2022 Volker Christian <me@vchrist.at>
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

#include "core/EventLoop.h"

#include "core/DynamicLoader.h"
#include "core/EventMultiplexer.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "core/system/signal.h"
#include "log/Logger.h"
#include "utils/Config.h"

#include <cstdlib>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

/* Must be implemented in every variant of a multiplexer api */
core::EventMultiplexer& EventMultiplexer();

namespace core {

    bool EventLoop::initialized = false;
    bool EventLoop::running = false;
    bool EventLoop::stopped = true;
    int EventLoop::stopsig = 0;
    unsigned long EventLoop::tickCounter = 0;

    static std::string getTickCounterAsString(const el::LogMessage*) {
        std::string tick = std::to_string(EventLoop::getTickCounter());

        if (tick.length() < 10) {
            tick.insert(0, 10 - tick.length(), '0');
        }

        return tick;
    }

    EventLoop::EventLoop()
        : eventMultiplexer(::EventMultiplexer())

    {
    }

    EventLoop& EventLoop::instance() {
        static EventLoop eventLoop;

        return eventLoop;
    }

    unsigned long EventLoop::getTickCounter() {
        return tickCounter;
    }

    EventMultiplexer& EventLoop::getEventMultiplexer() {
        return eventMultiplexer;
    }

    // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays, hicpp-avoid-c-arrays, modernize-avoid-c-arrays)
    void EventLoop::init(int argc, char* argv[]) {
        logger::Logger::setCustomFormatSpec("%tick", core::getTickCounterAsString);

        utils::Config::init(argc, argv);

        EventLoop::initialized = true;
    }

    TickStatus EventLoop::_tick(const utils::Timeval& tickTimeOut) {
        tickCounter++;

        return eventMultiplexer.tick(tickTimeOut);
    }

    TickStatus EventLoop::tick(const utils::Timeval& timeOut) {
        if (!initialized) {
            PLOG(ERROR) << "snode.c not initialized. Use SNodeC::init(argc, argv) before SNodeC::tick().";
            exit(1);
        }

        sighandler_t oldSigPipeHandler = core::system::signal(SIGPIPE, SIG_IGN);

        TickStatus tickStatus = EventLoop::instance()._tick(timeOut);

        core::system::signal(SIGPIPE, oldSigPipeHandler);

        return tickStatus;
    }

    int EventLoop::start(const utils::Timeval& timeOut) {
        if (!initialized) {
            PLOG(ERROR) << "snode.c not initialized. Use SNodeC::init(argc, argv) before SNodeC::start().";
            exit(1);
        }

        utils::Config::prepare();

        struct sigaction sact {};
        sigemptyset(&sact.sa_mask);
        sact.sa_flags = 0;

        sact.sa_handler = SIG_IGN;

        struct sigaction oldPipeAct {};
        sigaction(SIGPIPE, &sact, &oldPipeAct);

        sact.sa_handler = EventLoop::stoponsig;

        struct sigaction oldIntAct {};
        sigaction(SIGINT, &sact, &oldIntAct);

        struct sigaction oldTermAct {};
        sigaction(SIGTERM, &sact, &oldTermAct);

        struct sigaction oldAlarmAct {};
        sigaction(SIGALRM, &sact, &oldAlarmAct);

        struct sigaction oldHupAct {};
        sigaction(SIGHUP, &sact, &oldHupAct);

        if (!running) {
            running = true;
            stopped = false;

            core::TickStatus tickStatus = TickStatus::SUCCESS;

            while (tickStatus == TickStatus::SUCCESS && !stopped) {
                tickStatus = EventLoop::instance()._tick(timeOut);
            }

            switch (tickStatus) {
                case TickStatus::SUCCESS:
                    LOG(INFO) << "EventLoop terminated: Releasing resources";
                    break;
                case TickStatus::NO_OBSERVER:
                    LOG(INFO) << "EventLoop: No Observer - exiting";
                    break;
                case TickStatus::ERROR:
                    PLOG(ERROR) << "EventPublisher::publish()";
                    break;
            }

            running = false;
        }

        sigaction(SIGPIPE, &oldPipeAct, nullptr);
        sigaction(SIGINT, &oldIntAct, nullptr);
        sigaction(SIGTERM, &oldTermAct, nullptr);
        sigaction(SIGALRM, &oldAlarmAct, nullptr);
        sigaction(SIGHUP, &oldHupAct, nullptr);

        free();

        int returnReason = 0;
        if (stopsig != 0) {
            returnReason = -stopsig;
        }

        return returnReason;
    }

    void EventLoop::stop() {
        stopped = true;
    }

    void EventLoop::free() {
        core::TickStatus tickStatus{};

        do {
            EventLoop::instance().eventMultiplexer.stop();
            tickStatus = EventLoop::instance()._tick(3);
        } while (tickStatus == TickStatus::SUCCESS);

        DynamicLoader::execDlCloseAll();

        utils::Config::terminate();

        LOG(INFO) << "All resources released";
    }

    void EventLoop::stoponsig(int sig) {
        stopsig = sig;
        stop();
    }

} // namespace core
