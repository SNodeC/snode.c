/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include "core/file/FileReader.h"

#include "core/EventReceiver.h"
#include "core/SNodeC.h"
#include "core/pipe/Sink.h"
#include "core/pipe/Source.h"
#include "core/timer/Timer.h"
#include "support/TestResult.h"
#include "utils/Timeval.h"

#include <algorithm>
#include <array>
#include <cerrno>
#include <cstdlib>
#include <fcntl.h>
#include <filesystem>
#include <functional>
#include <stdexcept>
#include <string>
#include <unistd.h>
#include <utility>

namespace {

    bool isClosed(int fd) {
        errno = 0;
        return ::fcntl(fd, F_GETFD) < 0 && errno == EBADF;
    }

    class CollectingSink final : public core::pipe::Sink {
    public:
        explicit CollectingSink(std::function<void()> onComplete)
            : onComplete(std::move(onComplete)) {
        }

        void resumeSource() {
            if (source != nullptr) {
                source->resume();
            }
        }

        std::string content;
        int error = 0;
        int connectCount = 0;
        int eofCount = 0;

    private:
        void onSourceConnect(core::pipe::Source* connectedSource) override {
            source = connectedSource;
            ++connectCount;
            source->start();
        }

        void onSourceData(const char* chunk, std::size_t chunkLen) override {
            content.append(chunk, chunkLen);
        }

        void onSourceEof() override {
            ++eofCount;
            stop();
            source = nullptr;
            onComplete();
        }

        void onSourceError(int errnum) override {
            error = errnum;
            stop();
            source = nullptr;
            onComplete();
        }

        core::pipe::Source* source = nullptr;
        std::function<void()> onComplete;
    };

} // namespace

