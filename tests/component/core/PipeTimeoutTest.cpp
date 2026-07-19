/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include "core/SNodeC.h"
#include "core/pipe/Pipe.h"
#include "core/pipe/PipeSink.h"
#include "core/timer/Timer.h"
#include "support/TestResult.h"
#include "utils/Timeval.h"

#include <cerrno>
#include <chrono>
#include <fcntl.h>

int main(int argc, char* argv[]) {
    tests::support::TestResult testResult;
    if (tests::support::shouldSkipRootWithoutSNodeCGroup()) {
        tests::support::printRootWithoutSNodeCGroupSkipMessage("PipeTimeoutTest");
        return tests::support::cTestSkipReturnCode;
    }

    core::SNodeC::init(argc, argv);
    core::pipe::Pipe pipe(O_NONBLOCK | O_CLOEXEC);
    testResult.expectTrue(pipe.isValid(), "timeout test pipe is created");
    if (!pipe.isValid()) {
        core::SNodeC::free();
        return testResult.processResult();
    }

    const auto startedAt = std::chrono::steady_clock::now();
    core::pipe::PipeSink* sink = pipe.releaseReadAsSink(core::pipe::PipeSink::DEFAULT_MAX_BYTES_PER_EVENT, utils::Timeval({0, 50000}));
    const int sinkFd = sink->getRegisteredFd();
    bool closed = false;
    bool descriptorClosedBeforeCallback = false;
    bool receivedData = false;
    bool receivedEof = false;
    bool receivedError = false;
    bool watchdogExpired = false;
    std::chrono::milliseconds elapsed{0};

    sink->setOnData([&receivedData]([[maybe_unused]] const char* data, [[maybe_unused]] std::size_t length) {
        receivedData = true;
    });
    sink->setOnEof([&receivedEof]() {
        receivedEof = true;
    });
    sink->setOnError([&receivedError]([[maybe_unused]] int errorNumber) {
        receivedError = true;
    });
    sink->setOnClosed([&]() {
        closed = true;
        elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - startedAt);
        errno = 0;
        descriptorClosedBeforeCallback = ::fcntl(sinkFd, F_GETFD) < 0 && errno == EBADF;
        core::SNodeC::stop();
    });

    [[maybe_unused]] core::timer::Timer watchdog = core::timer::Timer::singleshotTimer(
        [&]() {
            watchdogExpired = true;
            core::SNodeC::stop();
        },
        utils::Timeval({2, 0}));

    const int startResult = core::SNodeC::start(utils::Timeval({3, 0}));
    testResult.expectTrue(!watchdogExpired, "configured PipeSink inactivity timeout closes before the watchdog");
    testResult.expectEqual(0, startResult, "pipe timeout test stops the event loop cleanly");
    testResult.expectTrue(closed && descriptorClosedBeforeCallback,
                          "timeout removes the sink and closes its descriptor before notification");
    testResult.expectTrue(!receivedData && !receivedEof && !receivedError,
                          "inactivity timeout is distinct from data, EOF, and read errors");
    testResult.expectTrue(elapsed >= std::chrono::milliseconds(30) && elapsed < std::chrono::seconds(1),
                          "the configured inactivity interval controls timeout closure");

    core::SNodeC::free();
    return testResult.processResult();
}
