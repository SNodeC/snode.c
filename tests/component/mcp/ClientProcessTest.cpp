/*
 * SNode.C - A Slim Toolkit for Network Communication
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

#include "core/EventReceiver.h"
#include "core/SNodeC.h"
#include "core/timer/Timer.h"
#include "mcp/client/Mcp.h"
#include "mcp/client/Stdio.h"
#include "tests/support/TestResult.h"
#ifndef DOXYGEN_SHOULD_SKIP_THIS
#include <cerrno>
#include <csignal>
#include <functional>
#include <memory>
#include <sys/wait.h>
#endif
int main(int argc, char* argv[]) {
    tests::support::TestResult test;
    unsigned exits = 0;
    mcp::client::Stdio child("child-lifecycle", []() {
        return new mcp::client::Mcp();
    });
    auto transient = std::make_unique<mcp::client::Stdio>("pending-child", []() {
        return new mcp::client::Mcp();
    });
    transient->start({"/bin/sleep", "30"}, [&test](int) {
        test.expectTrue(false, "destroyed facade suppresses late exit callback");
    });
    core::EventReceiver::atNextTick([&transient]() {
        transient.reset();
    });
    core::SNodeC::init(argc, argv);
    std::function<void(int)> exited;
    exited = [&](int status) {
        ++exits;
        if (exits == 1) {
            test.expectTrue(status == -ENOENT, "spawn failure reports errno and releases pipes");
            child.start({"/bin/true"}, exited);
        } else if (exits == 2) {
            test.expectTrue(WIFEXITED(status) && WEXITSTATUS(status) == 0, "early child EOF is reaped exactly once");
            child.start({"/bin/sleep", "30"}, exited);
            core::timer::Timer::singleshotTimer(
                [&child]() {
                    child.stop();
                },
                utils::Timeval(0.05));
        } else {
            test.expectTrue(WIFSIGNALED(status) && WTERMSIG(status) == SIGTERM,
                            "teardown terminates and reaps a child with a reset signal mask");
            core::SNodeC::stop();
        }
    };
    child.start({"/definitely/not/a/program"}, exited);
    core::timer::Timer::singleshotTimer(
        []() {
            core::SNodeC::stop();
        },
        utils::Timeval(3));
    core::SNodeC::start();
    test.expectTrue(exits == 3, "all spawn/EOF/teardown callbacks execute");
    int status = 0;
    test.expectTrue(::waitpid(-1, &status, WNOHANG) == -1 && errno == ECHILD, "no zombie or live child remains");
    return test.processResult();
}
