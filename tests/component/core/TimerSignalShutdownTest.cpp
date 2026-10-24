/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2026
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include "core/SNodeC.h"
#include "core/timer/Timer.h"
#include "support/TestResult.h"

#include <array>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <pthread.h>
#include <string>
#include <sys/wait.h>
#include <unistd.h>

int main(int argc, char* argv[]) {
    tests::support::TestResult result;
    int exitCode = tests::support::cTestSkipReturnCode;

    if (tests::support::shouldSkipRootWithoutSNodeCGroup()) {
        tests::support::printRootWithoutSNodeCGroupSkipMessage("TimerSignalShutdownTest");
    } else {
        const int shutdownSignal = argc > 1 ? std::atoi(argv[1]) : SIGTERM;
        const bool initiallyBlocked = argc > 2 && std::string(argv[2]) == "blocked";
        constexpr std::array shutdownSignals{SIGINT, SIGTERM, SIGALRM, SIGHUP};
        sigset_t originalMask{};
        result.expectEqual(0, ::pthread_sigmask(SIG_SETMASK, nullptr, &originalMask), "capture the caller's signal mask");
        sigset_t incomingMask = originalMask;
        for (const int signal : shutdownSignals) {
            result.expectEqual(0,
                               initiallyBlocked ? ::sigaddset(&incomingMask, signal) : ::sigdelset(&incomingMask, signal),
                               "set incoming shutdown-signal mask");
        }
        result.expectEqual(0, ::sigaddset(&incomingMask, SIGUSR1), "block an unrelated signal");
        result.expectEqual(0, ::pthread_sigmask(SIG_SETMASK, &incomingMask, nullptr), "install the incoming signal mask");

        char* arguments[] = {argv[0], nullptr};
        core::SNodeC::init(1, arguments);

        bool callbackRan = false;
        const auto probe = core::timer::Timer::singleshotTimer(
            [&]() {
                callbackRan = true;
                sigset_t callbackMask{};
                result.expectEqual(0, ::pthread_sigmask(SIG_SETMASK, nullptr, &callbackMask), "read the callback's signal mask");
                for (const int signal : shutdownSignals) {
                    result.expectEqual(1, ::sigismember(&callbackMask, signal), "shutdown signals stay blocked during callbacks");
                }
                result.expectEqual(1, ::sigismember(&callbackMask, SIGUSR1), "an unrelated signal stays blocked during callbacks");
            },
            {0, 1000});
        const auto keepAlive = core::timer::Timer::singleshotTimer(
            []() {
            },
            {30, 0});

        const pid_t signaler = ::fork();
        result.expectTrue(signaler >= 0, "start the external signal sender");
        if (signaler == 0) {
            ::usleep(100000);
            ::_exit(::kill(::getppid(), shutdownSignal) == 0 ? 0 : 1);
        } else if (signaler > 0) {
            const auto startedAt = std::chrono::steady_clock::now();
            result.expectEqual(-shutdownSignal, core::SNodeC::start(), "timer-only shutdown preserves the triggering signal");
            result.expectTrue(std::chrono::steady_clock::now() - startedAt < std::chrono::seconds(2),
                              "shutdown interrupts the long timer wait");
            result.expectTrue(callbackRan, "a timer callback ran before shutdown");

            int status = 0;
            result.expectEqual(signaler, ::waitpid(signaler, &status, 0), "reap the signal sender");
            result.expectTrue(WIFEXITED(status) && WEXITSTATUS(status) == 0, "the external signal sender succeeded");

            sigset_t restoredMask{};
            result.expectEqual(0, ::pthread_sigmask(SIG_SETMASK, nullptr, &restoredMask), "read the restored signal mask");
            for (int signal = 1; signal < NSIG; ++signal) {
                result.expectEqual(::sigismember(&incomingMask, signal),
                                   ::sigismember(&restoredMask, signal),
                                   "shutdown restores the complete incoming signal mask");
            }
            sigset_t pending{};
            result.expectEqual(0, ::sigpending(&pending), "read pending signals after shutdown");
            result.expectEqual(0, ::sigismember(&pending, shutdownSignal), "the shutdown signal was consumed");
        } else {
            core::SNodeC::free();
        }

        result.expectEqual(0, ::pthread_sigmask(SIG_SETMASK, &originalMask, nullptr), "restore the test caller's signal mask");
        exitCode = result.processResult();
    }

    return exitCode;
}
