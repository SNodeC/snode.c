/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include "ai/openai/codex/AppServerClient.h"
#include "ai/openai/codex/stdio/Client.h"
#include "ai/openai/codex/stdio/StdioTransport.h"
#include "core/EventReceiver.h"
#include "core/SNodeC.h"
#include "core/timer/Timer.h"
#include "support/TestResult.h"
#include "utils/Timeval.h"

#include <cerrno>
#include <chrono>
#include <csignal>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <sys/wait.h>
#include <unistd.h>
#include <utility>
#include <vector>

namespace {
    class ForcedPollingClient final : public ai::openai::codex::AppServerClient {
    public:
        ForcedPollingClient(std::string executable, std::vector<std::string> arguments)
            : AppServerClient(
                  std::make_unique<ai::openai::codex::stdio::detail::StdioTransport>(std::move(executable), std::move(arguments), true),
                  {}) {
        }
    };

    std::size_t descriptorCount() {
        std::size_t count = 0;
        for ([[maybe_unused]] const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator("/proc/self/fd")) {
            ++count;
        }
        return count;
    }

    std::filesystem::path uniqueRemovedPath() {
        std::string pathTemplate = (std::filesystem::temp_directory_path() / "snodec-codex-child-XXXXXX").string();
        const int fd = ::mkstemp(pathTemplate.data());
        if (fd >= 0) {
            ::close(fd);
            ::unlink(pathTemplate.c_str());
        }
        return pathTemplate;
    }

    pid_t readPid(const std::filesystem::path& path) {
        std::ifstream input(path);
        pid_t pid = -1;
        input >> pid;
        return pid;
    }
} // namespace

int main(int argc, char* argv[]) {
    tests::support::TestResult testResult;
    if (argc != 2) {
        testResult.expectTrue(false, "one global-shutdown scenario is required");
        return testResult.processResult();
    }

    const std::string scenario = argv[1];
    char* snodeArguments[] = {argv[0], nullptr};
    core::SNodeC::init(1, snodeArguments);

    const std::filesystem::path pidFile = uniqueRemovedPath();
    const std::size_t descriptorsBefore = descriptorCount();
    const bool forcePolling = scenario == "forced-polling-normal" || scenario == "forced-polling-ignore-sigterm";
    const bool ignoreSigterm = scenario == "ignore-sigterm" || scenario == "forced-polling-ignore-sigterm";
    const bool destroyClient = scenario == "destroy-client";
    const std::string fakeMode = ignoreSigterm ? "pidfile-ignore-shutdown" : "pidfile-normal";

    std::unique_ptr<ai::openai::codex::AppServerClient> client;
    if (forcePolling) {
        client = std::make_unique<ForcedPollingClient>(CODEX_FAKE_APP_SERVER, std::vector<std::string>{fakeMode, pidFile.string()});
    } else {
        client =
            std::make_unique<ai::openai::codex::stdio::Client>(CODEX_FAKE_APP_SERVER, std::vector<std::string>{fakeMode, pidFile.string()});
    }

    bool ready = false;
    bool failed = false;
    bool timedOut = false;
    std::chrono::steady_clock::time_point shutdownStartedAt;
    client->setOnStateChanged([&](const ai::openai::codex::StateChange& change) {
        if (change.current == ai::openai::codex::State::Ready) {
            ready = true;
            shutdownStartedAt = std::chrono::steady_clock::now();
            if (destroyClient) {
                static_cast<void>(core::EventReceiver::atNextTick([&client]() {
                    client.reset();
                    core::SNodeC::stop();
                }));
            } else {
                core::SNodeC::stop();
            }
        } else if (change.current == ai::openai::codex::State::Failed) {
            failed = true;
            core::SNodeC::stop();
        }
    });

    [[maybe_unused]] core::timer::Timer watchdog = core::timer::Timer::singleshotTimer(
        [&timedOut]() {
            timedOut = true;
            core::SNodeC::stop();
        },
        utils::Timeval({4, 0}));

    client->start();
    const int startResult = core::SNodeC::start(utils::Timeval({5, 0}));
    const auto completedAt = std::chrono::steady_clock::now();
    const pid_t childPid = readPid(pidFile);
    const std::size_t descriptorsAfter = descriptorCount();

    errno = 0;
    const bool childGone = childPid > 0 && ::kill(childPid, 0) != 0 && errno == ESRCH;
    int status = 0;
    errno = 0;
    const pid_t waitResult = childPid > 0 ? ::waitpid(childPid, &status, WNOHANG) : 0;
    const bool childAlreadyReaped = childPid > 0 && waitResult < 0 && errno == ECHILD;

    testResult.expectEqual(0, startResult, scenario + ": global shutdown completes programmatically");
    testResult.expectTrue(ready && !failed && !timedOut, scenario + ": child reaches Ready before global shutdown");
    testResult.expectTrue(childPid > 0, scenario + ": fake server records its child PID");
    testResult.expectTrue(childGone, scenario + ": child does not survive global shutdown");
    testResult.expectTrue(childAlreadyReaped, scenario + ": child is reaped before global shutdown returns");
    testResult.expectTrue(descriptorsAfter == descriptorsBefore, scenario + ": global shutdown restores the descriptor baseline");
    testResult.expectTrue(completedAt - shutdownStartedAt < std::chrono::seconds(2), scenario + ": shutdown drain remains bounded");
    if (ignoreSigterm) {
        testResult.expectTrue(completedAt - shutdownStartedAt >= std::chrono::milliseconds(450),
                              "uncooperative child receives the EOF, SIGTERM, and SIGKILL phases in order");
    }
    if (destroyClient) {
        testResult.expectTrue(client == nullptr, "public client can be destroyed while its child remains self-retained for reaping");
    } else {
        testResult.expectTrue(client && client->getState() == ai::openai::codex::State::Stopped,
                              scenario + ": internal lifecycle reaches Stopped during the drain");
    }

    std::error_code removeError;
    std::filesystem::remove(pidFile, removeError);
    core::SNodeC::free();
    return testResult.processResult();
}
