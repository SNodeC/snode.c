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

#ifndef UTILS_PROGRESSLOG_H
#define UTILS_PROGRESSLOG_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <deque>
#include <memory>
#include <string>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace core {

    struct ProgressLogEntry {
        ProgressLogEntry(unsigned short level);

    private:
        int errnum = 0;
        unsigned short level = 0;
        std::string message{};

        friend class ProgressLog;
        friend ProgressLogEntry& operator<<(ProgressLogEntry& ple, const std::string& message);
    };

    ProgressLogEntry& operator<<(ProgressLogEntry& ple, const std::string& message);

    class ProgressLog {
    public:
        ProgressLog();
        ProgressLog(const ProgressLog&) = default;

        ProgressLog& operator=(const ProgressLog&) = default;

        ~ProgressLog();

        ProgressLogEntry& addEntry(unsigned int level);

        unsigned int getErrorCount() const;
        bool getHasErrors() const;

        void logProgress() const;

    private:
        std::shared_ptr<std::deque<core::ProgressLogEntry>> progressLog;
        unsigned int errorCount = 0;
    };

} // namespace core

#endif // UTILS_PROGRESSLOG_H
