/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include "core/SNodeC.h"
#include "core/pipe/PipeSink.h"
#include "core/pipe/PipeSource.h"
#include "core/system/unistd.h"
#include "core/timer/Timer.h"
#include "support/TestResult.h"
#include "utils/Timeval.h"

#include <string>

int main(int argc, char* argv[]) {
    tests::support::TestResult testResult;
    if (tests::support::shouldSkipRootWithoutSNodeCGroup()) {
        tests::support::printRootWithoutSNodeCGroupSkipMessage("PipeBoundedQueueTest");
        return tests::support::cTestSkipReturnCode;
    }

    core::SNodeC::init(argc, argv);

    int descriptors[2] = {-1, -1};
    const int pipeResult = core::system::pipe2(descriptors, O_NONBLOCK | O_CLOEXEC);
    testResult.expectEqual(0, pipeResult, "nonblocking test pipe is created");
    if (pipeResult != 0) {
        core::SNodeC::free();
        return testResult.processResult();
    }

    bool sourceClosed = false;
    bool sinkClosed = false;
    bool timedOut = false;
    std::string received;

    auto* source = new core::pipe::PipeSource(descriptors[1], 32);
    auto* sink = new core::pipe::PipeSink(descriptors[0]);

    source->setOnClosed([&sourceClosed]() {
        sourceClosed = true;
    });
    sink->setOnData([&received](const char* data, std::size_t length) {
        received.append(data, length);
    });
    sink->setOnEof([&sinkClosed]() {
        sinkClosed = true;
        core::SNodeC::stop();
    });

    const bool firstQueued = source->trySend("123456789012345678901234", 24);
    const bool overflowQueued = source->trySend("abcdefghijklmnop", 16);
    testResult.expectTrue(firstQueued, "data within the queue limit is accepted");
    testResult.expectTrue(!overflowQueued, "data beyond the queue limit is rejected without growing the queue");
    testResult.expectTrue(source->getQueuedBytes() == 24, "rejected data does not change the queued byte count");
    source->eof();

    [[maybe_unused]] core::timer::Timer watchdog = core::timer::Timer::singleshotTimer(
        [&timedOut]() {
            timedOut = true;
            core::SNodeC::stop();
        },
        utils::Timeval({2, 0}));

    const int startResult = core::SNodeC::start(utils::Timeval({3, 0}));

    testResult.expectTrue(!timedOut, "queued data drains without the watchdog");
    testResult.expectEqual(0, startResult, "pipe test stops the event loop cleanly");
    testResult.expectTrue(received == "123456789012345678901234", "graceful EOF drains all accepted bytes");
    testResult.expectTrue(sourceClosed, "write descriptor closure is observed");
    testResult.expectTrue(sinkClosed, "read descriptor EOF is observed");

    core::SNodeC::free();
    return testResult.processResult();
}
