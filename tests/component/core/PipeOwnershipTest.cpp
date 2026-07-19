/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include "core/pipe/Pipe.h"
#include "support/TestResult.h"

#include <cerrno>
#include <fcntl.h>
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

    int destructorReadFd = -1;
    int destructorWriteFd = -1;
    {
        core::pipe::Pipe pipe;
        testResult.expectTrue(pipe.isValid(), "Pipe owns both pipe2 endpoints after construction");
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
    testResult.expectTrue(moveSource.getReadFd() < 0 && moveSource.getWriteFd() < 0,
                          "move construction leaves the source without endpoint ownership");
    testResult.expectTrue(moveDestination.getReadFd() == movedReadFd && moveDestination.getWriteFd() == movedWriteFd,
                          "move construction transfers both endpoint descriptors");

    core::pipe::Pipe assignmentDestination;
    const int replacedReadFd = assignmentDestination.getReadFd();
    const int replacedWriteFd = assignmentDestination.getWriteFd();
    assignmentDestination = std::move(moveDestination);
    testResult.expectTrue(isClosed(replacedReadFd) && isClosed(replacedWriteFd),
                          "move assignment closes the destination's previously owned endpoints");
    testResult.expectTrue(moveDestination.getReadFd() < 0 && moveDestination.getWriteFd() < 0,
                          "move assignment leaves the source without endpoint ownership");
    testResult.expectTrue(assignmentDestination.getReadFd() == movedReadFd && assignmentDestination.getWriteFd() == movedWriteFd,
                          "move assignment transfers both endpoint descriptors");

    const int releasedReadFd = assignmentDestination.releaseReadFd();
    testResult.expectTrue(releasedReadFd == movedReadFd && assignmentDestination.getReadFd() < 0,
                          "releaseReadFd transfers read-end ownership explicitly");
    ::close(releasedReadFd);
    assignmentDestination.closeRead();
    assignmentDestination.closeWrite();
    assignmentDestination.closeWrite();
    testResult.expectTrue(isClosed(movedWriteFd), "closeWrite is idempotent and closes the owned write endpoint");

    core::pipe::Pipe endpointPipe;
    const int ownedReadFd = endpointPipe.getReadFd();
    const int releasedWriteFd = endpointPipe.releaseWriteFd();
    testResult.expectTrue(releasedWriteFd >= 0 && endpointPipe.getWriteFd() < 0, "releaseWriteFd transfers write-end ownership explicitly");
    endpointPipe.closeRead();
    endpointPipe.closeRead();
    testResult.expectTrue(isClosed(ownedReadFd), "closeRead is idempotent and closes the owned read endpoint");
    testResult.expectTrue(!isClosed(releasedWriteFd), "Pipe does not close a released write endpoint");
    ::close(releasedWriteFd);

    return testResult.processResult();
}
