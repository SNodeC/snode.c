/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at> 2026
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include "core/SNodeC.h"
#include "core/pipe/Pipe.h"
#include "core/pipe/PipeSink.h"
#include "core/timer/Timer.h"
#include "support/TestResult.h"

#include <cstdlib>
#include <fcntl.h>

int main(int argc, char* argv[]) {
    tests::support::TestResult testResult;
    int result = tests::support::cTestSkipReturnCode;

    if (tests::support::shouldSkipRootWithoutSNodeCGroup()) {
        tests::support::printRootWithoutSNodeCGroupSkipMessage("PipeCascadingCloseTest");
    } else {
        core::SNodeC::init(argc, argv);

        core::pipe::Pipe firstPipe(O_NONBLOCK | O_CLOEXEC);
        core::pipe::Pipe secondPipe(O_NONBLOCK | O_CLOEXEC);
        auto* first = firstPipe.releaseReadAsSink(4096, core::pipe::PipeSink::TIMEOUT::DISABLE);
        auto* second = secondPipe.releaseReadAsSink(4096, core::pipe::PipeSink::TIMEOUT::DISABLE);
        testResult.expectTrue(first != nullptr && second != nullptr, "both pipe readers are registered");

        int closed = 0;
        bool timedOut = false;
        core::timer::Timer watchdog = core::timer::Timer::singleshotTimer(
            [&]() {
                timedOut = true;
                core::SNodeC::stop();
            },
            {2, 0});

        if (first != nullptr && second != nullptr) {
            first->setOnClosed([&]() {
                first = nullptr;
                ++closed;
                if (second != nullptr) {
                    second->close();
                }
                if (closed == 2) {
                    watchdog.cancel();
                }
            });

            second->setOnClosed([&]() {
                second = nullptr;
                ++closed;
                if (first != nullptr) {
                    first->close();
                }
                if (closed == 2) {
                    watchdog.cancel();
                }
            });

            if (std::getenv("SNODEC_CLOSE_SECOND_FIRST") != nullptr) {
                second->close();
            } else {
                first->close();
            }
        }

        const int loopResult = core::SNodeC::start();
        testResult.expectEqual(0, loopResult, "the framework returns after the last reader is removed");
        testResult.expectTrue(!timedOut, "closing another reader from its peer's close callback needs no forced stop");
        testResult.expectEqual(2, closed, "both close callbacks run exactly once");
        result = testResult.processResult();
    }

    return result;
}
