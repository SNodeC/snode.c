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
#include "core/SNodeC.h"
#include "core/pipe/PipeSource.h"
#include "core/timer/Timer.h"
#include "support/TestResult.h"
#include "utils/Timeval.h"

#include <cerrno>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <system_error>
#include <unistd.h>
#include <utility>
#include <vector>

namespace {
    using CodexState = ai::openai::codex::State;

    class ForcedPollingClient final : public ai::openai::codex::AppServerClient {
    public:
        ForcedPollingClient(std::string executable, std::vector<std::string> arguments, ai::openai::codex::ClientInfo clientInfo)
            : AppServerClient(
                  std::make_unique<ai::openai::codex::stdio::detail::StdioTransport>(std::move(executable), std::move(arguments), true),
                  std::move(clientInfo)) {
        }
    };

    class SetupFailureClient final : public ai::openai::codex::AppServerClient {
    public:
        SetupFailureClient(std::string executable, std::vector<std::string> arguments, ai::openai::codex::ClientInfo clientInfo)
            : AppServerClient(std::make_unique<ai::openai::codex::stdio::detail::StdioTransport>(
                                  std::move(executable), std::move(arguments), false, true),
                              std::move(clientInfo)) {
        }
    };

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

    bool containsDiagnostic(const std::vector<std::string>& diagnostics, const std::string& expected) {
        for (const std::string& diagnostic : diagnostics) {
            if (diagnostic == expected) {
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
    std::optional<core::timer::Timer> delayedStopTimer;
    std::optional<std::filesystem::path> recoveryMarker;
    std::vector<CodexState> states;
    std::vector<std::string> diagnostics;
    int failureCount = 0;
    bool timedOut = false;
    bool startCallReturned = false;
    bool failureWasAsynchronous = false;
    int stoppedCount = 0;

    std::string fakeMode = scenario;
    std::vector<std::string> fakeArguments;
    ai::openai::codex::ClientInfo clientInfo;
    if (scenario == "bounded-overflow") {
        fakeMode = "hold-initialize";
        clientInfo.title.assign(core::pipe::PipeSource::DEFAULT_MAX_QUEUED_BYTES + 1024, 'x');
    } else if (scenario == "slow-stdin") {
        clientInfo.title.assign(512 * 1024, 'x');
    } else if (scenario == "blocked-shutdown" || scenario == "blocked-ignore-shutdown") {
        fakeMode = scenario == "blocked-shutdown" ? "never-read-stdin" : "never-read-stdin-ignore-term";
        clientInfo.title.assign(512 * 1024, 'x');
    } else if (scenario == "stop-starting" || scenario == "stop-initializing") {
        fakeMode = "hold-initialize";
    } else if (scenario == "post-spawn-setup-failure") {
        fakeMode = "hold-initialize";
    } else if (scenario == "forced-fallback-normal") {
        fakeMode = "normal";
    } else if (scenario == "forced-fallback-exit-before-initialize") {
        fakeMode = "exit-before-initialize";
    } else if (scenario == "recover-after-failure") {
        fakeMode = "fail-once";
        std::string markerTemplate = (std::filesystem::temp_directory_path() / "snodec-codex-recovery-XXXXXX").string();
        const int markerFd = ::mkstemp(markerTemplate.data());
        testResult.expectTrue(markerFd >= 0, "recovery test reserves a unique marker path");
        if (markerFd < 0) {
            core::SNodeC::free();
            return testResult.processResult();
        }
        ::close(markerFd);
        const bool markerRemoved = ::unlink(markerTemplate.c_str()) == 0;
        testResult.expectTrue(markerRemoved, "recovery marker path is clear before the first launch");
        if (!markerRemoved) {
            core::SNodeC::free();
            return testResult.processResult();
        }
        recoveryMarker = std::move(markerTemplate);
    }
    fakeArguments.push_back(fakeMode);
    if (recoveryMarker) {
        fakeArguments.push_back(recoveryMarker->string());
    }

    const std::string executable = scenario == "launch-failure" ? "/snodec/nonexistent/codex" : CODEX_FAKE_APP_SERVER;
    std::unique_ptr<ai::openai::codex::AppServerClient> client;
    if (scenario == "forced-fallback-normal" || scenario == "forced-fallback-exit-before-initialize") {
        client = std::make_unique<ForcedPollingClient>(executable, std::move(fakeArguments), std::move(clientInfo));
    } else if (scenario == "post-spawn-setup-failure") {
        client = std::make_unique<SetupFailureClient>(executable, std::move(fakeArguments), std::move(clientInfo));
    } else {
        client = std::make_unique<ai::openai::codex::stdio::Client>(executable, std::move(fakeArguments), std::move(clientInfo));
    }

    client->setOnDiagnostic([&diagnostics](const ai::openai::codex::Diagnostic& diagnostic) {
        diagnostics.push_back(diagnostic.message);
    });
    client->setOnStateChanged([&](const ai::openai::codex::StateChange& stateChange) {
        states.push_back(stateChange.current);

        if (stateChange.current == CodexState::Failed) {
            ++failureCount;
            failure = stateChange.error;
            failureWasAsynchronous = startCallReturned;
            client->stop();
            return;
        }

        if (scenario == "stop-starting" && stateChange.current == CodexState::Starting) {
            client->stop();
        } else if ((scenario == "stop-initializing" || scenario == "blocked-shutdown" || scenario == "blocked-ignore-shutdown") &&
                   stateChange.current == CodexState::Initializing) {
            if (scenario == "blocked-shutdown" || scenario == "blocked-ignore-shutdown") {
                stoppedAt = std::chrono::steady_clock::now();
            }
            client->stop();
        } else if (scenario == "close-stderr" && stateChange.current == CodexState::Ready) {
            delayedStopTimer.emplace(core::timer::Timer::singleshotTimer(
                [&client]() {
                    client->stop();
                },
                utils::Timeval({0, 100000})));
        } else if ((scenario == "slow-stdin" || scenario == "ignore-shutdown" || scenario == "stdio-framing" ||
                    scenario == "forced-fallback-normal" || scenario == "recover-after-failure") &&
                   stateChange.current == CodexState::Ready) {
            stoppedAt = std::chrono::steady_clock::now();
            client->stop();
        } else if (stateChange.current == CodexState::Stopped) {
            ++stoppedCount;
            if (scenario == "recover-after-failure" && stoppedCount == 1) {
                client->start();
            } else {
                core::SNodeC::stop();
            }
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
    if (recoveryMarker) {
        std::error_code removeError;
        std::filesystem::remove(*recoveryMarker, removeError);
    }
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
    } else if (scenario == "post-spawn-setup-failure") {
        testResult.expectEqual(1, failureCount, "post-spawn parent setup failure is reported exactly once");
        testResult.expectTrue(failure && failure->category == ai::openai::codex::Error::Category::Transport,
                              "post-spawn parent setup failure retains its transport category");
        testResult.expectTrue(containsState(states, CodexState::Stopped),
                              "post-spawn rollback kills and reaps the child before reaching Stopped");
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
        testResult.expectTrue(containsDiagnostic(diagnostics, "initialized-ok"),
                              "queued initialized output reaches the child before shutdown signaling");
        testResult.expectTrue(stoppedAt && completedAt - *stoppedAt >= std::chrono::milliseconds(450),
                              "shutdown allows EOF and SIGTERM grace intervals before SIGKILL");
        testResult.expectTrue(stoppedAt && completedAt - *stoppedAt < std::chrono::seconds(2),
                              "an uncooperative child cannot stall asynchronous stop");
    } else if (scenario == "blocked-shutdown" || scenario == "blocked-ignore-shutdown") {
        testResult.expectEqual(0, failureCount, "blocked stdin flush remains an expected stop");
        testResult.expectTrue(containsState(states, CodexState::Stopped), "blocked stdin child is terminated and reaped");
        testResult.expectTrue(stoppedAt && completedAt - *stoppedAt >= std::chrono::milliseconds(200),
                              "blocked stdin observes its flush deadline before SIGTERM");
        testResult.expectTrue(stoppedAt && completedAt - *stoppedAt < std::chrono::seconds(2), "blocked stdin shutdown remains bounded");
        if (scenario == "blocked-ignore-shutdown") {
            testResult.expectTrue(stoppedAt && completedAt - *stoppedAt >= std::chrono::milliseconds(450),
                                  "SIGTERM-ignoring blocked child reaches SIGKILL escalation");
        }
    } else if (scenario == "stop-starting") {
        const std::vector<CodexState> expected = {CodexState::Starting, CodexState::Stopping, CodexState::Stopped};
        testResult.expectTrue(states == expected, "stop during Starting cancels launch and preserves callback order");
    } else if (scenario == "stop-initializing") {
        const std::vector<CodexState> expected = {
            CodexState::Starting, CodexState::Initializing, CodexState::Stopping, CodexState::Stopped};
        testResult.expectTrue(states == expected, "stop during Initializing closes the child transport cleanly");
    } else if (scenario == "stdio-framing") {
        testResult.expectEqual(0, failureCount, "stdio framing completes without a transport failure");
        testResult.expectTrue(containsState(states, CodexState::Ready), "pure JSON protocol documents initialize over JSONL stdio");
        testResult.expectTrue(containsDiagnostic(diagnostics, "stdio-framing-ok"),
                              "the fake server receives exactly the initialize and initialized JSONL operations");
    } else if (scenario == "close-stderr") {
        testResult.expectEqual(0, failureCount, "stderr EOF does not fail the protocol transport");
        testResult.expectTrue(containsState(states, CodexState::Ready), "the client reaches Ready before stderr closes");
        testResult.expectTrue(containsState(states, CodexState::Stopped), "the client stops cleanly after stderr closes");
        testResult.expectTrue(!containsState(states, CodexState::Failed), "stderr closure never enters Failed");
        testResult.expectTrue(containsDiagnostic(diagnostics, "trailing diagnostic without newline"),
                              "stderr EOF emits the final unterminated diagnostic line");
    } else if (scenario == "recover-after-failure") {
        const std::vector<CodexState> expected = {CodexState::Starting,
                                                  CodexState::Initializing,
                                                  CodexState::Failed,
                                                  CodexState::Stopping,
                                                  CodexState::Stopped,
                                                  CodexState::Starting,
                                                  CodexState::Initializing,
                                                  CodexState::Ready,
                                                  CodexState::Stopping,
                                                  CodexState::Stopped};
        testResult.expectEqual(1, failureCount, "the first initialization failure is reported exactly once");
        testResult.expectTrue(states == expected, "an explicit stop recovers Failed and permits a successful second start");
    } else if (scenario == "forced-fallback-normal") {
        testResult.expectEqual(0, failureCount, "forced waitpid polling completes the normal lifecycle without failure");
        testResult.expectTrue(containsState(states, CodexState::Ready), "forced waitpid polling reaches Ready");
        testResult.expectTrue(containsState(states, CodexState::Stopped), "forced waitpid polling observes graceful child exit");
    } else if (scenario == "forced-fallback-exit-before-initialize") {
        testResult.expectEqual(1, failureCount, "forced waitpid polling reports early exit exactly once");
        testResult.expectTrue(failure && failure->category == ai::openai::codex::Error::Category::Process,
                              "forced waitpid polling classifies early exit as a process failure");
        testResult.expectTrue(failure && failure->code == 17, "forced waitpid polling preserves the child exit status");
    }

    core::SNodeC::free();
    return testResult.processResult();
}