int main(int argc, char* argv[]) {
    tests::support::TestResult result;
    if (tests::support::shouldSkipRootWithoutSNodeCGroup()) {
        tests::support::printRootWithoutSNodeCGroupSkipMessage("FileReaderTest");
        return tests::support::cTestSkipReturnCode;
    }

    core::SNodeC::init(argc, argv);

    std::array<char, 64> temporaryDirectoryTemplate{};
    const std::string templateValue = "/tmp/snodec-file-reader-XXXXXX";
    std::copy(templateValue.begin(), templateValue.end(), temporaryDirectoryTemplate.begin());
    char* temporaryDirectoryName = ::mkdtemp(temporaryDirectoryTemplate.data());
    result.expectTrue(temporaryDirectoryName != nullptr, "temporary directory is created");
    if (temporaryDirectoryName == nullptr) {
        core::SNodeC::free();
        return result.processResult();
    }

    const std::filesystem::path temporaryDirectory(temporaryDirectoryName);
    const std::filesystem::path filePath = temporaryDirectory / "payload.txt";
    constexpr std::string_view payload = "descriptor-backed file streaming\n";

    const int writeFd = ::open(filePath.c_str(), O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0600);
    result.expectTrue(writeFd >= 0, "test payload file is created");
    if (writeFd < 0 || ::write(writeFd, payload.data(), payload.size()) != static_cast<ssize_t>(payload.size())) {
        if (writeFd >= 0) {
            ::close(writeFd);
        }
        std::filesystem::remove_all(temporaryDirectory);
        core::SNodeC::free();
        return result.processResult();
    }
    ::close(writeFd);

    const int directoryFd = ::open(temporaryDirectory.c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC);
    result.expectTrue(directoryFd >= 0, "directory descriptor is opened");

    core::file::FileReader* pathReader = core::file::FileReader::open(filePath.string());
    core::file::FileReader* relativeReader = core::file::FileReader::open(directoryFd, "payload.txt", O_RDONLY | O_CLOEXEC);
    core::file::FileReader* absoluteReader = core::file::FileReader::open(-1, filePath.string(), O_RDONLY | O_CLOEXEC);

    const int savedWorkingDirectory = ::open(".", O_RDONLY | O_DIRECTORY | O_CLOEXEC);
    result.expectTrue(savedWorkingDirectory >= 0 && ::chdir(temporaryDirectory.c_str()) == 0,
                      "working directory is changed for AT_FDCWD coverage");
    core::file::FileReader* currentDirectoryReader = core::file::FileReader::open(AT_FDCWD, "payload.txt");
    if (savedWorkingDirectory >= 0) {
        static_cast<void>(::fchdir(savedWorkingDirectory));
        ::close(savedWorkingDirectory);
    }

    const int adoptedFd = ::open(filePath.c_str(), O_RDONLY | O_CLOEXEC);
    core::file::FileReader* adoptedReader = core::file::FileReader::adopt(adoptedFd);

    const int pathFd = pathReader != nullptr ? pathReader->getFd() : -1;
    const int relativeFd = relativeReader != nullptr ? relativeReader->getFd() : -1;
    const int absoluteFd = absoluteReader != nullptr ? absoluteReader->getFd() : -1;
    const int currentDirectoryFd = currentDirectoryReader != nullptr ? currentDirectoryReader->getFd() : -1;

    result.expectTrue(pathReader != nullptr && relativeReader != nullptr && absoluteReader != nullptr &&
                          currentDirectoryReader != nullptr && adoptedReader != nullptr,
                      "path, directory-relative, absolute, AT_FDCWD, and adopted readers are created");

    errno = 0;
    core::file::FileReader* missingReader = core::file::FileReader::open(directoryFd, "missing.txt");
    result.expectTrue(missingReader == nullptr && errno == ENOENT, "failed directory-relative open reports ENOENT without a reader");

    errno = 0;
    core::file::FileReader* modeReader = core::file::FileReader::open(directoryFd, "must-not-be-created.txt", O_CREAT | O_RDONLY);
    result.expectTrue(modeReader == nullptr && errno == EINVAL && !std::filesystem::exists(temporaryDirectory / "must-not-be-created.txt"),
                      "flags requiring a mode are rejected without creating a file");

    errno = 0;
    result.expectTrue(core::file::FileReader::adopt(-1) == nullptr && errno == EBADF, "adopting a negative descriptor fails with EBADF");

    int callbackFd = -1;
    bool callbackExceptionObserved = false;
    try {
        static_cast<void>(core::file::FileReader::open(filePath.string(), [&callbackFd](int fd) {
            callbackFd = fd;
            throw std::runtime_error("callback failure");
        }));
    } catch (const std::runtime_error&) {
        callbackExceptionObserved = true;
    }
    result.expectTrue(callbackExceptionObserved && callbackFd >= 0 && isClosed(callbackFd),
                      "a callback exception cannot leak the newly opened descriptor");

    int compatibilityCallbackFd = -1;
    int compatibilityCallbackErrno = -1;
    core::file::FileReader* compatibilityReader =
        core::file::FileReader::open(filePath.string(), [&compatibilityCallbackFd, &compatibilityCallbackErrno](int fd) {
            compatibilityCallbackFd = fd;
            compatibilityCallbackErrno = errno;
        });
    result.expectTrue(compatibilityReader != nullptr && compatibilityCallbackFd >= 0 && compatibilityCallbackErrno == 0,
                      "the compatibility callback observes a successful path open with errno cleared");
    if (compatibilityReader != nullptr) {
        compatibilityReader->stop();
    }

    const int stoppedFd = ::open(filePath.c_str(), O_RDONLY | O_CLOEXEC);
    core::file::FileReader* stoppedReader = core::file::FileReader::adopt(stoppedFd);
    result.expectTrue(stoppedReader != nullptr, "a reader can adopt a descriptor for pre-start cleanup");
    if (stoppedReader != nullptr) {
        stoppedReader->stop();
        stoppedReader->start();
    }

    int completedReaders = 0;
    const auto onComplete = [&completedReaders]() {
        ++completedReaders;
        if (completedReaders == 5) {
            core::EventReceiver::atNextTick([]() {
                core::EventReceiver::atNextTick([]() {
                    core::SNodeC::stop();
                });
            });
        }
    };

    CollectingSink pathSink(onComplete);
    CollectingSink relativeSink(onComplete);
    CollectingSink absoluteSink(onComplete);
    CollectingSink currentDirectorySink(onComplete);
    CollectingSink adoptedSink(onComplete);

    bool allPipesAccepted = pathReader != nullptr && pathReader->pipe(&pathSink) && relativeReader != nullptr &&
                            absoluteReader != nullptr && currentDirectoryReader != nullptr && adoptedReader != nullptr;
    if (relativeReader != nullptr) {
        relativeReader->suspend();
        allPipesAccepted = relativeReader->pipe(&relativeSink) && allPipesAccepted;
    }
    if (absoluteReader != nullptr) {
        allPipesAccepted = absoluteReader->pipe(&absoluteSink) && allPipesAccepted;
    }
    if (currentDirectoryReader != nullptr) {
        allPipesAccepted = currentDirectoryReader->pipe(&currentDirectorySink) && allPipesAccepted;
    }
    if (adoptedReader != nullptr) {
        allPipesAccepted = adoptedReader->pipe(&adoptedSink) && allPipesAccepted;
    }
    result.expectTrue(allPipesAccepted, "all valid readers attach to pipe sinks");

    core::EventReceiver::atNextTick([&relativeSink]() {
        relativeSink.resumeSource();
    });

    bool timedOut = false;
    [[maybe_unused]] core::timer::Timer watchdog = core::timer::Timer::singleshotTimer(
        [&timedOut]() {
            timedOut = true;
            core::SNodeC::stop();
        },
        utils::Timeval({2, 0}));

    const int startResult = core::SNodeC::start(utils::Timeval({3, 0}));

    result.expectTrue(!timedOut && startResult == 0, "all file readers finish before the watchdog");
    result.expectTrue(completedReaders == 5, "every attached reader reports completion exactly once");
    for (const CollectingSink* sink : {&pathSink, &relativeSink, &absoluteSink, &currentDirectorySink, &adoptedSink}) {
        result.expectTrue(sink->connectCount == 1 && sink->eofCount == 1 && sink->error == 0 && sink->content == payload,
                          "each FileReader streams the complete payload and one EOF");
    }
    result.expectTrue(relativeSink.content == payload, "a source suspended before start waits for resume and then streams normally");

    if (pathReader != nullptr && relativeReader != nullptr && absoluteReader != nullptr && currentDirectoryReader != nullptr &&
        adoptedReader != nullptr) {
        result.expectTrue(isClosed(pathFd) && isClosed(relativeFd) && isClosed(absoluteFd) && isClosed(currentDirectoryFd) &&
                              isClosed(adoptedFd),
                          "FileReader closes path-opened and adopted descriptors after streaming");
    }
    result.expectTrue(stoppedReader == nullptr || isClosed(stoppedFd), "stopping an unstarted FileReader releases its adopted descriptor");
    result.expectTrue(compatibilityReader == nullptr || isClosed(compatibilityCallbackFd),
                      "the compatibility path reader closes its descriptor when stopped before start");

    if (directoryFd >= 0) {
        ::close(directoryFd);
    }
    std::filesystem::remove_all(temporaryDirectory);
    core::SNodeC::free();

    return result.processResult();
}
