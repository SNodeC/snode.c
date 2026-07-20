/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include "ai/openai/codex/stdio/Client.h"
#include "core/SNodeC.h"
#include "support/TestResult.h"

#include <filesystem>
#include <string>
#include <unistd.h>

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
    core::SNodeC::init(argc, argv);

    std::string markerTemplate = (std::filesystem::temp_directory_path() / "snodec-codex-not-started-XXXXXX").string();
    const int markerFd = ::mkstemp(markerTemplate.data());
    testResult.expectTrue(markerFd >= 0, "startup-admission test reserves a marker path");
    if (markerFd >= 0) {
        ::close(markerFd);
        ::unlink(markerTemplate.c_str());
    }

    const std::size_t descriptorsBefore = descriptorCount();
    ai::openai::codex::stdio::Client client(CODEX_FAKE_APP_SERVER, {"pidfile-normal", markerTemplate});
    client.start();
    core::SNodeC::stop();
    core::SNodeC::free();
    const std::size_t descriptorsAfter = descriptorCount();

    testResult.expectTrue(!std::filesystem::exists(markerTemplate), "deferred start does not spawn a child after shutdown begins");
    testResult.expectTrue(client.getState() == ai::openai::codex::State::Stopped, "shutdown-cancelled deferred start returns to Stopped");
    testResult.expectTrue(descriptorsAfter == descriptorsBefore, "shutdown-cancelled start registers no persistent descriptor or timer");
    return testResult.processResult();
}
