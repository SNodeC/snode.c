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

#include <csignal>
#include <string>

namespace {

    class ShutdownReceiver final : public core::eventreceiver::ReadEventReceiver {
    public:
        ShutdownReceiver(int readFd, int writeFd, int& callbackCount, core::ShutdownContext& receivedContext)
            : core::eventreceiver::ReadEventReceiver("shutdown notification test", TIMEOUT::DISABLE)
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
        const bool signalShutdown = scenario == "signal";
        int callbackCount = 0;
        core::ShutdownContext receivedContext;

        char* snodeArguments[] = {argv[0], nullptr};
        core::SNodeC::init(1, snodeArguments);

        core::pipe::Pipe pipe(O_CLOEXEC | O_NONBLOCK);
        if (pipe.hasReadFd() && pipe.hasWriteFd()) {
            static_cast<void>(new ShutdownReceiver(pipe.releaseReadFd(), pipe.releaseWriteFd(), callbackCount, receivedContext));
        }

        core::timer::Timer stopTimer = core::timer::Timer::singleshotTimer(
            [signalShutdown]() {
                if (signalShutdown) {
                    static_cast<void>(::kill(::getpid(), SIGTERM));
                } else {
                    core::SNodeC::stop();
                }
            },
            utils::Timeval({0, 1000}));

        const int startResult = core::SNodeC::start(utils::Timeval({1, 0}));

        testResult.expectEqual(1, callbackCount, "active receiver receives shutdown exactly once");
        testResult.expectTrue(receivedContext.reason == (signalShutdown ? core::ShutdownReason::Signal : core::ShutdownReason::Requested),
                              "shutdown reason identifies the trigger");
        testResult.expectEqual(signalShutdown ? SIGTERM : 0, receivedContext.signal, "signal number is preserved only for signal shutdown");
        testResult.expectEqual(signalShutdown ? -SIGTERM : 0, startResult, "event loop returns the triggering signal");

        core::SNodeC::free();
        result = testResult.processResult();
    }

    return result;
}
