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

#ifndef UTILS_DAEMON_H
#define UTILS_DAEMON_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <exception>
#include <stdexcept>
#include <string>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace utils {

    class DaemonizeFailure : public std::runtime_error {
    public:
        explicit DaemonizeFailure(const std::string& failureMessage);
    };

    class DaemonizeError : public DaemonizeFailure {
    public:
        explicit DaemonizeError(const std::string& errorMessage);
    };

    class DaemonizeSuccess : public std::exception {};

    class Daemon {
    public:
        Daemon() = delete;
        ~Daemon() = delete;

        static void startDaemon(const std::string& pidFileName, const std::string& userName, const std::string& groupName);
        static void stopDaemon(const std::string& pidFileName);

        static void erasePidFile(const std::string& pidFileName);
    };

} // namespace utils

#endif // UTILS_DAEMON_H
