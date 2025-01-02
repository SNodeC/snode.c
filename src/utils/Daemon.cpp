/*
 * SNode.C - a slim toolkit for network communication
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

#include "utils/Daemon.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "core/system/poll.h"

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <grp.h>
#include <pwd.h>
#include <sys/syscall.h>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace utils {

    DaemonFailure::DaemonFailure(const std::string& failureMessage)
        : std::runtime_error(failureMessage) {
    }

    DaemonFailure::~DaemonFailure() {
    }

    DaemonError::DaemonError(const std::string& errorMessage)
        : DaemonFailure(errorMessage + ": " + std::strerror(errno)) {
    }

    DaemonError::~DaemonError() {
    }

    void Daemon::startDaemon(const std::string& pidFileName, const std::string& userName, const std::string& groupName) {
        if (std::filesystem::exists(pidFileName)) {
            throw DaemonFailure("Pid file '" + pidFileName + "' exists. Daemon already running?");
        }
        errno = 0;

        /* Fork off the parent process */
        pid_t pid = fork();
        if (pid < 0) {
            /* An error occurred */
            throw DaemonError("First fork()");
        }
        if (pid > 0) {
            /* Success: Let the parent terminate */
            throw DaemonSignaled("Lead new session", pid);
        }

        if (setsid() < 0) {
            /* On success: The child process becomes session leader */
            throw DaemonError("setsid()");
        }

        if (signal(SIGHUP, SIG_IGN) == SIG_ERR) {
            /* Ignore signal sent from parent to child process */
            throw DaemonError("signal()");
        }

        /* Fork off for the second time*/
        pid = fork();
        if (pid < 0) {
            /* An error occurred */
            throw DaemonError("Second fork()");
        }
        if (pid > 0) {
            /* Success: Let the second parent terminate */
            std::ofstream pidFile(pidFileName, std::ofstream::out);

            if (!pidFile.good()) {
                kill(pid, SIGTERM);
                throw DaemonError("Writing pid file '" + pidFileName);
            }
            pidFile << pid << std::endl;
            pidFile.close();

            throw DaemonSignaled("Drop session lead", pid);
        }

        struct passwd* pw = nullptr;
        struct group* gr = nullptr;

        if (((void) (errno = 0), gr = getgrnam(groupName.c_str())) == nullptr) {
            if (errno != 0) {
                throw DaemonError("getgrnam()");
            }
            throw DaemonFailure("getgrname() group not existing");
        }
        if (setegid(gr->gr_gid) != 0) {
            throw DaemonError("setegid()");
        }
        if (((void) (errno = 0), (pw = getpwnam(userName.c_str())) == nullptr)) {
            if (errno != 0) {
                throw DaemonError("getpwnam()");
            }
            throw DaemonFailure("getpwnam() user not existing");
        }
        if (seteuid(pw->pw_uid) != 0) {
            throw DaemonError("seteuid()");
        } /* Set new file permissions */
        umask(0);
        chdir("/");

        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);

        if (std::freopen("/dev/null", "r", stdin) == nullptr) {
        }
        if (std::freopen("/dev/null", "w+", stdout) == nullptr) {
        }
        if (std::freopen("/dev/null", "w+", stderr) == nullptr) {
        }
    }

    pid_t Daemon::stopDaemon(const std::string& pidFileName) {
        if (pidFileName.empty()) {
            throw DaemonFailure("No pid file given");
        } /* Try to read PID of daemon to from lockfile and kill the daemon */
        std::ifstream pidFile(pidFileName, std::ifstream::in);

        if (!pidFile.good()) {
            pidFile.close();
            throw DaemonError("Reading pid file '" + pidFileName + "'");
        }
        pid_t pid = 0;
        pidFile >> pid;
        pidFile.close();

        const int pidfd = static_cast<int>(syscall(SYS_pidfd_open, pid, 0)); // NOLINT

        if (pidfd == -1) {
            erasePidFile(pidFileName);
            throw DaemonFailure("Daemon not running");
        }
        if (kill(pid, SIGTERM) != 0) {
            throw DaemonError("kill()");
        }
        struct pollfd pollfd{};
        pollfd.fd = pidfd;
        pollfd.events = POLLIN;

        const int ready = core::system::poll(&pollfd, 1, 5000);
        close(pidfd);

        if (ready == -1) {
            throw DaemonError("poll()");
        }
        if (ready == 0) {
            kill(pid, SIGKILL);
            erasePidFile(pidFileName);
            throw DaemonFailure("Daemon not responding - killed");
        }

        return pid;
    }

    void Daemon::erasePidFile(const std::string& pidFileName) {
        (void) seteuid(getuid());             // In case we are here seteguid can not fail
        (void) setegid(getgid());             // In case we are here setegid can not fail
        std::filesystem::remove(pidFileName); // In case we are here std::Filesystem::remove can not fail
    }

    DaemonSignaled::DaemonSignaled(const std::string& message, pid_t pid)
        : std::runtime_error(message)
        , pid(pid) {
    }

    DaemonSignaled::~DaemonSignaled() {
    }

    pid_t DaemonSignaled::getPid() const {
        return pid;
    }

} // namespace utils
