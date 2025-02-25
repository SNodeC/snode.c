/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025
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

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "utils/system/signal.h"

#include <cerrno>
#include <map>
#include <utility>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace utils::system {

    sighandler_t signal(int sig, sighandler_t handler) {
        errno = 0;
        return std::signal(sig, handler);
    }

    std::string sigabbrev_np(int sig) {
        static std::map<int, std::string> sigMap = {
            {SIGABRT, "ABRT"}, {SIGALRM, "ALRM"},     {SIGBUS, "BUS"},   {SIGCHLD, "CHLD"}, {SIGCONT, "CONT"},  {SIGFPE, "FPE"},
            {SIGHUP, "HUP"},   {SIGILL, "ILL"},       {SIGINT, "INT"},   {SIGKILL, "KILL"}, {SIGPIPE, "PIPE"},  {SIGPOLL, "POLL"},
            {SIGPROF, "PROF"}, {SIGQUIT, "QUIT"},     {SIGSEGV, "SEGV"}, {SIGSTOP, "STOP"}, {SIGSYS, "SYS"},    {SIGTERM, "TERM"},
            {SIGTRAP, "TRAP"}, {SIGTSTP, "TSTP"},     {SIGTTIN, "TTIN"}, {SIGTTOU, "TTOU"}, {SIGUSR1, "USR1"},  {SIGUSR2, "USR2"},
            {SIGURG, "URG"},   {SIGVTALRM, "VTALRM"}, {SIGXCPU, "XCPU"}, {SIGXFSZ, "XFSZ"}, {SIGWINCH, "WINCH"}};

        return sigMap.contains(sig) ? sigMap[sig] : "UNKNOWN";
    }

} // namespace utils::system
