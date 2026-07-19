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
#include "core/timer/Timer.h"
#include "support/TestResult.h"
#include "utils/Timeval.h"

#include <filesystem>
#include <string>
#include <vector>

namespace {
    std::size_t descriptorCount() {
        std::size_t count = 0;
        for ([[maybe_unused]] const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator("/proc/self/fd")) {
            ++count;
        }
        return count;
    }
} // namespace

int main(int argc, char* argv[]) {
    tests::support::TestResult testResult;
    int result = tests::support::cTestSkipReturnCode;

    if (tests::support::shouldSkipRootWithoutSNodeCGroup()) {
        tests::support::printRootWithoutSNodeCGroupSkipMessage("CodexStdioLifecycleTest");
    } else {
        core::SNodeC::init(argc, argv);

        const std::size_t descriptorsBefore = descriptorCount();
        std::vector<ai::openai::codex::State> states;
        std::vector<std::string> diagnostics;
        bool timedOut = false;
        bool insideStateCallback = false;
        bool reentrantStateCallback = false;

        ai::openai::codex::stdio::Client client(CODEX_FAKE_APP_SERVER, {"normal"});
        client.setOnDiagnostic([&diagnostics](const ai::openai::codex::Diagnostic& diagnostic) {
            diagnostics.push_back(diagnostic.message);
        });
        client.setOnStateChanged([&](const ai::openai::codex::StateChange& stateChange) {
            if (insideStateCallback) {
                reentrantStateCallback = true;
            }
            insideStateCallback = true;
            states.push_back(stateChange.current);
            if (stateChange.current == ai::openai::codex::State::Ready) {
                client.stop();
            } else if (stateChange.current == ai::openai::codex::State::Stopped ||
                       stateChange.current == ai::openai::codex::State::Failed) {
                core::SNodeC::stop();
            }
            insideStateCallback = false;
        });

        [[maybe_unused]] core::timer::Timer watchdog = core::timer::Timer::singleshotTimer(
            [&timedOut]() {
                timedOut = true;
                core::SNodeC::stop();
            },
            utils::Timeval({4, 0}));

        client.start();
        const int startResult = core::SNodeC::start(utils::Timeval({5, 0}));
        const std::size_t descriptorsAfter = descriptorCount();

        const std::vector<ai::openai::codex::State> expectedStates = {ai::openai::codex::State::Starting,
                                                                      ai::openai::codex::State::Initializing,
                                                                      ai::openai::codex::State::Ready,
                                                                      ai::openai::codex::State::Stopping,
                                                                      ai::openai::codex::State::Stopped};

        testResult.expectTrue(!timedOut, "stdio lifecycle completes without the watchdog");
        testResult.expectEqual(0, startResult, "event loop stops cleanly after the client stops");
        testResult.expectTrue(states == expectedStates, "state callbacks preserve the full lifecycle order");
        testResult.expectTrue(!reentrantStateCallback, "state callbacks are never invoked reentrantly");
        testResult.expectTrue(diagnostics.size() == 4, "stderr is framed into four independent diagnostics");
        testResult.expectTrue(diagnostics.size() > 0 && diagnostics[0] == "diagnostic one", "first stderr line is preserved");
        testResult.expectTrue(diagnostics.size() > 1 && diagnostics[1] == "diagnostic two", "split stderr line is reassembled");
        testResult.expectTrue(diagnostics.size() > 2 && diagnostics[2].find("stderr only") != std::string::npos,
                              "JSON-looking stderr remains a diagnostic");
        testResult.expectTrue(diagnostics.size() > 3 && diagnostics[3] == "initialized-ok",
                              "initialized notification is flushed before graceful stdin EOF");
        testResult.expectTrue(descriptorsAfter == descriptorsBefore, "all parent-side process descriptors are closed");

        core::SNodeC::free();
        result = testResult.processResult();
    }

    return result;
}
