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

#include "ProcessRunner.h"

#include <algorithm>
#include <array>
#include <cerrno>
#include <csignal>
#include <cstring>
#include <spawn.h>
#include <sys/select.h>
#include <sys/wait.h>
#include <unistd.h>

extern char** environ; // NOLINT

namespace snodec::control {

    namespace {

        struct Pipe {
            int readEnd = -1;
            int writeEnd = -1;
        };

        bool makePipe(Pipe& pipe, std::string& error) {
            std::array<int, 2> fds{-1, -1};

            if (::pipe(fds.data()) != 0) {
                error = std::strerror(errno);
                return false;
            }

            pipe.readEnd = fds[0];
            pipe.writeEnd = fds[1];

            return true;
        }

        // Reads from both pipes until both have reached EOF, appending into the matching output string.
        void drainPipes(Pipe& outPipe, Pipe& errPipe, std::string& stdOut, std::string& stdErr) {
            std::array<char, 4096> buffer{};

            bool outOpen = outPipe.readEnd >= 0;
            bool errOpen = errPipe.readEnd >= 0;

            while (outOpen || errOpen) {
                fd_set readSet;
                FD_ZERO(&readSet);

                int maxFd = -1;
                if (outOpen) {
                    FD_SET(outPipe.readEnd, &readSet);
                    maxFd = std::max(maxFd, outPipe.readEnd);
                }
                if (errOpen) {
                    FD_SET(errPipe.readEnd, &readSet);
                    maxFd = std::max(maxFd, errPipe.readEnd);
                }

                const int selected = ::select(maxFd + 1, &readSet, nullptr, nullptr, nullptr);
                if (selected < 0) {
                    if (errno == EINTR) {
                        continue;
                    }
                    break;
                }

                if (outOpen && FD_ISSET(outPipe.readEnd, &readSet)) {
                    const ssize_t bytesRead = ::read(outPipe.readEnd, buffer.data(), buffer.size());
                    if (bytesRead > 0) {
                        stdOut.append(buffer.data(), static_cast<std::size_t>(bytesRead));
                    } else {
                        ::close(outPipe.readEnd);
                        outPipe.readEnd = -1;
                        outOpen = false;
                    }
                }

                if (errOpen && FD_ISSET(errPipe.readEnd, &readSet)) {
                    const ssize_t bytesRead = ::read(errPipe.readEnd, buffer.data(), buffer.size());
                    if (bytesRead > 0) {
                        stdErr.append(buffer.data(), static_cast<std::size_t>(bytesRead));
                    } else {
                        ::close(errPipe.readEnd);
                        errPipe.readEnd = -1;
                        errOpen = false;
                    }
                }
            }
        }

        std::vector<char*> buildArgv(const std::string& executable, const std::vector<std::string>& arguments) {
            std::vector<char*> argv;
            argv.reserve(arguments.size() + 2);
            argv.push_back(const_cast<char*>(executable.c_str()));
            for (const std::string& argument : arguments) {
                argv.push_back(const_cast<char*>(argument.c_str()));
            }
            argv.push_back(nullptr);
            return argv;
        }

        // The terminal-generated job-control signals runProcessAttachedAsForegroundJob() needs to
        // temporarily disarm in snodec-control itself around handing the terminal's foreground job to
        // the spawned child, and reset to default in the child so it behaves like any normal foreground
        // process regardless of what snodec-control's own disposition was at the moment of the call.
        constexpr std::array<int, 5> jobControlSignals{SIGINT, SIGQUIT, SIGTSTP, SIGTTIN, SIGTTOU};

        // Sets every signal in jobControlSignals to SIG_IGN in the calling (parent) process, recording
        // the previous dispositions in `previous` (indexed the same as jobControlSignals) so they can be
        // restored afterward via restoreJobControlSignals(). Ignoring (rather than blocking) is enough:
        // once the terminal's foreground process group is switched to the child, the kernel delivers
        // these signals to the child's process group only, never to snodec-control's, regardless of its
        // disposition; ignoring here is purely a safety net for the brief window around the handover.
        void ignoreJobControlSignals(std::array<struct sigaction, 5>& previous) {
            struct sigaction ignoreAction {};
            ignoreAction.sa_handler = SIG_IGN;
            sigemptyset(&ignoreAction.sa_mask);

            for (std::size_t i = 0; i < jobControlSignals.size(); ++i) {
                ::sigaction(jobControlSignals[i], &ignoreAction, &previous[i]);
            }
        }

        void restoreJobControlSignals(const std::array<struct sigaction, 5>& previous) {
            for (std::size_t i = 0; i < jobControlSignals.size(); ++i) {
                ::sigaction(jobControlSignals[i], &previous[i], nullptr);
            }
        }

    } // namespace

