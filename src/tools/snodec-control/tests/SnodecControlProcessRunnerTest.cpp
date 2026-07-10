/*
 * snodec-control - Out-of-tree companion tool for SNode.C applications
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

// Exercises runProcessAttachedAsForegroundJob()'s basic spawn/exit/signal contract without needing a
// real controlling terminal: under CTest, stdin is not a tty, so the job-control-specific parts
// (tcsetpgrp() terminal handover/reclaim) are simply skipped by the function's own isatty() guard, and
// only the process-group/signal-handling machinery around posix_spawn()/waitpid() is exercised. The
// actual terminal foreground handover and Ctrl-C behavior are covered by the manual smoke test in
// tests/manual/ (see smoke-ui-notes.md), which needs a real pty and is deliberately not part of CTest.

#include "ProcessRunner.h"
#include "TestResult.h"

#include <csignal>
#include <string>
#include <vector>

using snodec::control::ProcessResult;
using snodec::control::runProcessAttachedAsForegroundJob;

namespace {

    void testNormalExitCode(snodec::control::test::TestResult& testResult) {
        const ProcessResult result = runProcessAttachedAsForegroundJob("/bin/sh", {"-c", "exit 0"});
        testResult.expectTrue(result.spawned, "normal exit: process spawns successfully");
        testResult.expectTrue(!result.signaled, "normal exit: not reported as signaled");
        testResult.expectEqual(0, result.exitCode, "normal exit: exit code 0 is reported correctly");
    }

    void testCustomExitCode(snodec::control::test::TestResult& testResult) {
        const ProcessResult result = runProcessAttachedAsForegroundJob("/bin/sh", {"-c", "exit 42"});
        testResult.expectTrue(result.spawned, "custom exit: process spawns successfully");
        testResult.expectEqual(42, result.exitCode, "custom exit: a non-zero exit code is reported correctly");
    }

    // A signal not among the job-control signals runProcessAttachedAsForegroundJob() specially manages
    // (SIGINT/SIGQUIT/SIGTSTP/SIGTTIN/SIGTTOU): confirms ordinary signal reporting still works normally,
    // i.e. the new job-control/signal-disposition handling doesn't interfere with signals it isn't meant
    // to touch at all.
    void testSignaledProcessIsReportedCorrectly(snodec::control::test::TestResult& testResult) {
        const ProcessResult result = runProcessAttachedAsForegroundJob("/bin/sh", {"-c", "kill -TERM $$"});
        testResult.expectTrue(result.spawned, "signaled: process spawns successfully");
        testResult.expectTrue(result.signaled, "signaled: self-terminating via SIGTERM is reported as signaled");
        testResult.expectEqual(SIGTERM, result.signalNumber, "signaled: the correct signal number is reported");
    }

    void testSpawnFailureForNonexistentExecutable(snodec::control::test::TestResult& testResult) {
        const ProcessResult result = runProcessAttachedAsForegroundJob("/does/not/exist/snodec-control-fake", {});
        testResult.expectTrue(!result.spawned, "spawn failure: a nonexistent executable is never reported as spawned");
        testResult.expectTrue(!result.spawnError.empty(), "spawn failure: a descriptive spawn error is set");
    }

    // A basic smoke check that the function still works when given real arguments (not just -c scripts),
    // matching how CursesUi.cpp actually calls it (executable + a real argv, never a shell command line).
    void testWithRealArguments(snodec::control::test::TestResult& testResult) {
        const ProcessResult result = runProcessAttachedAsForegroundJob("/bin/sh", std::vector<std::string>{"--version"});
        testResult.expectTrue(result.spawned, "real arguments: process spawns successfully");
        testResult.expectTrue(!result.signaled, "real arguments: not reported as signaled");
    }

} // namespace

int main() {
    snodec::control::test::TestResult testResult;

    testNormalExitCode(testResult);
    testCustomExitCode(testResult);
    testSignaledProcessIsReportedCorrectly(testResult);
    testSpawnFailureForNonexistentExecutable(testResult);
    testWithRealArguments(testResult);

    return testResult.processResult();
}
