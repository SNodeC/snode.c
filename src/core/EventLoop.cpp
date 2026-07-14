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
#include <vector>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core {

    int EventLoop::stopsig = 0;
    unsigned long EventLoop::tickCounter = 0;
    core::State EventLoop::eventLoopState = State::LOADED;

    namespace {
        std::vector<EventLoop::PreShutdownCallback> preShutdownCallbacks;

        std::string signalName(int signum) {
            std::string signal = "SIG" + utils::system::sigabbrev_np(signum);
            if (signal == "SIGUNKNOWN") {
                signal += "(" + std::to_string(signum) + ")";
            }
            return signal;
        }

        bool logSignalFailure(int result, const char* syscall, const char* phase) {
            if (result == 0) {
                return true;
            }

            const int errnum = errno;
            EventLoop::instance().log().sysError(logger::LogLevel::Error, errnum, "Core::EventLoop {} failed: phase={}", syscall, phase);
            return false;
        }

        bool logSigactionFailure(int result, int signum, const char* phase) {
            if (result == 0) {
                return true;
            }

            const int errnum = errno;
            EventLoop::instance().log().sysError(
                logger::LogLevel::Error, errnum, "Core::EventLoop sigaction failed: signal={} phase={}", signalName(signum), phase);
            return false;
        }

        bool logSigaddsetFailure(int result, int signum, const char* phase) {
            if (result == 0) {
                return true;
            }

            const int errnum = errno;
            EventLoop::instance().log().sysError(
                logger::LogLevel::Error, errnum, "Core::EventLoop sigaddset failed: signal={} phase={}", signalName(signum), phase);
            return false;
        }
    } // namespace

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
        if (logger::LogManager::isFrozen()) {
            const unsigned long generation = logger::LogManager::generation();
            if (!cachedLog_ || cachedLogGeneration_ != generation) {
                cachedLogGeneration_ = generation;
                cachedLog_.emplace(logScope.logger(logger::Logger::semanticSink()));
            }
            return *cachedLog_;
        }

        return logScope.logger(logger::Logger::semanticSink());
    }

    logger::BoundaryLogger
    EventLoop::log(logger::BoundaryLogger::Sink sink, logger::LogLevel threshold, logger::BoundaryLogger::Clock clock) const {
        return logScope.logger(std::move(sink), threshold, std::move(clock));
    }

    State EventLoop::getEventLoopState() {
        return eventLoopState;
    }

    void EventLoop::addPreShutdownCallback(PreShutdownCallback callback) {
        preShutdownCallbacks.push_back(std::move(callback));
    }

    // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays, hicpp-avoid-c-arrays, modernize-avoid-c-arrays)
    bool EventLoop::init(int argc, char* argv[]) {
        struct sigaction sact{};
        logSignalFailure(sigemptyset(&sact.sa_mask), "sigemptyset", "init-install-mask");
        sact.sa_flags = 0;
        sact.sa_handler = SIG_IGN;

        struct sigaction oldPipeAct{};
        logSigactionFailure(sigaction(SIGPIPE, &sact, &oldPipeAct), SIGPIPE, "init-install-ignore");

        struct sigaction oldIntAct{};
        logSigactionFailure(sigaction(SIGINT, &sact, &oldIntAct), SIGINT, "init-install-ignore");

        struct sigaction oldTermAct{};
        logSigactionFailure(sigaction(SIGTERM, &sact, &oldTermAct), SIGTERM, "init-install-ignore");

        struct sigaction oldAlarmAct{};
        logSigactionFailure(sigaction(SIGALRM, &sact, &oldAlarmAct), SIGALRM, "init-install-ignore");

        struct sigaction oldHupAct{};
        logSigactionFailure(sigaction(SIGHUP, &sact, &oldHupAct), SIGHUP, "init-install-ignore");

        logger::Logger::setTickResolver(core::getTickCounterAsString);

        if (utils::Config::init(argc, argv)) {
            eventLoopState = State::INITIALIZED;

            EventLoop::instance().log().trace("SNode.C: Starting ... HELLO");
        }

        logSigactionFailure(sigaction(SIGPIPE, &oldPipeAct, nullptr), SIGPIPE, "init-restore");
        logSigactionFailure(sigaction(SIGINT, &oldIntAct, nullptr), SIGINT, "init-restore");
        logSigactionFailure(sigaction(SIGTERM, &oldTermAct, nullptr), SIGTERM, "init-restore");
        logSigactionFailure(sigaction(SIGALRM, &oldAlarmAct, nullptr), SIGALRM, "init-restore");
        logSigactionFailure(sigaction(SIGHUP, &oldHupAct, nullptr), SIGHUP, "init-restore");

        return eventLoopState == State::INITIALIZED;
    }

    TickStatus EventLoop::_tick(const utils::Timeval& timeOut) {
        TickStatus tickStatus = TickStatus::SUCCESS;

        tickCounter++;

        sigset_t newSet{};
        logSignalFailure(sigemptyset(&newSet), "sigemptyset", "tick-block-mask");
        logSigaddsetFailure(sigaddset(&newSet, SIGINT), SIGINT, "tick-block-mask");
        logSigaddsetFailure(sigaddset(&newSet, SIGTERM), SIGTERM, "tick-block-mask");
        logSigaddsetFailure(sigaddset(&newSet, SIGALRM), SIGALRM, "tick-block-mask");
        logSigaddsetFailure(sigaddset(&newSet, SIGHUP), SIGHUP, "tick-block-mask");

        sigset_t oldSet{};
        logSignalFailure(sigprocmask(SIG_BLOCK, &newSet, &oldSet), "sigprocmask", "tick-block");

        if (eventLoopState == State::RUNNING || eventLoopState == State::STOPPING) {
            tickStatus = eventMultiplexer.tick(timeOut, oldSet);
        }

        logSignalFailure(sigprocmask(SIG_SETMASK, &oldSet, nullptr), "sigprocmask", "tick-restore");

        return tickStatus;
    }

    TickStatus EventLoop::tick(const utils::Timeval& timeOut) {
        TickStatus tickStatus = TickStatus::TRACE;

        if (eventLoopState == State::INITIALIZED) {
            struct sigaction sact{};
            logSignalFailure(sigemptyset(&sact.sa_mask), "sigemptyset", "tick-install-mask");
            sact.sa_flags = 0;
            sact.sa_handler = SIG_IGN;

            struct sigaction oldPipeAct{};
            logSigactionFailure(sigaction(SIGPIPE, &sact, &oldPipeAct), SIGPIPE, "tick-install-ignore");

            tickStatus = EventLoop::instance()._tick(timeOut);

            logSigactionFailure(sigaction(SIGPIPE, &oldPipeAct, nullptr), SIGPIPE, "tick-restore");
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
        logSignalFailure(sigemptyset(&sact.sa_mask), "sigemptyset", "start-install-mask");
        sact.sa_flags = 0;
        sact.sa_handler = SIG_IGN;

        struct sigaction oldPipeAct{};
        logSigactionFailure(sigaction(SIGPIPE, &sact, &oldPipeAct), SIGPIPE, "start-install-ignore");

        sact.sa_handler = EventLoop::stoponsig;

        struct sigaction oldIntAct{};
        logSigactionFailure(sigaction(SIGINT, &sact, &oldIntAct), SIGINT, "start-install-handler");

        struct sigaction oldTermAct{};
        logSigactionFailure(sigaction(SIGTERM, &sact, &oldTermAct), SIGTERM, "start-install-handler");

        struct sigaction oldAlarmAct{};
        logSigactionFailure(sigaction(SIGALRM, &sact, &oldAlarmAct), SIGALRM, "start-install-handler");

        struct sigaction oldHupAct{};
        logSigactionFailure(sigaction(SIGHUP, &sact, &oldHupAct), SIGHUP, "start-install-handler");

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

        logSigactionFailure(sigaction(SIGPIPE, &oldPipeAct, nullptr), SIGPIPE, "start-restore");
        logSigactionFailure(sigaction(SIGTERM, &oldTermAct, nullptr), SIGTERM, "start-restore");
        logSigactionFailure(sigaction(SIGALRM, &oldAlarmAct, nullptr), SIGALRM, "start-restore");
        logSigactionFailure(sigaction(SIGHUP, &oldHupAct, nullptr), SIGHUP, "start-restore");

        free();

        logSigactionFailure(sigaction(SIGINT, &oldIntAct, nullptr), SIGINT, "start-restore");

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

        for (const PreShutdownCallback& callback : preShutdownCallbacks) {
            callback();
        }
        preShutdownCallbacks.clear();

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
