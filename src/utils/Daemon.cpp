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

#include "utils/Daemon.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "utils/system/signal.h"

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <grp.h>
#include <poll.h>
#include <pwd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <syscall.h>
#include <unistd.h>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace utils {

    DaemonizeFailure::DaemonizeFailure(const std::string& failureMessage)
        : std::runtime_error(failureMessage) {
    }

    DaemonizeFailure::~DaemonizeFailure() {
    }

    DaemonizeError::DaemonizeError(const std::string& errorMessage)
        : DaemonizeFailure(errorMessage + ": " + std::strerror(errno)) {
    }

    DaemonizeError::~DaemonizeError() {
    }

    void Daemon::startDaemon(const std::string& pidFileName, const std::string& userName, const std::string& groupName) {
        if (std::filesystem::exists(pidFileName)) {
            throw DaemonizeFailure("Pid file '" + pidFileName + "' exists. Daemon already running?");
        } else {
            errno = 0;

            /* Fork off the parent process */
            pid_t pid = fork();
            if (pid < 0) {
                /* An error occurred */
                throw DaemonizeError("First fork()");
            } else if (pid > 0) {
                waitpid(pid, nullptr, 0); // Wait for the first child to exit
                /* Success: Let the parent terminate */
                throw DaemonizeSuccess();
            } else if (setsid() < 0) {
                /* On success: The child process becomes session leader */
                throw DaemonizeError("setsid()");
            } else if (signal(SIGHUP, SIG_IGN) == SIG_ERR) {
                /* Ignore signal sent from parent to child process */
                throw DaemonizeError("signal()");
            } else {
                /* Fork off for the second time*/
                pid = fork();
                if (pid < 0) {
                    /* An error occurred */
                    throw DaemonizeError("Second fork()");
                } else if (pid > 0) {
                    /* Success: Let the second parent terminate */
                    std::ofstream pidFile(pidFileName, std::ofstream::out);

                    if (!pidFile.good()) {
                        kill(pid, SIGTERM);
                        throw DaemonizeError("Writing pid file '" + pidFileName);
                    } else {
                        pidFile << pid << std::endl;
                        pidFile.close();
                    }
                    throw DaemonizeSuccess();
                } else {
                    waitpid(getppid(), nullptr, 0); // Wait for the parent (first child) to exit
                    struct passwd* pw = nullptr;
                    struct group* gr = nullptr;

                    if (((void) (errno = 0), gr = getgrnam(groupName.c_str())) == nullptr) {
                        if (errno != 0) {
                            throw DaemonizeError("getgrnam()");
                        } else {
                            throw DaemonizeFailure("getgrname() group not existing");
                        }
                    } else if (setegid(gr->gr_gid) != 0) {
                        throw DaemonizeError("setegid()");
                    } else if (((void) (errno = 0), (pw = getpwnam(userName.c_str())) == nullptr)) {
                        if (errno != 0) {
                            throw DaemonizeError("getpwnam()");
                        } else {
                            throw DaemonizeFailure("getpwnam() user not existing");
                        }
                    } else if (seteuid(pw->pw_uid) != 0) {
                        throw DaemonizeError("seteuid()");
                    } else {
                        /* Set new file permissions */
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
                }
            }
        }
    }

    void Daemon::stopDaemon(const std::string& pidFileName) {
        if (pidFileName.empty()) {
            throw DaemonizeFailure("No pid file given");
        } else {
            /* Try to read PID of daemon to from lockfile and kill the daemon */
            std::ifstream pidFile(pidFileName, std::ifstream::in);

            if (!pidFile.good()) {
                pidFile.close();
                throw DaemonizeError("Reading pid file '" + pidFileName + "'");
            } else {
                int pid = 0;
                pidFile >> pid;
                pidFile.close();

                int pidfd = static_cast<int>(syscall(SYS_pidfd_open, pid, 0));

                if (pidfd == -1) {
                    erasePidFile(pidFileName);
                    throw DaemonizeFailure("Daemon not running");
                } else if (kill(pid, SIGTERM) != 0) {
                    throw DaemonizeError("kill()");
                } else {
                    struct pollfd pollfd {};
                    pollfd.fd = pidfd;
                    pollfd.events = POLLIN;

                    int ready = poll(&pollfd, 1, 5000);
                    close(pidfd);

                    if (ready == -1) {
                        throw DaemonizeError("poll()");
                    } else if (ready == 0) {
                        kill(pid, SIGKILL);
                        erasePidFile(pidFileName);
                        throw DaemonizeFailure("Daemon not responding - killed");
                    } else {
                        throw DaemonizeSuccess();
                    }
                }
            }
        }
    }

    void Daemon::erasePidFile(const std::string& pidFileName) {
        (void) seteuid(getuid());             // In case  we are here seteguid can not fail
        (void) setegid(getgid());             // In case we are here setegid can not fail
        std::filesystem::remove(pidFileName); // In case we are here std::Filesystem::remove can not fail
    }

    DaemonizeSuccess::~DaemonizeSuccess() {
    }

} // namespace utils