    ProcessResult runProcess(const std::string& executable, const std::vector<std::string>& arguments) {
        ProcessResult result;

        Pipe outPipe;
        Pipe errPipe;

        if (!makePipe(outPipe, result.spawnError) || !makePipe(errPipe, result.spawnError)) {
            return result;
        }

        posix_spawn_file_actions_t fileActions;
        posix_spawn_file_actions_init(&fileActions);
        posix_spawn_file_actions_adddup2(&fileActions, outPipe.writeEnd, STDOUT_FILENO);
        posix_spawn_file_actions_adddup2(&fileActions, errPipe.writeEnd, STDERR_FILENO);
        posix_spawn_file_actions_addclose(&fileActions, outPipe.readEnd);
        posix_spawn_file_actions_addclose(&fileActions, errPipe.readEnd);
        posix_spawn_file_actions_addclose(&fileActions, outPipe.writeEnd);
        posix_spawn_file_actions_addclose(&fileActions, errPipe.writeEnd);

        std::vector<char*> argv = buildArgv(executable, arguments);

        pid_t pid = -1;
        const int spawnRc = posix_spawn(&pid, executable.c_str(), &fileActions, nullptr, argv.data(), environ);

        posix_spawn_file_actions_destroy(&fileActions);

        ::close(outPipe.writeEnd);
        ::close(errPipe.writeEnd);

        if (spawnRc != 0) {
            ::close(outPipe.readEnd);
            ::close(errPipe.readEnd);
            result.spawnError = std::strerror(spawnRc);
            return result;
        }

        result.spawned = true;

        drainPipes(outPipe, errPipe, result.stdOut, result.stdErr);

        int status = 0;
        if (::waitpid(pid, &status, 0) == pid) {
            if (WIFEXITED(status)) {
                result.exitCode = WEXITSTATUS(status);
            } else if (WIFSIGNALED(status)) {
                result.signaled = true;
                result.signalNumber = WTERMSIG(status);
            }
        }

        return result;
    }

    ProcessResult runProcessAttached(const std::string& executable, const std::vector<std::string>& arguments) {
        ProcessResult result;

        std::vector<char*> argv = buildArgv(executable, arguments);

        pid_t pid = -1;
        const int spawnRc = posix_spawn(&pid, executable.c_str(), nullptr, nullptr, argv.data(), environ);

        if (spawnRc != 0) {
            result.spawnError = std::strerror(spawnRc);
            return result;
        }

        result.spawned = true;

        int status = 0;
        if (::waitpid(pid, &status, 0) == pid) {
            if (WIFEXITED(status)) {
                result.exitCode = WEXITSTATUS(status);
            } else if (WIFSIGNALED(status)) {
                result.signaled = true;
                result.signalNumber = WTERMSIG(status);
            }
        }

        return result;
    }

    ProcessResult runProcessAttachedAsForegroundJob(const std::string& executable, const std::vector<std::string>& arguments) {
        ProcessResult result;

        const bool haveTerminal = ::isatty(STDIN_FILENO) == 1;
        const pid_t parentPgrp = haveTerminal ? ::getpgrp() : -1;

        // Disarm the job-control signals in snodec-control itself before spawning (see
        // ignoreJobControlSignals()'s comment); restored via restoreJobControlSignals() on every exit
        // path below.
        std::array<struct sigaction, 5> previousSignalActions{};
        ignoreJobControlSignals(previousSignalActions);

        std::vector<char*> argv = buildArgv(executable, arguments);

        posix_spawnattr_t attr;
        posix_spawnattr_init(&attr);

        short flags = POSIX_SPAWN_SETPGROUP;

        // The child gets its own process group (pgid == its own pid, via posix_spawnattr_setpgroup(&attr,
        // 0)) atomically as part of the spawn, avoiding the classic fork()+setpgid() race between parent
        // and child both trying to set the child's process group before either one uses it.
        posix_spawnattr_setpgroup(&attr, 0);

        // Regardless of what snodec-control's own disposition/mask for these signals happens to be at
        // the moment of the call (including the SIG_IGN just set above), the child always starts with
        // default disposition and an empty blocked-signal mask for them, so it behaves like a normal,
        // freshly started foreground process.
        sigset_t defaultSignals;
        sigemptyset(&defaultSignals);
        for (const int signalNumber : jobControlSignals) {
            sigaddset(&defaultSignals, signalNumber);
        }
        posix_spawnattr_setsigdefault(&attr, &defaultSignals);
        flags |= POSIX_SPAWN_SETSIGDEF;

        sigset_t emptyMask;
        sigemptyset(&emptyMask);
        posix_spawnattr_setsigmask(&attr, &emptyMask);
        flags |= POSIX_SPAWN_SETSIGMASK;

        posix_spawnattr_setflags(&attr, flags);

        pid_t pid = -1;
        const int spawnRc = posix_spawn(&pid, executable.c_str(), nullptr, &attr, argv.data(), environ);

        posix_spawnattr_destroy(&attr);

        if (spawnRc != 0) {
            result.spawnError = std::strerror(spawnRc);
            restoreJobControlSignals(previousSignalActions);
            return result;
        }

        result.spawned = true;

        if (haveTerminal) {
            // Hand the terminal's foreground job to the child's own process group so terminal-generated
            // signals (Ctrl-C, Ctrl-\, Ctrl-Z) go to the target, not to snodec-control. SIGTTOU is
            // already ignored above, so this cannot stop snodec-control even though it is not currently
            // the foreground job at the instant this runs.
            ::tcsetpgrp(STDIN_FILENO, pid);
        }

        int status = 0;
        pid_t waited = 0;
        do {
            waited = ::waitpid(pid, &status, 0);
        } while (waited < 0 && errno == EINTR);

        if (waited == pid) {
            if (WIFEXITED(status)) {
                result.exitCode = WEXITSTATUS(status);
            } else if (WIFSIGNALED(status)) {
                result.signaled = true;
                result.signalNumber = WTERMSIG(status);
            }
        }

        if (haveTerminal) {
            // Reclaim the terminal for snodec-control itself. SIGTTOU is still ignored here too, for the
            // same reason as above.
            ::tcsetpgrp(STDIN_FILENO, parentPgrp);
        }

        restoreJobControlSignals(previousSignalActions);

        return result;
    }

} // namespace snodec::control
