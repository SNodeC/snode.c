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

#include "log/Logger.h"

#include <csignal>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <grp.h>
#include <poll.h>
#include <pwd.h>
#include <sys/stat.h>
#include <syscall.h>
#include <unistd.h>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace utils {

    DaemonizeError::DaemonizeError(const std::string& errorMessage)
        : std::runtime_error(errorMessage) {
    }

    Daemon::State Daemon::startDaemon(const std::string& pidFileName, const std::string& userName, const std::string& groupName) {
        State state = State::StartDaemonSuccess;

        if (!std::filesystem::exists(pidFileName)) {
            /* Fork off the parent process */
            pid_t pid = fork();

            if (pid < 0) {
                /* An error occurred */
                state = State::FirstForkFailure;
            } else if (pid > 0) {
                /* Success: Let the parent terminate */
                state = State::FirstForkSuccess;
            } else if (setsid() < 0) {
                /* On success: The child process becomes session leader */
                state = State::SetSidFailure;
            } else if (signal(SIGHUP, SIG_IGN) == SIG_ERR) {
                /* Ignore signal sent from parent to child process */
                state = State::SetSignalsFailure;
            } else {
                /* Fork off for the second time*/
                pid = fork();

                if (pid < 0) {
                    /* An error occurred */
                    state = State::SecondForkFailure;
                } else if (pid > 0) {
                    /* Success: Let the second parent terminate */
                    state = State::SecondForkSuccess;
                } else {
                    /* Try to write PID of daemon to lockfile */
                    std::ofstream pidFile(pidFileName, std::ofstream::out);

                    if (pidFile.good()) {
                        pidFile << getpid() << std::endl;
                        pidFile.close();

                        struct passwd* pw = nullptr;
                        struct group* gr = nullptr;

                        if ((gr = getgrnam(groupName.c_str())) == nullptr) {
                            state = State::ChangeGroupIdFailure;
                        } else if (setegid(gr->gr_gid) != 0) {
                            state = State::ChangeGroupIdFailure;
                        } else if ((pw = getpwnam(userName.c_str())) == nullptr) {
                            state = State::ChangeUserIdFailure;
                        } else if (seteuid(pw->pw_uid) != 0) {
                            state = State::ChangeUserIdFailure;
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

                            state = State::StartDaemonSuccess;
                        }
                    } else {
                        state = State::WritePidFileFailure;
                    }
                }
            }
        } else {
            state = State::PidFileExistsFailure;
        }

        return state;
    }

    void Daemon::stopDaemon(const std::string& pidFileName) {
        if (!pidFileName.empty()) {
            /* Try to read PID of daemon to from lockfile and kill the daemon */
            std::ifstream pidFile(pidFileName, std::ifstream::in);

            if (pidFile.good()) {
                int pid = 0;
                pidFile >> pid;

                struct pollfd pollfd {};

                int pidfd = static_cast<int>(syscall(SYS_pidfd_open, pid, 0));

                if (pidfd == -1) {
                    VLOG(0) << "Daemon not running";
                    erasePidFile(pidFileName);
                } else if (::kill(pid, SIGTERM) == 0) {
                    pidFile.close();

                    pollfd.fd = pidfd;
                    pollfd.events = POLLIN;

                    int ready = poll(&pollfd, 1, 5000);
                    if (ready == -1) {
                        PLOG(ERROR) << "Poll";
                        close(pidfd);
                    } else if (ready == 0) {
                        LOG(WARNING) << "Daemon not responding - killing";
                        ::kill(pid, SIGKILL);
                        erasePidFile(pidFileName);
                    } else {
                        VLOG(0) << "Daemon terminated";
                    }

                    close(pidfd);
                } else {
                    PLOG(ERROR) << "Kill daemon";
                }
            } else {
                VLOG(0) << "Daemon not running";
                pidFile.close();
            }
        }
    }

    void Daemon::erasePidFile(const std::string& pidFileName) {
        (void) seteuid(getuid());               // In case  we are here seteguid can not fail
        (void) setegid(getgid());               // In case we are here setegid can not fail
        (void) std::remove(pidFileName.data()); // In case we are here std::remove can not fail
    }

} // namespace utils
