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

#include <chrono>
#include <cstdlib>
#include <cstring>
#include <string>

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

    // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays, hicpp-avoid-c-arrays, modernize-avoid-c-arrays)
    bool EventLoop::init(int argc, char* argv[]) {
        logger::Logger::setCustomFormatSpec("%tick", core::getTickCounterAsString);

        if (utils::Config::init(argc, argv)) {
            eventLoopState = State::INITIALIZED;

            LOG(TRACE) << "SNode.C: Starting ... HELLO";
        }

        return eventLoopState == State::INITIALIZED;
    }

    TickStatus EventLoop::_tick(const utils::Timeval& tickTimeOut) {
        TickStatus tickStatus = TickStatus::SUCCESS;

        tickCounter++;

        sigset_t newSet{};
        sigset_t oldSet{};

        sigaddset(&newSet, SIGINT);
        sigaddset(&newSet, SIGTERM);
        sigaddset(&newSet, SIGALRM);
        sigaddset(&newSet, SIGHUP);

        sigprocmask(SIG_BLOCK, &newSet, &oldSet);

        if (eventLoopState == State::RUNNING || eventLoopState == State::STOPING) {
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
        if (eventLoopState == State::INITIALIZED && utils::Config::bootstrap()) {
            LOG(TRACE) << "Core::EventLoop: started";

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

            eventLoopState = State::RUNNING;
            core::TickStatus tickStatus = TickStatus::SUCCESS;

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

            sigaction(SIGPIPE, &oldPipeAct, nullptr);
            sigaction(SIGINT, &oldIntAct, nullptr);
            sigaction(SIGTERM, &oldTermAct, nullptr);
            sigaction(SIGALRM, &oldAlarmAct, nullptr);
            sigaction(SIGHUP, &oldHupAct, nullptr);
        } else {
            EventLoop::instance().eventMultiplexer.clearEventQueue();
        }

        free();

        return stopsig;
    }

    void EventLoop::stop() {
        eventLoopState = State::STOPING;
    }

    void EventLoop::free() {
        std::string signal = "SIG" + utils::system::sigabbrev_np(stopsig);

        if (signal == "SIGUNKNOWN") {
            signal = std::to_string(stopsig);
        }

        LOG(TRACE) << "Core: Sending 'onExit(" << signal << ")' to all DescriptorEventReceivers";

        eventLoopState = State::STOPING;

        if (stopsig != 0) {
            EventLoop::instance().eventMultiplexer.sigExit(stopsig);
        }

        utils::Timeval timeout = 2;

        core::TickStatus tickStatus = TickStatus::SUCCESS;
        do {
            LOG(TRACE) << "Core: Stopping all DescriptorEventReceivers";

            auto t1 = std::chrono::system_clock::now();

            EventLoop::instance().eventMultiplexer.stop();

            const utils::Timeval timeoutOp = timeout;
            tickStatus = EventLoop::instance()._tick(timeoutOp);

            auto t2 = std::chrono::system_clock::now();

            const std::chrono::duration<double> seconds = t2 - t1;

            timeout -= seconds.count();
        } while (timeout > 0 && (tickStatus == TickStatus::SUCCESS || tickStatus == TickStatus::INTERRUPTED));

        LOG(TRACE) << "Core: Closing all libraries opened during runtime";

        DynamicLoader::execDlCloseAll();

        LOG(TRACE) << "Core:: Cleaning up filesystem";

        utils::Config::terminate();

        LOG(TRACE) << "Core:: All resources released";

        LOG(TRACE) << "SNode.C: Ended ... BYE";
    }

    State EventLoop::state() {
        return eventLoopState;
    }

    void EventLoop::stoponsig(int sig) {
        LOG(INFO) << "Core: Received signal '" << strsignal(sig) << "' (SIG" << utils::system::sigabbrev_np(sig) << " = " << sig << ")";
        stopsig = sig;
        stop();
    }

} // namespace core
