/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include "core/SNodeC.h"
#include "core/Shutdown.h"
#include "core/eventreceiver/ReadEventReceiver.h"
#include "core/pipe/Pipe.h"
#include "core/system/unistd.h"
#include "core/timer/Timer.h"
#include "support/TestResult.h"
#include "utils/Timeval.h"

#include <chrono>
#include <csignal>
#include <string>
#include <sys/wait.h>
#include <unistd.h>

namespace {

    class ShutdownReceiver final : public core::eventreceiver::ReadEventReceiver {
    public:
        ShutdownReceiver(int readFd, int writeFd, int& callbackCount, core::ShutdownContext& receivedContext)
            : core::eventreceiver::ReadEventReceiver("shutdown notification test",
                                                     logger::LogScope{logger::LogOrigin::Framework,
                                                                      logger::LogBoundary::System,
                                                                      "core.eventreceiver",
                                                                      "shutdown notification test read",
                                                                      logger::LogRole::Unknown,
                                                                      {}},
                                                     TIMEOUT::DISABLE)
            , writeFd(writeFd)
            , callbackCount(callbackCount)
            , receivedContext(receivedContext) {
            ReadEventReceiver::enable(readFd);
        }

    private:
        ~ShutdownReceiver() override {
            core::system::close(getRegisteredFd());
            core::system::close(writeFd);
        }

        void readEvent() override {
        }

        void unobservedEvent() override {
            delete this;
        }

        void shutdownEvent(const core::ShutdownContext& context) override {
            ++callbackCount;
            receivedContext = context;
            ReadEventReceiver::disable();
        }

        int writeFd;
        int& callbackCount;
        core::ShutdownContext& receivedContext;
    };

} // namespace

int main(int argc, char* argv[]) {
    tests::support::TestResult testResult;
    int result = tests::support::cTestSkipReturnCode;

    if (tests::support::shouldSkipRootWithoutSNodeCGroup()) {
        tests::support::printRootWithoutSNodeCGroupSkipMessage("ShutdownReceiverNotificationTest");
    } else {
        const std::string scenario = argc > 1 ? argv[1] : "requested";
        const bool signalWhileWaiting = scenario == "signal-wait";
        const bool signalShutdown = scenario == "signal" || signalWhileWaiting;
        const int shutdownSignal = signalWhileWaiting ? SIGINT : signalShutdown ? SIGTERM : 0;
        int callbackCount = 0;
        core::ShutdownContext receivedContext;

        char* snodeArguments[] = {argv[0], nullptr};
        core::SNodeC::init(1, snodeArguments);

        core::pipe::Pipe pipe(O_CLOEXEC | O_NONBLOCK);
        if (pipe.hasReadFd() && pipe.hasWriteFd()) {
            static_cast<void>(new ShutdownReceiver(pipe.releaseReadFd(), pipe.releaseWriteFd(), callbackCount, receivedContext));
        }

        pid_t signalerPid = -1;
        core::timer::Timer stopTimer;
        if (signalWhileWaiting) {
            signalerPid = ::fork();
            testResult.expectTrue(signalerPid >= 0, "SIGINT sender process starts");
            if (signalerPid == 0) {
                ::usleep(100000);
                const int killResult = ::kill(::getppid(), SIGINT);
                ::_exit(killResult == 0 ? 0 : 1);
            }
        } else {
            stopTimer = core::timer::Timer::singleshotTimer(
                [signalShutdown]() {
                    if (signalShutdown) {
                        static_cast<void>(::kill(::getpid(), SIGTERM));
                    } else {
                        core::SNodeC::stop();
                    }
                },
                utils::Timeval({0, 1000}));
        }

        const auto startTime = std::chrono::steady_clock::now();
        const int startResult = core::SNodeC::start(signalWhileWaiting ? utils::Timeval({5, 0}) : utils::Timeval({1, 0}));
        const auto elapsed = std::chrono::steady_clock::now() - startTime;

        if (signalerPid > 0) {
            int signalerStatus = 0;
            testResult.expectEqual(signalerPid, ::waitpid(signalerPid, &signalerStatus, 0), "SIGINT sender process is reaped");
            testResult.expectTrue(WIFEXITED(signalerStatus) && WEXITSTATUS(signalerStatus) == 0, "SIGINT sender process succeeds");
            testResult.expectTrue(elapsed < std::chrono::seconds(2), "SIGINT interrupts the waiting event-loop thread immediately");
        }

        testResult.expectEqual(1, callbackCount, "active receiver receives shutdown exactly once");
        testResult.expectTrue(receivedContext.reason == (signalShutdown ? core::ShutdownReason::Signal : core::ShutdownReason::Requested),
                              "shutdown reason identifies the trigger");
        testResult.expectEqual(shutdownSignal, receivedContext.signal, "signal number is preserved only for signal shutdown");
        testResult.expectEqual(-shutdownSignal, startResult, "event loop returns the triggering signal");

        core::SNodeC::free();
        result = testResult.processResult();
    }

    return result;
}
