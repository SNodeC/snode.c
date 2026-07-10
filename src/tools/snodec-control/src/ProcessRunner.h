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

#ifndef SNODECCONTROL_PROCESSRUNNER_H
#define SNODECCONTROL_PROCESSRUNNER_H

#include <string>
#include <vector>

namespace snodec::control {

    struct ProcessResult {
        bool spawned = false;
        std::string spawnError;
        int exitCode = -1;
        bool signaled = false;
        int signalNumber = 0;
        std::string stdOut;
        std::string stdErr;
    };

    // Runs `executable` with `arguments` (argv[0] is `executable` itself; no shell is involved, so no
    // command injection risk from quoting) and captures stdout/stderr into separate strings. This is
    // the sole mechanism snodec-control uses to interact with a target application: the target is
    // always treated as an opaque, separate process, never dlopen()ed or run in-process.
    ProcessResult runProcess(const std::string& executable, const std::vector<std::string>& arguments);

    // Runs `executable` with `arguments` the same way, but lets the child inherit snodec-control's own
    // stdin/stdout/stderr directly instead of capturing them. Used for `--run`, where the target is a
    // long-running (or interactive) application whose output should stream straight to the terminal
    // rather than being buffered in memory. `stdOut`/`stdErr` in the result are always left empty.
    ProcessResult runProcessAttached(const std::string& executable, const std::vector<std::string>& arguments);

    // Like runProcessAttached(), but additionally performs POSIX job control: the child is spawned into
    // its own process group and, if stdin is a real controlling terminal, that process group is made the
    // terminal's foreground job for the duration of the run. Terminal-generated signals (Ctrl-C/SIGINT,
    // Ctrl-\/SIGQUIT, Ctrl-Z/SIGTSTP, and the background-tty SIGTTIN/SIGTTOU) are then delivered by the
    // kernel to the child's process group only, not to snodec-control's own, so Ctrl-C stops the target
    // without also killing snodec-control. snodec-control's own foreground terminal ownership is restored
    // before this function returns, whether the child exited normally or was signaled.
    //
    // Used exclusively by the interactive UI's Run action, where snodec-control must survive the target
    // being interrupted so it can restore the Curses UI afterward. The plain `--run` path deliberately
    // keeps using the simpler runProcessAttached() above: there, snodec-control's own foreground job also
    // naturally ending together with the target on Ctrl-C is normal, expected shell behavior.
    ProcessResult runProcessAttachedAsForegroundJob(const std::string& executable, const std::vector<std::string>& arguments);

} // namespace snodec::control

#endif // SNODECCONTROL_PROCESSRUNNER_H
