/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include "ai/openai/codex/stdio/Client.h"
#include "core/SNodeC.h"
#include "core/timer/Timer.h"
#include "support/TestResult.h"
#include "utils/Timeval.h"

#include <cerrno>
#include <fcntl.h>
#include <filesystem>
#include <string>
#include <unistd.h>
#include <utility>
#include <vector>

namespace {
    std::size_t descriptorCount() {
        std::size_t count = 0;
        for ([[maybe_unused]] const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator("/proc/self/fd")) {
            ++count;
        }
        return count;
    }

    std::vector<int> selectedDescriptors(const std::string& scenario) {
        if (scenario == "stdin") {
            return {STDIN_FILENO};
        }
        if (scenario == "stdout") {
            return {STDOUT_FILENO};
        }
        if (scenario == "stderr") {
            return {STDERR_FILENO};
        }
        return {STDIN_FILENO, STDOUT_FILENO, STDERR_FILENO};
    }
} // namespace

int main(int argc, char* argv[]) {
    tests::support::TestResult testResult;
    if (argc != 2) {
        testResult.expectTrue(false, "one descriptor-collision scenario is required");
        return testResult.processResult();
    }

    const std::string scenario = argv[1];
    char* snodeArguments[] = {argv[0], nullptr};
    core::SNodeC::init(1, snodeArguments);

    std::vector<int> unrelatedDescriptors;
    for (int index = 0; index < 6; ++index) {
        unrelatedDescriptors.push_back(::open("/dev/null", O_RDONLY | O_CLOEXEC));
    }
    const std::size_t descriptorsBefore = descriptorCount();
    const std::vector<int> descriptorsToClose = selectedDescriptors(scenario);
    std::vector<std::pair<int, int>> standardBackups;
    bool backupSucceeded = true;
    bool restoreSucceeded = true;
    bool ready = false;
    bool failed = false;
    bool timedOut = false;

    const auto restoreStandards = [&]() {
        for (const auto& [standardFd, backupFd] : standardBackups) {
            if (::dup2(backupFd, standardFd) < 0) {
                restoreSucceeded = false;
            }
            ::close(backupFd);
        }
        standardBackups.clear();
    };

    ai::openai::codex::stdio::Client client(CODEX_FAKE_APP_SERVER, {"normal"});
    client.setOnStateChanged([&](const ai::openai::codex::StateChange& change) {
        if (change.current == ai::openai::codex::State::Starting) {
            for (const int standardFd : descriptorsToClose) {
                const int backupFd = ::fcntl(standardFd, F_DUPFD_CLOEXEC, 64);
                if (backupFd < 0) {
                    backupSucceeded = false;
                    continue;
                }
                standardBackups.emplace_back(standardFd, backupFd);
                if (::close(standardFd) != 0) {
                    backupSucceeded = false;
                }
            }
        } else if (change.current == ai::openai::codex::State::Initializing) {
            restoreStandards();
        } else if (change.current == ai::openai::codex::State::Ready) {
            ready = true;
            client.stop();
        } else if (change.current == ai::openai::codex::State::Failed) {
            failed = true;
            restoreStandards();
            client.stop();
        } else if (change.current == ai::openai::codex::State::Stopped) {
            core::SNodeC::stop();
        }
    });

    [[maybe_unused]] core::timer::Timer watchdog = core::timer::Timer::singleshotTimer(
        [&timedOut]() {
            timedOut = true;
            core::SNodeC::stop();
        },
        utils::Timeval({4, 0}));

    client.start();
    const int startResult = core::SNodeC::start(utils::Timeval({5, 0}));
    restoreStandards();
    const std::size_t descriptorsAfter = descriptorCount();

    bool unrelatedDescriptorsIntact = true;
    for (const int fd : unrelatedDescriptors) {
        if (fd < 0 || ::fcntl(fd, F_GETFD) < 0) {
            unrelatedDescriptorsIntact = false;
        }
    }

    testResult.expectEqual(0, startResult, scenario + ": lifecycle completes with selected standard descriptors initially closed");
    testResult.expectTrue(backupSucceeded && restoreSucceeded, scenario + ": parent standard descriptors are backed up and restored");
    testResult.expectTrue(ready && !failed && !timedOut, scenario + ": child standard descriptors remain blocking and protocol-capable");
    testResult.expectTrue(unrelatedDescriptorsIntact, scenario + ": nearby unrelated descriptors are not overwritten or closed");
    testResult.expectTrue(descriptorsAfter == descriptorsBefore, scenario + ": Codex descriptors return to baseline after restoration");

    for (const int fd : unrelatedDescriptors) {
        if (fd >= 0) {
            ::close(fd);
        }
    }
    core::SNodeC::free();
    return testResult.processResult();
}
