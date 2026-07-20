/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include "core/pipe/Pipe.h"
#include "core/pipe/PipeSink.h"
#include "core/pipe/PipeSource.h"
#include "support/TestResult.h"

#include <cerrno>
#include <fcntl.h>
#include <system_error>
#include <type_traits>
#include <unistd.h>
#include <utility>

namespace {
    bool isClosed(int fd) {
        errno = 0;
        return ::fcntl(fd, F_GETFD) < 0 && errno == EBADF;
    }
} // namespace

int main() {
    tests::support::TestResult testResult;

    static_assert(std::is_final_v<core::pipe::PipeSource>);
    static_assert(std::is_final_v<core::pipe::PipeSink>);
    static_assert(!std::is_constructible_v<core::pipe::PipeSource, int>);
    static_assert(!std::is_constructible_v<core::pipe::PipeSink, int>);
    static_assert(!std::is_destructible_v<core::pipe::PipeSource>);
    static_assert(!std::is_destructible_v<core::pipe::PipeSink>);

    int destructorReadFd = -1;
    int destructorWriteFd = -1;
    {
        core::pipe::Pipe pipe;
        testResult.expectTrue(pipe.hasReadFd() && pipe.hasWriteFd() && pipe.isValid(), "Pipe owns both pipe2 endpoints after construction");
        testResult.expectEqual(0, pipe.getError(), "valid Pipe records no creation error");
        destructorReadFd = pipe.getReadFd();
        destructorWriteFd = pipe.getWriteFd();

        const int readStatusFlags = ::fcntl(destructorReadFd, F_GETFL);
        const int writeStatusFlags = ::fcntl(destructorWriteFd, F_GETFL);
        const int readDescriptorFlags = ::fcntl(destructorReadFd, F_GETFD);
        const int writeDescriptorFlags = ::fcntl(destructorWriteFd, F_GETFD);
        testResult.expectTrue((readStatusFlags & O_NONBLOCK) == 0 && (writeStatusFlags & O_NONBLOCK) == 0,
                              "default Pipe endpoints are blocking");
        testResult.expectTrue((readDescriptorFlags & FD_CLOEXEC) != 0 && (writeDescriptorFlags & FD_CLOEXEC) != 0,
                              "default Pipe endpoints are close-on-exec");
    }
    testResult.expectTrue(isClosed(destructorReadFd) && isClosed(destructorWriteFd),
                          "Pipe destructor closes every endpoint that was not transferred");

    core::pipe::Pipe moveSource;
    const int movedReadFd = moveSource.getReadFd();
    const int movedWriteFd = moveSource.getWriteFd();
    core::pipe::Pipe moveDestination(std::move(moveSource));
    testResult.expectTrue(!moveSource.hasReadFd() && !moveSource.hasWriteFd() && !moveSource.isValid(),
                          "move construction leaves the source without endpoint ownership");
    testResult.expectTrue(moveDestination.getReadFd() == movedReadFd && moveDestination.getWriteFd() == movedWriteFd,
                          "move construction transfers both endpoint descriptors");

    core::pipe::Pipe assignmentDestination;
    const int replacedReadFd = assignmentDestination.getReadFd();
    const int replacedWriteFd = assignmentDestination.getWriteFd();
    assignmentDestination = std::move(moveDestination);
    testResult.expectTrue(isClosed(replacedReadFd) && isClosed(replacedWriteFd),
                          "move assignment closes the destination's previously owned endpoints");
    testResult.expectTrue(!moveDestination.hasReadFd() && !moveDestination.hasWriteFd() && !moveDestination.isValid(),
                          "move assignment leaves the source without endpoint ownership");
    testResult.expectTrue(assignmentDestination.getReadFd() == movedReadFd && assignmentDestination.getWriteFd() == movedWriteFd,
                          "move assignment transfers both endpoint descriptors");

    const int releasedReadFd = assignmentDestination.releaseReadFd();
    testResult.expectTrue(releasedReadFd == movedReadFd && !assignmentDestination.hasReadFd() && assignmentDestination.hasWriteFd() &&
                              assignmentDestination.isValid(),
                          "releaseReadFd leaves valid partial ownership of only the write endpoint");
    ::close(releasedReadFd);
    assignmentDestination.closeRead();
    assignmentDestination.closeWrite();
    assignmentDestination.closeWrite();
    testResult.expectTrue(isClosed(movedWriteFd), "closeWrite is idempotent and closes the owned write endpoint");
    testResult.expectTrue(!assignmentDestination.hasReadFd() && !assignmentDestination.hasWriteFd() && !assignmentDestination.isValid(),
                          "Pipe is invalid only after neither endpoint remains owned");

    core::pipe::Pipe endpointPipe;
    const int ownedReadFd = endpointPipe.getReadFd();
    const int releasedWriteFd = endpointPipe.releaseWriteFd();
    testResult.expectTrue(releasedWriteFd >= 0 && endpointPipe.hasReadFd() && !endpointPipe.hasWriteFd() && endpointPipe.isValid(),
                          "releaseWriteFd leaves valid partial ownership of only the read endpoint");
    endpointPipe.closeRead();
    endpointPipe.closeRead();
    testResult.expectTrue(isClosed(ownedReadFd), "closeRead is idempotent and closes the owned read endpoint");
    testResult.expectTrue(!isClosed(releasedWriteFd), "Pipe does not close a released write endpoint");
    ::close(releasedWriteFd);

    core::pipe::Pipe transferFailurePipe;
    const int invalidatedReadFd = transferFailurePipe.getReadFd();
    ::close(invalidatedReadFd);
    bool transferFailedObservably = false;
    try {
        static_cast<void>(transferFailurePipe.releaseReadAsSink());
    } catch (const std::system_error& exception) {
        transferFailedObservably = exception.code().value() == EBADF;
    }
    testResult.expectTrue(transferFailedObservably, "nonblocking transfer setup reports fcntl failure to the caller");
    testResult.expectTrue(transferFailurePipe.hasReadFd() && transferFailurePipe.getReadFd() == invalidatedReadFd,
                          "failed nonblocking setup retains read-end ownership in Pipe");
    static_cast<void>(transferFailurePipe.releaseReadFd());

    core::pipe::Pipe creationFailurePipe(-1);
    testResult.expectTrue(!creationFailurePipe.hasReadFd() && !creationFailurePipe.hasWriteFd() && !creationFailurePipe.isValid(),
                          "pipe2 creation failure owns neither endpoint");
    testResult.expectTrue(creationFailurePipe.getError() != 0, "pipe2 creation failure preserves the system error");

    return testResult.processResult();
}
