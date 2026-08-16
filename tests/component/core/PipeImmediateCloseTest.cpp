/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include "core/SNodeC.h"
#include "core/pipe/Pipe.h"
#include "core/pipe/PipeSource.h"
#include "core/timer/Timer.h"
#include "log/SemanticLogger.h"
#include "support/TestResult.h"
#include "utils/Timeval.h"

#include <cerrno>
#include <fcntl.h>
#include <string>
#include <unistd.h>
#include <utility>
#include <vector>

int main(int argc, char* argv[]) {
    tests::support::TestResult testResult;
    if (tests::support::shouldSkipRootWithoutSNodeCGroup()) {
        tests::support::printRootWithoutSNodeCGroupSkipMessage("PipeImmediateCloseTest");
        return tests::support::cTestSkipReturnCode;
    }

    core::SNodeC::init(argc, argv);
    core::pipe::Pipe pipe(O_NONBLOCK | O_CLOEXEC, "immediate-close-pipe");
    const bool completePipe = pipe.hasReadFd() && pipe.hasWriteFd();
    testResult.expectTrue(completePipe, "immediate-close test pipe is created");
    if (!completePipe) {
        core::SNodeC::free();
        return testResult.processResult();
    }

    core::pipe::PipeSource* source = pipe.releaseWriteAsSource(64, core::pipe::PipeSource::TIMEOUT::DISABLE);
    testResult.expectTrue(source != nullptr, "write endpoint transfers into a PipeSource");
    if (source == nullptr) {
        core::SNodeC::free();
        return testResult.processResult();
    }

    std::vector<logger::LogRecord> scopeRecords;
    source
        ->log(
            [&scopeRecords](logger::LogRecord record) {
                scopeRecords.push_back(std::move(record));
            },
            logger::LogLevel::Trace)
        .info("pipe scope probe");
    testResult.expectEqual(static_cast<std::size_t>(1), scopeRecords.size(), "PipeSource exposes one captured semantic scope record");
    if (!scopeRecords.empty()) {
        const logger::LogRecord& record = scopeRecords.front();
        testResult.expectTrue(record.origin == logger::LogOrigin::Framework && record.boundary == logger::LogBoundary::Connection,
                              "PipeSource uses the framework connection boundary");
        testResult.expectTrue(record.component == "core.pipe", "PipeSource uses the core.pipe component");
        testResult.expectTrue(record.instance && *record.instance == "immediate-close-pipe",
                              "PipeSource inherits the configured Pipe instance name");
        testResult.expectTrue(record.connection && !record.connection->empty(), "PipeSource inherits a logical Pipe connection ID");
    }
    const int readFd = pipe.getReadFd();
    bool closed = false;
    int closureCount = 0;
    source->setOnClosed([&]() {
        closed = true;
        ++closureCount;
        core::SNodeC::stop();
    });

    testResult.expectTrue(source->send("queued-data-that-must-be-discarded"), "data is accepted before immediate close");
    source->close();
    testResult.expectEqual(static_cast<std::size_t>(0), source->getQueuedBytes(), "close discards every queued byte immediately");
    testResult.expectTrue(!source->send("rejected"), "close rejects all subsequent data");

    bool timedOut = false;
    [[maybe_unused]] core::timer::Timer watchdog = core::timer::Timer::singleshotTimer(
        [&]() {
            timedOut = true;
            core::SNodeC::stop();
        },
        utils::Timeval({2, 0}));

    const int startResult = core::SNodeC::start(utils::Timeval({3, 0}));
    char byte = 0;
    const ssize_t readResult = ::read(readFd, &byte, 1);

    testResult.expectTrue(!timedOut, "close removes the source without waiting for pipe writability");
    testResult.expectEqual(0, startResult, "immediate-close test stops the event loop cleanly");
    testResult.expectTrue(closed && closureCount == 1, "close reports descriptor closure exactly once");
    testResult.expectTrue(readResult == 0, "no discarded queued data reaches the read endpoint");

    core::SNodeC::free();
    return testResult.processResult();
}
