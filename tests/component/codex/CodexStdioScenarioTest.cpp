/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include "ai/openai/codex/AppServerClient.h"
#include "ai/openai/codex/stdio/Client.h"
#include "core/SNodeC.h"
#include "core/pipe/PipeSource.h"
#include "core/timer/Timer.h"
#include "support/TestResult.h"
#include "utils/Timeval.h"

#include <cerrno>
#include <chrono>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace {
    using CodexState = ai::openai::codex::State;

    std::size_t descriptorCount() {
        std::size_t count = 0;
        for ([[maybe_unused]] const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator("/proc/self/fd")) {
            ++count;
        }
        return count;
    }

    bool containsState(const std::vector<CodexState>& states, CodexState expected) {
        for (const CodexState state : states) {
            if (state == expected) {
                return true;
            }
        }
        return false;
    }
} // namespace

int main(int argc, char* argv[]) {
    tests::support::TestResult testResult;
    if (argc != 2) {
        testResult.expectTrue(false, "exactly one scenario argument is required");
        return testResult.processResult();
    }

    if (tests::support::shouldSkipRootWithoutSNodeCGroup()) {
        tests::support::printRootWithoutSNodeCGroupSkipMessage("CodexStdioScenarioTest");
        return tests::support::cTestSkipReturnCode;
    }

    const std::string scenario = argv[1];
    char* snodeArguments[] = {argv[0], nullptr};
    core::SNodeC::init(1, snodeArguments);

    const std::size_t descriptorsBefore = descriptorCount();
    const auto startedAt = std::chrono::steady_clock::now();
    std::optional<std::chrono::steady_clock::time_point> stoppedAt;
    std::optional<std::chrono::milliseconds> probeDelay;
    std::optional<ai::openai::codex::Error> failure;
    std::optional<core::timer::Timer> settleTimer;
    std::vector<CodexState> states;
    std::vector<std::string> diagnostics;
    int failureCount = 0;
    bool timedOut = false;
    bool startCallReturned = false;
    bool failureWasAsynchronous = false;

    std::string fakeMode = scenario;
    ai::openai::codex::ClientInfo clientInfo;
    if (scenario == "bounded-overflow") {
        fakeMode = "hold-initialize";
        clientInfo.title.assign(core::pipe::PipeSource::DEFAULT_MAX_QUEUED_BYTES + 1024, 'x');
    } else if (scenario == "slow-stdin") {
        clientInfo.title.assign(512 * 1024, 'x');
    } else if (scenario == "stop-starting" || scenario == "stop-initializing") {
        fakeMode = "hold-initialize";
    }

    const std::string executable = scenario == "launch-failure" ? "/snodec/nonexistent/codex" : CODEX_FAKE_APP_SERVER;
    auto client = std::make_unique<ai::openai::codex::stdio::Client>(executable, std::vector<std::string>{fakeMode}, std::move(clientInfo));

    const auto stopAfterFailure = [&settleTimer]() {
        if (!settleTimer) {
            settleTimer.emplace(core::timer::Timer::singleshotTimer(
                []() {
                    core::SNodeC::stop();
                },
                utils::Timeval({1, 0})));
        }
    };

    client->setOnDiagnostic([&diagnostics](const ai::openai::codex::Diagnostic& diagnostic) {
        diagnostics.push_back(diagnostic.message);
    });
    client->setOnStateChanged([&](const ai::openai::codex::StateChange& stateChange) {
        states.push_back(stateChange.current);

        if (stateChange.current == CodexState::Failed) {
            ++failureCount;
            failure = stateChange.error;
            failureWasAsynchronous = startCallReturned;
            stopAfterFailure();
            return;
        }

        if (scenario == "stop-starting" && stateChange.current == CodexState::Starting) {
            client->stop();
        } else if (scenario == "stop-initializing" && stateChange.current == CodexState::Initializing) {
            client->stop();
        } else if ((scenario == "slow-stdin" || scenario == "ignore-shutdown") && stateChange.current == CodexState::Ready) {
            stoppedAt = std::chrono::steady_clock::now();
            client->stop();
        } else if (stateChange.current == CodexState::Stopped) {
            core::SNodeC::stop();
        }
    });

    [[maybe_unused]] core::timer::Timer watchdog = core::timer::Timer::singleshotTimer(
        [&timedOut]() {
            timedOut = true;
            core::SNodeC::stop();
        },
        utils::Timeval({6, 0}));

    [[maybe_unused]] core::timer::Timer responsivenessProbe = core::timer::Timer::singleshotTimer(
        [&probeDelay, startedAt]() {
            probeDelay = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - startedAt);
        },
        utils::Timeval({0, 20000}));

    client->start();
    startCallReturned = true;
    const int startResult = core::SNodeC::start(utils::Timeval({7, 0}));
    const auto completedAt = std::chrono::steady_clock::now();
    const std::size_t descriptorsAfter = descriptorCount();

    testResult.expectTrue(!timedOut, scenario + ": scenario completes before the watchdog");
    testResult.expectEqual(0, startResult, scenario + ": event loop stops programmatically");
    testResult.expectTrue(descriptorsAfter == descriptorsBefore, scenario + ": descriptors return to baseline");

    if (scenario == "exit-before-initialize") {
        testResult.expectEqual(1, failureCount, "early child exit reports failure exactly once");
        testResult.expectTrue(failure && failure->category == ai::openai::codex::Error::Category::Process,
                              "early child exit is classified as a process failure");
        testResult.expectTrue(failure && failure->code == 17, "early child exit status is observed through the event loop");
    } else if (scenario == "close-stdout") {
        testResult.expectEqual(1, failureCount, "unexpected stdout EOF reports failure exactly once");
        testResult.expectTrue(failure && failure->category == ai::openai::codex::Error::Category::Transport,
                              "unexpected stdout EOF is a transport failure");
    } else if (scenario == "initialization-error") {
        testResult.expectEqual(1, failureCount, "initialization rejection reports failure exactly once");
        testResult.expectTrue(failure && failure->category == ai::openai::codex::Error::Category::Initialization,
                              "JSON-RPC initialization errors retain their category");
        testResult.expectTrue(failure && failure->code == -32000, "JSON-RPC initialization error code is preserved");
    } else if (scenario == "malformed-json") {
        testResult.expectEqual(1, failureCount, "malformed protocol input reports failure exactly once");
        testResult.expectTrue(failure && failure->category == ai::openai::codex::Error::Category::Protocol,
                              "malformed stdout JSON is a protocol failure");
    } else if (scenario == "launch-failure") {
        testResult.expectEqual(1, failureCount, "launch failure reports failure exactly once");
        testResult.expectTrue(failure && failure->category == ai::openai::codex::Error::Category::Launch,
                              "posix_spawnp failure is classified as launch failure");
        testResult.expectTrue(failureWasAsynchronous, "launch failure callback runs after start() returns");
    } else if (scenario == "bounded-overflow") {
        testResult.expectEqual(1, failureCount, "bounded queue overflow reports failure exactly once");
        testResult.expectTrue(failure && failure->category == ai::openai::codex::Error::Category::Transport,
                              "bounded queue overflow is a transport failure");
        testResult.expectTrue(failure && failure->code == ENOBUFS, "bounded queue overflow reports ENOBUFS");
    } else if (scenario == "slow-stdin") {
        testResult.expectEqual(0, failureCount, "slow stdin consumption does not fail initialization");
        testResult.expectTrue(containsState(states, CodexState::Ready), "partial writes eventually complete initialization");
        testResult.expectTrue(containsState(states, CodexState::Stopped), "slow-consumer child stops cleanly");
        testResult.expectTrue(probeDelay && *probeDelay < std::chrono::milliseconds(200),
                              "event-loop timer remains responsive while stdin is backpressured");
        testResult.expectTrue(!diagnostics.empty() && diagnostics.back() == "slow-initialized",
                              "slow child receives the initialized notification after the large request");
    } else if (scenario == "ignore-shutdown") {
        testResult.expectEqual(0, failureCount, "forced shutdown remains an expected stop");
        testResult.expectTrue(containsState(states, CodexState::Stopped), "SIGKILL escalation is observed as a completed stop");
        testResult.expectTrue(stoppedAt && completedAt - *stoppedAt >= std::chrono::milliseconds(450),
                              "shutdown allows EOF and SIGTERM grace intervals before SIGKILL");
        testResult.expectTrue(stoppedAt && completedAt - *stoppedAt < std::chrono::seconds(2),
                              "an uncooperative child cannot stall asynchronous stop");
    } else if (scenario == "stop-starting") {
        const std::vector<CodexState> expected = {CodexState::Starting, CodexState::Stopping, CodexState::Stopped};
        testResult.expectTrue(states == expected, "stop during Starting cancels launch and preserves callback order");
    } else if (scenario == "stop-initializing") {
        const std::vector<CodexState> expected = {
            CodexState::Starting, CodexState::Initializing, CodexState::Stopping, CodexState::Stopped};
        testResult.expectTrue(states == expected, "stop during Initializing closes the child transport cleanly");
    }

    core::SNodeC::free();
    return testResult.processResult();
}
