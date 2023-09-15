/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021, 2022, 2023 Volker Christian <me@vchrist.at>
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

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "utils/system/signal.h"

#include <cerrno>
#include <map>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace utils::system {

    sighandler_t signal(int sig, sighandler_t handler) {
        errno = 0;
        return std::signal(sig, handler);
    }

    std::string sigabbrev_np(int sig) {
        static std::map<int, std::string> sigMap = {
            {SIGHUP, "HUP"},    {SIGINT, "INT"},   {SIGQUIT, "QUIT"}, {SIGILL, "ILL"},       {SIGTRAP, "TRAP"},     {SIGABRT, "ABRT"},
            {SIGIOT, "IOT"},    {SIGBUS, "BUS"},   {SIGFPE, "FPE"},   {SIGKILL, "KILL"},     {SIGUSR1, "USR1"},     {SIGSEGV, "SEGV"},
            {SIGUSR2, "USR2"},  {SIGPIPE, "PIPE"}, {SIGALRM, "ALRM"}, {SIGTERM, "TERM"},     {SIGSTKFLT, "STKFLT"}, {SIGCHLD, "CHLD"},
            {SIGCHLD, "CHLD "}, {SIGCONT, "CONT"}, {SIGSTOP, "STOP"}, {SIGTSTP, "TSTP"},     {SIGTTIN, "TTIN"},     {SIGTTOU, "TTOU"},
            {SIGURG, "URG"},    {SIGXCPU, "XCPU"}, {SIGXFSZ, "XFSZ"}, {SIGVTALRM, "VTALRM"}, {SIGPROF, "PROF"},     {SIGWINCH, "WINCH"},
            {SIGIO, "IO"},      {SIGPOLL, "POLL"}, {SIGPWR, "PWR"},   {SIGSYS, "SYS"}};

        return sigMap.contains(sig) ? sigMap[sig] : "UNKNOWN";
    }

} // namespace utils::system
