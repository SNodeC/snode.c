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
#include "core/Shutdown.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"
#include "utils/Config.h"
#include "utils/Timeval.h"
#include "utils/system/signal.h"

#include <algorithm>
#include <array>
#include <cerrno>
#include <chrono>
#include <cstddef>
#include <mutex>
#include <set>
#include <utility>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core {

    static_assert(std::atomic<int>::is_always_lock_free);
    std::atomic<int> EventLoop::stopsig{0};
    thread_local unsigned long EventLoop::tickCounter = 0;
    thread_local core::State EventLoop::eventLoopState = State::LOADED;

    namespace {
        constexpr std::array signals{SIGPIPE, SIGINT, SIGTERM, SIGALRM, SIGHUP};
        using SignalActions = std::array<struct sigaction, signals.size()>;

        // Process services outlive every initialized loop, including loops not started yet.
        std::mutex runtimeMutex;
        std::set<EventLoop*> initializedLoops;
        State applicationState = State::LOADED;
        std::optional<SignalActions> savedSignalActions;

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

        bool logPthreadSigmaskFailure(int error, const char* phase) {
            if (error == 0) {
                return true;
            }

            EventLoop::instance().log().sysError(logger::LogLevel::Error, error, "Core::EventLoop pthread_sigmask failed: phase={}", phase);
            return false;
        }

        SignalActions installSignalActions(void (*handler)(int), const char* phase) {
            struct sigaction action{};
            logSignalFailure(sigemptyset(&action.sa_mask), "sigemptyset", phase);
            SignalActions previous{};
            for (std::size_t i = 0; i < signals.size(); ++i) {
                action.sa_handler = signals[i] == SIGPIPE ? SIG_IGN : handler;
                logSigactionFailure(sigaction(signals[i], &action, &previous[i]), signals[i], phase);
            }
            return previous;
        }

        void restoreSignalActions(const SignalActions& previous, const char* phase) {
            for (std::size_t i = 0; i < signals.size(); ++i) {
                logSigactionFailure(sigaction(signals[i], &previous[i], nullptr), signals[i], phase);
            }
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
        static thread_local EventLoop eventLoop;

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

    // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays, hicpp-avoid-c-arrays, modernize-avoid-c-arrays)
    bool EventLoop::init(int argc, char* argv[]) {
        const std::lock_guard lock(runtimeMutex);
        if (eventLoopState != State::LOADED) {
            return eventLoopState == State::INITIALIZED;
        }
        if (applicationState == State::LOADED) {
            const SignalActions previous = installSignalActions(SIG_IGN, "init-install-ignore");
            logger::Logger::setTickResolver(core::getTickCounterAsString);
            const bool initialized = utils::Config::init(argc, argv);
            restoreSignalActions(previous, "init-restore");
            applicationState = initialized ? State::INITIALIZED : State::STOPPING;
        }
        if (applicationState == State::STOPPING) {
            return false;
        }
        initializedLoops.insert(&instance());
        eventLoopState = State::INITIALIZED;
        instance().log().trace("SNode.C: Starting ... HELLO");
        return true;
    }

    int EventLoop::prepare() {
        const std::lock_guard lock(runtimeMutex);
        if (eventLoopState != State::INITIALIZED) {
            return 1;
        }
        if (applicationState == State::INITIALIZED) {
            savedSignalActions = installSignalActions(stoponsig, "start-install-handler");
            applicationState = utils::Config::bootstrap() ? State::RUNNING : State::STOPPING;
            if (applicationState == State::RUNNING) {
                logger::Logger::startAsync();
            } else {
                logger::Logger::discardPending();
            }
        }
        if (applicationState != State::RUNNING) {
            return 2;
        }
        eventLoopState = State::RUNNING;
        return 0;
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
        const bool signalMaskBlocked = logPthreadSigmaskFailure(pthread_sigmask(SIG_BLOCK, &newSet, &oldSet), "tick-block");

        if ((eventLoopState == State::RUNNING && stopsig.load(std::memory_order_relaxed) == 0) || eventLoopState == State::STOPPING) {
            // A process signal can interrupt another thread's wait. Bound detection in every loop.
            tickStatus = eventMultiplexer.tick(std::min(timeOut, utils::Timeval({0, 100000})), oldSet);
        }

        const int signalNumber = stopsig.load(std::memory_order_relaxed);
        if (signalNumber > 0 && eventLoopState == State::RUNNING) {
            EventLoop::instance().log().trace("Core: Received signal '{}' (SIG{} = {})",
                                              utils::system::strsignal(signalNumber),
                                              utils::system::sigabbrev_np(signalNumber),
                                              signalNumber);
            stop();
        }

        if (signalMaskBlocked) {
            logPthreadSigmaskFailure(pthread_sigmask(SIG_SETMASK, &oldSet, nullptr), "tick-restore");
        }

        return tickStatus;
    }

    TickStatus EventLoop::tick(const utils::Timeval& timeOut) {
        TickStatus tickStatus = TickStatus::TRACE;

        if (eventLoopState == State::RUNNING || prepare() == 0) {
            tickStatus = EventLoop::instance()._tick(timeOut);
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
        const int result = prepare();
        if (result == 0) {
            TickStatus tickStatus = TickStatus::SUCCESS;
            instance().log().trace("Core::EventLoop: started");
            do {
                tickStatus = instance()._tick(timeOut);
            } while ((tickStatus == TickStatus::SUCCESS || tickStatus == TickStatus::INTERRUPTED) && eventLoopState == State::RUNNING);

            if (tickStatus == TickStatus::TRACE) {
                const int errnum = errno;
                instance().log().sysError(logger::LogLevel::Critical, errnum, "Core::EventLoop: _tick()");
            }
        }
        free();
        return result != 0 ? result : -stopsig.load(std::memory_order_relaxed);
    }

    bool EventLoop::reconfigure() {
        if (eventLoopState != State::RUNNING) {
            instance().log().error("Configuration can only be reapplied while SNode.C is running");
            return false;
        }
        const std::lock_guard lock(runtimeMutex);
        if (initializedLoops.size() != 1) {
            instance().log().error("Configuration can only be reapplied with one initialized event loop");
            return false;
        }
        return utils::Config::reconfigure();
    }

    void EventLoop::stop() {
        eventLoopState = State::STOPPING;
    }

    void EventLoop::free() {
        {
            const std::lock_guard lock(runtimeMutex);
            if (!initializedLoops.contains(&instance())) {
                return;
            }
            logger::Logger::startAsync();
        }

        const int signalNumber = stopsig.load(std::memory_order_relaxed);
        const ShutdownReason reason = signalNumber > 0                   ? ShutdownReason::Signal
                                      : eventLoopState == State::RUNNING ? ShutdownReason::NoObserver
                                                                         : ShutdownReason::Requested;
        std::string signal = "SIG" + utils::system::sigabbrev_np(signalNumber);

        if (signal == "SIGUNKNOWN") {
            signal = std::to_string(signalNumber);
        }

        EventLoop& eventLoop = EventLoop::instance();
        eventLoopState = State::STOPPING;

        if (reason == ShutdownReason::Signal) {
            eventLoop.log().trace("Core: Graceful shutdown after signal {}", signal);
        } else {
            eventLoop.log().trace("Core: Graceful shutdown");
        }

        eventLoop.eventMultiplexer.shutdown({reason, signalNumber > 0 ? signalNumber : 0});

        utils::Timeval timeout = 2;
        TickStatus tickStatus = TickStatus::SUCCESS;
        do {
            const auto startedAt = std::chrono::steady_clock::now();
            const utils::Timeval timeoutOp =
                eventLoop.eventMultiplexer.hasPendingResources() ? std::min(timeout, utils::Timeval({0, 100000})) : utils::Timeval();

            tickStatus = eventLoop._tick(timeoutOp);

            const auto completedAt = std::chrono::steady_clock::now();
            timeout -= std::chrono::duration<double>(completedAt - startedAt).count();
        } while (timeout > 0 && eventLoop.eventMultiplexer.hasPendingResources() &&
                 (tickStatus == TickStatus::SUCCESS || tickStatus == TickStatus::INTERRUPTED));

        eventLoop.log().trace("Core: Terminate remaining DescriptorEventReceivers and timers");
        eventLoop.eventMultiplexer.terminate();
        eventLoop.eventMultiplexer.clearEventQueue();

        std::unique_lock lock(runtimeMutex);
        initializedLoops.erase(&eventLoop);
        if (initializedLoops.empty()) {
            applicationState = State::STOPPING;
            lock.unlock(); // Configuration destruction can invoke application callbacks.
            eventLoop.log().trace("Core: Shutdown config system");
            utils::Config::terminate();
            eventLoop.log().trace("Core: All resources released");
            eventLoop.log().trace("SNode.C: Ended ... BYE");
            logger::Logger::shutdown();
            if (savedSignalActions) {
                restoreSignalActions(*savedSignalActions, "shutdown-restore");
                savedSignalActions.reset();
            }
        }
    }

    void EventLoop::stoponsig(int sig) {
        stopsig.store(sig, std::memory_order_relaxed);
    }

} // namespace core
