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
#include "core/pipe/PipeSource.h"
#include "core/timer/Timer.h"
#include "support/TestResult.h"
#include "utils/Timeval.h"

#include <cerrno>
#include <fcntl.h>
#include <string>

int main(int argc, char* argv[]) {
    tests::support::TestResult testResult;
    if (tests::support::shouldSkipRootWithoutSNodeCGroup()) {
        tests::support::printRootWithoutSNodeCGroupSkipMessage("PipeBoundedQueueTest");
        return tests::support::cTestSkipReturnCode;
    }

    core::SNodeC::init(argc, argv);

    core::pipe::Pipe pipe;
    const bool completePipe = pipe.hasReadFd() && pipe.hasWriteFd();
    testResult.expectTrue(completePipe, "default blocking test pipe is created with both endpoints");
    if (!completePipe) {
        core::SNodeC::free();
        return testResult.processResult();
    }
    testResult.expectTrue((::fcntl(pipe.getReadFd(), F_GETFL) & O_NONBLOCK) == 0 && (::fcntl(pipe.getWriteFd(), F_GETFL) & O_NONBLOCK) == 0,
                          "default Pipe endpoints begin in blocking mode");

    bool sourceClosed = false;
    bool sinkClosed = false;
    bool timedOut = false;
    int sourceErrorCount = 0;
    std::string received;

    auto* source = pipe.releaseWriteAsSource(32, core::pipe::PipeSource::TIMEOUT::DISABLE);
    auto* sink = pipe.releaseReadAsSink(core::pipe::PipeSink::DEFAULT_MAX_BYTES_PER_EVENT, core::pipe::PipeSink::TIMEOUT::DISABLE);
    testResult.expectTrue(source != nullptr && sink != nullptr, "both default Pipe endpoints transfer into event receivers");
    if (source == nullptr || sink == nullptr) {
        if (source != nullptr) {
            source->close();
        }
        if (sink != nullptr) {
            sink->close();
        }
        core::SNodeC::free();
        return testResult.processResult();
    }
    const int sourceFd = source->getRegisteredFd();
    const int sinkFd = sink->getRegisteredFd();
    testResult.expectTrue((::fcntl(sourceFd, F_GETFL) & O_NONBLOCK) != 0 && (::fcntl(sinkFd, F_GETFL) & O_NONBLOCK) != 0,
                          "Pipe transfer makes both parent event-loop endpoints nonblocking");
    bool sourceDescriptorClosedBeforeCallback = false;
    bool sinkDescriptorClosedBeforeCallback = false;

    source->setOnClosed([&sourceClosed, &sourceDescriptorClosedBeforeCallback, sourceFd]() {
        sourceClosed = true;
        errno = 0;
        sourceDescriptorClosedBeforeCallback = ::fcntl(sourceFd, F_GETFD) < 0 && errno == EBADF;
    });
    source->setOnError([&sourceErrorCount]([[maybe_unused]] int errorNumber) {
        ++sourceErrorCount;
    });
    sink->setOnData([&received](const char* data, std::size_t length) {
        received.append(data, length);
    });
    sink->setOnEof([&sinkClosed]() {
        sinkClosed = true;
        core::SNodeC::stop();
    });
    sink->setOnClosed([&sinkDescriptorClosedBeforeCallback, sinkFd]() {
        errno = 0;
        sinkDescriptorClosedBeforeCallback = ::fcntl(sinkFd, F_GETFD) < 0 && errno == EBADF;
    });

    const bool emptyQueued = source->send("", 0);
    const bool firstQueued = source->send("123456789012345678901234", 24);
    const bool overflowQueued = source->send("abcdefghijklmnop", 16);
    testResult.expectTrue(emptyQueued, "zero-length input is accepted while the source is open");
    testResult.expectTrue(firstQueued, "data within the queue limit is accepted");
    testResult.expectTrue(!overflowQueued, "data beyond the queue limit is rejected without growing the queue");
    testResult.expectEqual(0, sourceErrorCount, "bounded queue rejection is reported only through the send return value");
    testResult.expectTrue(source->getQueuedBytes() == 24, "rejected data does not change the queued byte count");
    source->eof();
    testResult.expectTrue(!source->send("rejected-after-eof"), "EOF rejects all new data");

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
    testResult.expectTrue(sourceDescriptorClosedBeforeCallback, "source closure callback runs after physical descriptor closure");
    testResult.expectTrue(sinkDescriptorClosedBeforeCallback, "sink closure callback runs after physical descriptor closure");

    core::SNodeC::free();
    return testResult.processResult();
}
