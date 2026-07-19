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

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace {
    bool executableOnPath(std::string_view executable) {
        const char* path = std::getenv("PATH");
        if (path == nullptr) {
            return false;
        }

        const std::string searchPath(path);
        std::size_t start = 0;
        while (start <= searchPath.size()) {
            const std::size_t separator = searchPath.find(':', start);
            const std::string directory = searchPath.substr(start, separator - start);
            const std::filesystem::path candidate =
                (directory.empty() ? std::filesystem::path(".") : std::filesystem::path(directory)) / executable;
            std::error_code error;
            const std::filesystem::perms permissions = std::filesystem::status(candidate, error).permissions();
            if (!error && std::filesystem::is_regular_file(candidate, error) &&
                (permissions & (std::filesystem::perms::owner_exec | std::filesystem::perms::group_exec |
                                std::filesystem::perms::others_exec)) != std::filesystem::perms::none) {
                return true;
            }
            if (separator == std::string::npos) {
                break;
            }
            start = separator + 1;
        }
        return false;
    }
} // namespace

int main(int argc, char* argv[]) {
    if (!executableOnPath("codex")) {
        std::cout << "SKIP: codex executable is not available on PATH" << std::endl;
        return tests::support::cTestSkipReturnCode;
    }
    if (tests::support::shouldSkipRootWithoutSNodeCGroup()) {
        tests::support::printRootWithoutSNodeCGroupSkipMessage("CodexAppServerIntegrationTest");
        return tests::support::cTestSkipReturnCode;
    }

    tests::support::TestResult testResult;
    std::string codexHomeTemplate = (std::filesystem::temp_directory_path() / "snodec-codex-integration-XXXXXX").string();
    const char* const createdCodexHome = ::mkdtemp(codexHomeTemplate.data());
    const std::filesystem::path codexHome = createdCodexHome == nullptr ? std::filesystem::path() : createdCodexHome;
    std::error_code filesystemError;
    if (createdCodexHome == nullptr || ::setenv("CODEX_HOME", codexHome.c_str(), 1) != 0 ||
        ::setenv("CODEX_SQLITE_HOME", codexHome.c_str(), 1) != 0) {
        std::cout << "SKIP: unable to create an isolated writable CODEX_HOME" << std::endl;
        return tests::support::cTestSkipReturnCode;
    }

    core::SNodeC::init(argc, argv);

    std::vector<ai::openai::codex::State> states;
    std::vector<std::string> diagnostics;
    std::string failureMessage;
    bool timedOut = false;
    std::optional<core::timer::Timer> failureSettleTimer;

    ai::openai::codex::stdio::Client client;
    client.setOnDiagnostic([&diagnostics](const ai::openai::codex::Diagnostic& diagnostic) {
        diagnostics.push_back(diagnostic.message);
    });
    client.setOnStateChanged([&](const ai::openai::codex::StateChange& stateChange) {
        states.push_back(stateChange.current);
        if (stateChange.current == ai::openai::codex::State::Ready) {
            client.stop();
        } else if (stateChange.current == ai::openai::codex::State::Stopped) {
            core::SNodeC::stop();
        } else if (stateChange.current == ai::openai::codex::State::Failed) {
            if (stateChange.error) {
                failureMessage = stateChange.error->message;
            }
            failureSettleTimer.emplace(core::timer::Timer::singleshotTimer(
                []() {
                    core::SNodeC::stop();
                },
                utils::Timeval({1, 0})));
        }
    });

    [[maybe_unused]] core::timer::Timer watchdog = core::timer::Timer::singleshotTimer(
        [&timedOut]() {
            timedOut = true;
            core::SNodeC::stop();
        },
        utils::Timeval({10, 0}));

    client.start();
    const int startResult = core::SNodeC::start(utils::Timeval({12, 0}));

    const std::vector<ai::openai::codex::State> expected = {ai::openai::codex::State::Starting,
                                                            ai::openai::codex::State::Initializing,
                                                            ai::openai::codex::State::Ready,
                                                            ai::openai::codex::State::Stopping,
                                                            ai::openai::codex::State::Stopped};
    testResult.expectTrue(!timedOut, "real codex app-server initialization completes before the watchdog");
    testResult.expectEqual(0, startResult, "real codex integration stops the event loop cleanly");
    if (!diagnostics.empty()) {
        failureMessage += " stderr: " + diagnostics.back();
    }
    testResult.expectTrue(states == expected, "real codex app-server completes the lifecycle handshake: " + failureMessage);

    core::SNodeC::free();
    std::filesystem::remove_all(codexHome, filesystemError);
    return testResult.processResult();
}
