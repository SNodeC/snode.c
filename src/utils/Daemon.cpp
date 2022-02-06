/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021, 2022 Volker Christian <me@vchrist.at>
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

#include "log/Logger.h"

#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <poll.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <syscall.h>
#include <unistd.h>

#endif // DOXYGEN_SHOULD_SKIP_THIS

#ifndef __NR_pidfd_open
#define __NR_pidfd_open 434 /* System call # on most architectures */
#endif

namespace utils {

    bool Daemon::daemonized = false;

    long Daemon::pidfd_open(pid_t pid, unsigned int flags) {
        return syscall(__NR_pidfd_open, pid, flags);
    }

    void Daemon::daemonize(const std::string& pidFileName) {
        if (!std::filesystem::exists(pidFileName)) {
            pid_t pid = 0;

            /* Fork off the parent process */
            pid = fork();

            /* An error occurred */
            if (pid < 0) {
                exit(EXIT_FAILURE);
            }

            /* Success: Let the parent terminate */
            if (pid > 0) {
                exit(EXIT_SUCCESS);
            }

            /* On success: The child process becomes session leader */
            if (setsid() < 0) {
                exit(EXIT_FAILURE);
            }

            /* Ignore signal sent from child to parent process */
            signal(SIGCHLD, SIG_IGN);

            /* Fork off for the second time*/
            pid = fork();

            /* An error occurred */
            if (pid < 0) {
                exit(EXIT_FAILURE);
            }

            /* Success: Let the parent terminate */
            if (pid > 0) {
                exit(EXIT_SUCCESS);
            }

            /* Set new file permissions */
            umask(0);

            /* Change the working directory to the root directory */
            /* or another appropriated directory */
            chdir("/");

            /* Close all open file descriptors */
            for (long fd = sysconf(_SC_OPEN_MAX); fd >= 0; fd--) {
                close(static_cast<int>(fd));
            }

            /* Reopen stdin (fd = 0), stdout (fd = 1), stderr (fd = 2) */
            if (std::freopen("/dev/null", "r", stdin) == nullptr) {
            }
            if (std::freopen("/dev/null", "w+", stdout) == nullptr) {
            }
            if (std::freopen("/dev/null", "w+", stderr) == nullptr) {
            }

            /* Try to write PID of daemon to lockfile */
            if (!pidFileName.empty()) {
                std::ofstream pidFile(pidFileName, std::ofstream::out);

                if (pidFile.good()) {
                    pidFile << getpid() << std::endl;
                    pidFile.close();
                } else {
                    exit(EXIT_FAILURE);
                }
            }
            daemonized = true;
        } else {
            VLOG(0) << "Already running: Not daemonized ... exiting";
            daemonized = false;
        }
    }

    void Daemon::kill(const std::string& pidFileName) {
        /* Try to read PID of daemon to from lockfile and kill the daemon */
        if (!pidFileName.empty()) {
            std::ifstream pidFile(pidFileName, std::ifstream::in);

            if (pidFile.good()) {
                int pid;
                pidFile >> pid;

                struct pollfd pollfd;

                int pidfd = static_cast<int>(pidfd_open(pid, 0));

                if (pidfd == -1) {
                    VLOG(0) << "Daemon not running";
                    exit(EXIT_FAILURE);
                }

                if (::kill(pid, SIGTERM) == 0) {
                    pidFile.close();

                    pollfd.fd = pidfd;
                    pollfd.events = POLLIN;

                    int ready = poll(&pollfd, 1, -1);
                    if (ready == -1) {
                        PLOG(ERROR) << "Poll";
                        exit(EXIT_FAILURE);
                    } else if (ready == 0) {
                        PLOG(ERROR) << "Daemon not responding";
                        exit(EXIT_FAILURE);
                    } else {
                        VLOG(0) << "Daemon terminated";
                    }

                    close(pidfd);
                    exit(EXIT_SUCCESS);
                } else {
                    PLOG(ERROR) << "Kill daemon";
                    exit(EXIT_FAILURE);
                }
            } else {
                VLOG(0) << "Daemon not running";

                exit(EXIT_FAILURE);
            }
        }
    }

    void Daemon::erasePidFile(const std::string& pidFileName) {
        if (daemonized) {
            std::remove(pidFileName.data());
        }
    }

} // namespace utils
