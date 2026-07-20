/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include "core/EventReceiver.h"
#include "core/SNodeC.h"
#include "core/pipe/Pipe.h"
#include "core/pipe/PipeSink.h"
#include "core/timer/Timer.h"
#include "support/TestResult.h"
#include "utils/Timeval.h"

#include <fcntl.h>
#include <string>
#include <unistd.h>

int main(int argc, char* argv[]) {
    tests::support::TestResult testResult;
    if (tests::support::shouldSkipRootWithoutSNodeCGroup()) {
        tests::support::printRootWithoutSNodeCGroupSkipMessage("PipeSinkFairnessTest");
        return tests::support::cTestSkipReturnCode;
    }

    core::SNodeC::init(argc, argv);
    core::pipe::Pipe drainPipe(O_NONBLOCK | O_CLOEXEC);
    core::pipe::Pipe limitedPipe(O_NONBLOCK | O_CLOEXEC);
    const bool completePipes = drainPipe.hasReadFd() && drainPipe.hasWriteFd() && limitedPipe.hasReadFd() && limitedPipe.hasWriteFd();
    testResult.expectTrue(completePipes, "fairness test pipes are created");
    if (!completePipes) {
        core::SNodeC::free();
        return testResult.processResult();
    }

    const int drainWriteFd = drainPipe.releaseWriteFd();
    const int limitedWriteFd = limitedPipe.releaseWriteFd();
    core::pipe::PipeSink* drainSink = drainPipe.releaseReadAsSink(64, core::pipe::PipeSink::TIMEOUT::DISABLE);
    core::pipe::PipeSink* limitedSink = limitedPipe.releaseReadAsSink(4, core::pipe::PipeSink::TIMEOUT::DISABLE);
    testResult.expectTrue(drainSink != nullptr && limitedSink != nullptr, "both read endpoints transfer into PipeSinks");
    if (drainSink == nullptr || limitedSink == nullptr) {
        if (drainSink != nullptr) {
            drainSink->close();
        }
        if (limitedSink != nullptr) {
            limitedSink->close();
        }
        ::close(drainWriteFd);
        ::close(limitedWriteFd);
        core::SNodeC::free();
        return testResult.processResult();
    }

    std::string drained;
    std::string limited;
    int drainCallbacks = 0;
    int limitedCallbacks = 0;
    int eofCount = 0;
    bool nextTickReached = false;
    bool secondDrainReadBeforeNextTick = false;
    bool limitedChunksRespectFairness = true;

    drainSink->setOnData([&](const char* data, std::size_t length) {
        drained.append(data, length);
        ++drainCallbacks;
        if (drainCallbacks == 1) {
            static_cast<void>(::write(drainWriteFd, "efgh", 4));
            ::close(drainWriteFd);
            core::EventReceiver::atNextTick([&nextTickReached]() {
                nextTickReached = true;
            });
        } else if (drainCallbacks == 2) {
            secondDrainReadBeforeNextTick = !nextTickReached;
        }
    });
    drainSink->setOnEof([&]() {
        ++eofCount;
        if (eofCount == 2) {
            core::SNodeC::stop();
        }
    });

    limitedSink->setOnData([&](const char* data, std::size_t length) {
        limited.append(data, length);
        ++limitedCallbacks;
        limitedChunksRespectFairness = limitedChunksRespectFairness && length <= 4;
    });
    limitedSink->setOnEof([&]() {
        ++eofCount;
        if (eofCount == 2) {
            core::SNodeC::stop();
        }
    });

    static_cast<void>(::write(drainWriteFd, "abcd", 4));
    static_cast<void>(::write(limitedWriteFd, "12345678", 8));
    ::close(limitedWriteFd);

    bool timedOut = false;
    [[maybe_unused]] core::timer::Timer watchdog = core::timer::Timer::singleshotTimer(
        [&]() {
            timedOut = true;
            core::SNodeC::stop();
        },
        utils::Timeval({2, 0}));

    const int startResult = core::SNodeC::start(utils::Timeval({3, 0}));
    testResult.expectTrue(!timedOut, "pipe sinks reach EOF without the watchdog");
    testResult.expectEqual(0, startResult, "pipe sink fairness test stops the event loop cleanly");
    testResult.expectTrue(drained == "abcdefgh" && drainCallbacks == 2,
                          "PipeSink repeatedly reads newly available data before returning at EAGAIN");
    testResult.expectTrue(secondDrainReadBeforeNextTick, "read-until-EAGAIN completes within one descriptor event");
    testResult.expectTrue(limited == "12345678" && limitedCallbacks == 2 && limitedChunksRespectFairness,
                          "configured fairness limit bounds each descriptor event without losing data");

    core::SNodeC::free();
    return testResult.processResult();
}
