/*
 * SNode.C - a slim toolkit for network communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024
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

#include "log/Logger.h"
#include "utils/Config.h"
#include "utils/Timeval.h"
#include "utils/system/signal.h"

#include <chrono> // IWYU pragma: keep
#include <cstring>

// IWYU pragma: no_include <bits/chrono.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core {

    int EventLoop::stopsig = 0;
    unsigned long EventLoop::tickCounter = 0;
    core::State EventLoop::eventLoopState = State::LOADED;

    static std::string getTickCounterAsString([[maybe_unused]] const el::LogMessage* logMessage) {
        std::string tick = std::to_string(EventLoop::getTickCounter());

        if (tick.length() < 13) {
            tick.insert(0, 13 - tick.length(), '0');
        }

        return tick;
    }

    EventLoop::EventLoop()
        : eventMultiplexer(::EventMultiplexer()) {
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

    State EventLoop::getEventLoopState() {
        return eventLoopState;
    }

    // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays, hicpp-avoid-c-arrays, modernize-avoid-c-arrays)
    bool EventLoop::init(int argc, char* argv[]) {
        struct sigaction sact {};
        sigemptyset(&sact.sa_mask);
        sact.sa_flags = 0;
        sact.sa_handler = SIG_IGN;

        struct sigaction oldPipeAct {};
        sigaction(SIGPIPE, &sact, &oldPipeAct);

        struct sigaction oldIntAct {};
        sigaction(SIGINT, &sact, &oldIntAct);

        struct sigaction oldTermAct {};
        sigaction(SIGTERM, &sact, &oldTermAct);

        struct sigaction oldAlarmAct {};
        sigaction(SIGALRM, &sact, &oldAlarmAct);

        struct sigaction oldHupAct {};
        sigaction(SIGHUP, &sact, &oldHupAct);

        logger::Logger::setCustomFormatSpec("%tick", core::getTickCounterAsString);

        if (utils::Config::init(argc, argv)) {
            eventLoopState = State::INITIALIZED;

            LOG(TRACE) << "SNode.C: Starting ... HELLO";
        }

        sigaction(SIGPIPE, &oldPipeAct, nullptr);
        sigaction(SIGINT, &oldIntAct, nullptr);
        sigaction(SIGTERM, &oldTermAct, nullptr);
        sigaction(SIGALRM, &oldAlarmAct, nullptr);
        sigaction(SIGHUP, &oldHupAct, nullptr);

        return eventLoopState == State::INITIALIZED;
    }

    TickStatus EventLoop::_tick(const utils::Timeval& tickTimeOut) {
        TickStatus tickStatus = TickStatus::SUCCESS;

        tickCounter++;

        sigset_t newSet{};
        sigaddset(&newSet, SIGINT);
        sigaddset(&newSet, SIGTERM);
        sigaddset(&newSet, SIGALRM);
        sigaddset(&newSet, SIGHUP);

        sigset_t oldSet{};
        sigprocmask(SIG_BLOCK, &newSet, &oldSet);

        if (eventLoopState == State::RUNNING || eventLoopState == State::STOPPING) {
            tickStatus = eventMultiplexer.tick(tickTimeOut, oldSet);
        }

        sigprocmask(SIG_SETMASK, &oldSet, nullptr);

        return tickStatus;
    }

    TickStatus EventLoop::tick(const utils::Timeval& timeOut) {
        TickStatus tickStatus = TickStatus::TRACE;

        if (eventLoopState == State::INITIALIZED) {
            struct sigaction sact {};
            sigemptyset(&sact.sa_mask);
            sact.sa_flags = 0;
            sact.sa_handler = SIG_IGN;

            struct sigaction oldPipeAct {};
            sigaction(SIGPIPE, &sact, &oldPipeAct);

            tickStatus = EventLoop::instance()._tick(timeOut);

            sigaction(SIGPIPE, &oldPipeAct, nullptr);
        } else {
            EventLoop::instance().eventMultiplexer.clearEventQueue();
            free();

            PLOG(FATAL) << "Core: not initialized: No events will be processed\nCall SNodeC::init(argc, argv) before SNodeC::tick().";
        }

        return tickStatus;
    }

    int EventLoop::start(const utils::Timeval& timeOut) {
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

        if (eventLoopState == State::INITIALIZED && utils::Config::bootstrap()) {
            eventLoopState = State::RUNNING;
            core::TickStatus tickStatus = TickStatus::SUCCESS;

            LOG(TRACE) << "Core::EventLoop: started";

            do {
                tickStatus = EventLoop::instance()._tick(timeOut);
            } while ((tickStatus == TickStatus::SUCCESS || tickStatus == TickStatus::INTERRUPTED) && eventLoopState == State::RUNNING);

            switch (tickStatus) {
                case TickStatus::SUCCESS:
                    [[fallthrough]];
                case TickStatus::INTERRUPTED:
                    LOG(TRACE) << "Core::EventLoop: Interrupted";
                    break;
                case TickStatus::NOOBSERVER:
                    LOG(TRACE) << "Core::EventLoop: No Observer";
                    break;
                case TickStatus::TRACE:
                    PLOG(FATAL) << "Core::EventLoop: _tick()";
                    break;
            }

        } else {
            EventLoop::instance().eventMultiplexer.clearEventQueue();
        }

        sigaction(SIGPIPE, &oldPipeAct, nullptr);
        sigaction(SIGINT, &oldIntAct, nullptr);
        sigaction(SIGTERM, &oldTermAct, nullptr);
        sigaction(SIGALRM, &oldAlarmAct, nullptr);
        sigaction(SIGHUP, &oldHupAct, nullptr);

        free();

        return stopsig;
    }

    void EventLoop::stop() {
        eventLoopState = State::STOPPING;
    }

    void EventLoop::free() {
        std::string signal = "SIG" + utils::system::sigabbrev_np(stopsig);

        if (signal == "SIGUNKNOWN") {
            signal = std::to_string(stopsig);
        }

        eventLoopState = State::STOPPING;

        if (stopsig != 0) {
            LOG(TRACE) << "Core: Sending signal " << signal << " to all DescriptorEventReceivers";

            EventLoop::instance().eventMultiplexer.signal(stopsig);
        }

        utils::Timeval timeout = 2;

        core::TickStatus tickStatus = TickStatus::SUCCESS;
        do {
            auto t1 = std::chrono::system_clock::now();
            const utils::Timeval timeoutOp = timeout;

            tickStatus = EventLoop::instance()._tick(timeoutOp);

            auto t2 = std::chrono::system_clock::now();
            const std::chrono::duration<double> seconds = t2 - t1;

            timeout -= seconds.count();
        } while (timeout > 0 && (tickStatus == TickStatus::SUCCESS || tickStatus == TickStatus::INTERRUPTED));

        LOG(TRACE) << "Core: Terminating all stalled  DescriptorEventReceivers";

        EventLoop::instance().eventMultiplexer.terminate();

        LOG(TRACE) << "Core: Closing all libraries opened during runtime";

        DynamicLoader::execDlCloseAll();

        LOG(TRACE) << "Core:: Cleaning up filesystem";

        utils::Config::terminate();

        LOG(TRACE) << "Core:: All resources released";

        LOG(TRACE) << "SNode.C: Ended ... BYE";
    }

    void EventLoop::stoponsig(int sig) {
        LOG(INFO) << "Core: Received signal '" << strsignal(sig) << "' (SIG" << utils::system::sigabbrev_np(sig) << " = " << sig << ")";
        stopsig = sig;
        stop();
    }

} // namespace core
