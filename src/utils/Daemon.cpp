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
#include <syscall.h>
#include <unistd.h>

#endif // DOXYGEN_SHOULD_SKIP_THIS

#ifndef __NR_pidfd_open
#define __NR_pidfd_open 434 /* System call # on most architectures */
#endif

namespace utils {

    void Daemon::startDaemon(const std::string& pidFileName) {
        if (!std::filesystem::exists(pidFileName)) {
            pid_t pid = 0;

            /* Fork off the parent process */
            pid = fork();

            if (pid < 0) {
                /* An error occurred */
                exit(EXIT_FAILURE);
            } else if (pid > 0) {
                /* Success: Let the parent terminate */
                exit(EXIT_SUCCESS);
            } else if (setsid() < 0) {
                /* On success: The child process becomes session leader */
                exit(EXIT_FAILURE);
            } else if (signal(SIGHUP, SIG_IGN) == SIG_ERR) {
                /* Ignore signal sent from parent to child process */
                exit(EXIT_FAILURE);
            } else {
                /* Fork off for the second time*/
                pid = fork();

                if (pid < 0) {
                    /* An error occurred */
                    exit(EXIT_FAILURE);
                } else if (pid > 0) {
                    /* Success: Let the parent terminate */
                    exit(EXIT_SUCCESS);
                } else {
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

                    if (!pidFileName.empty()) {
                        /* Try to write PID of daemon to lockfile */
                        std::ofstream pidFile(pidFileName, std::ofstream::out);

                        if (pidFile.good()) {
                            pidFile << getpid() << std::endl;
                            pidFile.close();
                        } else {
                            exit(EXIT_FAILURE);
                        }
                    }
                }
            }
        } else {
            VLOG(0) << "Already running: Not daemonized ... exiting";
            exit(EXIT_FAILURE);
        }
    }

    void Daemon::stopDaemon(const std::string& pidFileName) {
        if (!pidFileName.empty()) {
            /* Try to read PID of daemon to from lockfile and kill the daemon */
            std::ifstream pidFile(pidFileName, std::ifstream::in);

            if (pidFile.good()) {
                int pid = 0;
                pidFile >> pid;

                struct pollfd pollfd;

                int pidfd = static_cast<int>(syscall(__NR_pidfd_open, pid, 0));

                if (pidfd == -1) {
                    VLOG(0) << "Daemon not running";
                    erasePidFile(pidFileName);
                    exit(EXIT_FAILURE);
                } else if (::kill(pid, SIGTERM) == 0) {
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
        (void) std::remove(pidFileName.data());
    }

} // namespace utils
