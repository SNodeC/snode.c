/*
 * SNode.C - a slim toolkit for network communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024
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

#ifndef UTILS_DAEMON_H
#define UTILS_DAEMON_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "core/system/unistd.h"

#include <stdexcept>
#include <string>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace utils {
    class DaemonFailure : public std::runtime_error {
    public:
        explicit DaemonFailure(const std::string& failureMessage);

        ~DaemonFailure() override;
    };

    class DaemonError : public DaemonFailure {
    public:
        explicit DaemonError(const std::string& errorMessage);

        ~DaemonError() override;
    };

    class DaemonSignaled : public std::runtime_error {
    public:
        explicit DaemonSignaled(const std::string& message, pid_t pid);

        ~DaemonSignaled() override;

        pid_t getPid() const;

    private:
        pid_t pid = 0;
    };

    class Daemon {
    public:
        Daemon() = delete;
        ~Daemon() = delete;

        static void startDaemon(const std::string& pidFileName, const std::string& userName, const std::string& groupName);
        static pid_t stopDaemon(const std::string& pidFileName);

        static void erasePidFile(const std::string& pidFileName);
    };

} // namespace utils

#endif // UTILS_DAEMON_H
