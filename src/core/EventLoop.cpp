/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
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

/*
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "core/EventLoop.h"

#include "core/EventMultiplexer.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"
#include "utils/Config.h"
#include "utils/Timeval.h"
#include "utils/system/signal.h"

#include <cerrno>
#include <chrono>
#include <utility>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core {

    int EventLoop::stopsig = 0;
    unsigned long EventLoop::tickCounter = 0;
    core::State EventLoop::eventLoopState = State::LOADED;

    static std::string getTickCounterAsString() {
        std::string tick = std::to_string(EventLoop::getTickCounter());

        if (tick.length() < 13) {
            tick.insert(0, 13 - tick.length(), '0');
        }

        return tick;
    }

    EventLoop::EventLoop()
        : eventMultiplexer(::EventMultiplexer())
        , logScope(logger::LogOrigin::Framework, logger::LogBoundary::System, "core.event-loop") {
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

    logger::BoundaryLogger EventLoop::log() const {
        return logScope.logger(logger::Logger::semanticSink());
    }

    logger::BoundaryLogger
    EventLoop::log(logger::BoundaryLogger::Sink sink, logger::LogLevel threshold, logger::BoundaryLogger::Clock clock) const {
        return logScope.logger(std::move(sink), threshold, std::move(clock));
    }

    State EventLoop::getEventLoopState() {
        return eventLoopState;
    }

    // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays, hicpp-avoid-c-arrays, modernize-avoid-c-arrays)
    bool EventLoop::init(int argc, char* argv[]) {
        struct sigaction sact{};
        sigemptyset(&sact.sa_mask);
        sact.sa_flags = 0;
        sact.sa_handler = SIG_IGN;

        struct sigaction oldPipeAct{};
        sigaction(SIGPIPE, &sact, &oldPipeAct);

        struct sigaction oldIntAct{};
        sigaction(SIGINT, &sact, &oldIntAct);

        struct sigaction oldTermAct{};
        sigaction(SIGTERM, &sact, &oldTermAct);

        struct sigaction oldAlarmAct{};
        sigaction(SIGALRM, &sact, &oldAlarmAct);

        struct sigaction oldHupAct{};
        sigaction(SIGHUP, &sact, &oldHupAct);

        logger::Logger::setTickResolver(core::getTickCounterAsString);

        if (utils::Config::init(argc, argv)) {
            eventLoopState = State::INITIALIZED;

            EventLoop::instance().log().trace("SNode.C: Starting ... HELLO");
        }

        sigaction(SIGPIPE, &oldPipeAct, nullptr);
        sigaction(SIGINT, &oldIntAct, nullptr);
        sigaction(SIGTERM, &oldTermAct, nullptr);
        sigaction(SIGALRM, &oldAlarmAct, nullptr);
        sigaction(SIGHUP, &oldHupAct, nullptr);

        return eventLoopState == State::INITIALIZED;
    }

    TickStatus EventLoop::_tick(const utils::Timeval& timeOut) {
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
            tickStatus = eventMultiplexer.tick(timeOut, oldSet);
        }

        sigprocmask(SIG_SETMASK, &oldSet, nullptr);

        return tickStatus;
    }

    TickStatus EventLoop::tick(const utils::Timeval& timeOut) {
        TickStatus tickStatus = TickStatus::TRACE;

        if (eventLoopState == State::INITIALIZED) {
            struct sigaction sact{};
            sigemptyset(&sact.sa_mask);
            sact.sa_flags = 0;
            sact.sa_handler = SIG_IGN;

            struct sigaction oldPipeAct{};
            sigaction(SIGPIPE, &sact, &oldPipeAct);

            tickStatus = EventLoop::instance()._tick(timeOut);

            sigaction(SIGPIPE, &oldPipeAct, nullptr);
        } else {
            EventLoop::instance().eventMultiplexer.clearEventQueue();
            free();

            const int errnum = errno;
            EventLoop::instance().log().sysError(
                logger::LogLevel::Critical,
                errnum,
                "Core: not initialized: No events will be processed\nCall SNodeC::init(argc, argv) before SNodeC::tick().");
        }

        return tickStatus;
    }

    int EventLoop::start(const utils::Timeval& timeOut) {
        struct sigaction sact{};
        sigemptyset(&sact.sa_mask);
        sact.sa_flags = 0;
        sact.sa_handler = SIG_IGN;

        struct sigaction oldPipeAct{};
        sigaction(SIGPIPE, &sact, &oldPipeAct);

        sact.sa_handler = EventLoop::stoponsig;

        struct sigaction oldIntAct{};
        sigaction(SIGINT, &sact, &oldIntAct);

        struct sigaction oldTermAct{};
        sigaction(SIGTERM, &sact, &oldTermAct);

        struct sigaction oldAlarmAct{};
        sigaction(SIGALRM, &sact, &oldAlarmAct);

        struct sigaction oldHupAct{};
        sigaction(SIGHUP, &sact, &oldHupAct);

        if (eventLoopState == State::INITIALIZED) {
            if (utils::Config::bootstrap()) {
                eventLoopState = State::RUNNING;
                core::TickStatus tickStatus = TickStatus::SUCCESS;

                EventLoop::instance().log().trace("Core::EventLoop: started");

                do {
                    tickStatus = EventLoop::instance()._tick(timeOut);
                } while ((tickStatus == TickStatus::SUCCESS || tickStatus == TickStatus::INTERRUPTED) && eventLoopState == State::RUNNING);

                switch (tickStatus) {
                    case TickStatus::SUCCESS:
                        EventLoop::instance().log().trace("Core::EventLoop: Stopped");
                        break;
                    case TickStatus::NOOBSERVER:
                        EventLoop::instance().log().trace("Core::EventLoop: No Observer");
                        break;
                    case TickStatus::INTERRUPTED:
                        EventLoop::instance().log().trace("Core::EventLoop: Interrupted");
                        break;
                    case TickStatus::TRACE:
                        const int errnum = errno;
                        EventLoop::instance().log().sysError(logger::LogLevel::Critical, errnum, "Core::EventLoop: _tick()");
                        break;
                }
            } else {
                stopsig = -2;
            }
        } else {
            stopsig = -1;
        }

        sigaction(SIGPIPE, &oldPipeAct, nullptr);
        sigaction(SIGTERM, &oldTermAct, nullptr);
        sigaction(SIGALRM, &oldAlarmAct, nullptr);
        sigaction(SIGHUP, &oldHupAct, nullptr);

        free();

        sigaction(SIGINT, &oldIntAct, nullptr);

        return -stopsig;
    }

    void EventLoop::stop() {
        eventLoopState = State::STOPPING;
    }

    void EventLoop::free() {
        std::string signal = "SIG" + utils::system::sigabbrev_np(stopsig);

        if (signal == "SIGUNKNOWN") {
            signal = std::to_string(stopsig);
        }

        if (stopsig != 0) {
            EventLoop::instance().log().trace("Core: Sending signal {} to all DescriptorEventReceivers", signal);

            EventLoop::instance().eventMultiplexer.signal(stopsig);
        }

        eventLoopState = State::STOPPING;

        utils::Timeval timeout = 2;

        EventLoop::instance().log().trace("Core: Terminate all stalled DescriptorEventReceivers");

        EventLoop::instance().eventMultiplexer.terminate();

        core::TickStatus tickStatus = TickStatus::SUCCESS;
        do {
            auto t1 = std::chrono::system_clock::now();
            const utils::Timeval timeoutOp = timeout;

            tickStatus = EventLoop::instance()._tick(timeoutOp);

            auto t2 = std::chrono::system_clock::now();
            const std::chrono::duration<double> seconds = t2 - t1;

            timeout -= seconds.count();
        } while (timeout > 0 && (tickStatus == TickStatus::SUCCESS));

        EventLoop::instance().log().trace("Core: Shutdown config system");

        utils::Config::terminate();

        EventLoop::instance().log().trace("Core: All resources released");

        EventLoop::instance().log().trace("SNode.C: Ended ... BYE");
    }

    void EventLoop::stoponsig(int sig) {
        EventLoop::instance().log().trace(
            "Core: Received signal '{}' (SIG{} = {})", utils::system::strsignal(sig), utils::system::sigabbrev_np(sig), sig);
        stopsig = sig;
        stop();
    }

} // namespace core
