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

#include "core/ProgressLog.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"
#include "utils/PreserveErrno.h"

#include <cerrno>
#include <cstring>
#include <type_traits>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace core {

    ProgressLogEntry::ProgressLogEntry(unsigned short level)
        : errnum(errno)
        , level(level)
        , message("") {
    }

    ProgressLogEntry& operator<<(ProgressLogEntry& ple, [[maybe_unused]] const std::string& message) {
        ple.message += message;

        return ple;
    }

    ProgressLog::ProgressLog()
        : progressLog(std::make_shared<std::deque<core::ProgressLogEntry>>()) {
    }

    ProgressLog::~ProgressLog() {
    }

    ProgressLogEntry& ProgressLog::addEntry(unsigned int level) {
        if (errno != 0) {
            errorCount++;
        }
        progressLog->emplace_back(static_cast<unsigned short>(level));

        return progressLog->back();
    }

    unsigned int ProgressLog::getErrorCount() const {
        return errorCount;
    }

    bool ProgressLog::getHasErrors() const {
        return errorCount != 0;
    }

    void ProgressLog::logProgress() const {
        utils::PreserveErrno p1;

        for (const ProgressLogEntry& progressLogEntry : *progressLog.get()) {
            utils::PreserveErrno p2(progressLogEntry.errnum);

            if (errno == 0) {
                VLOG(progressLogEntry.level) << progressLogEntry.message << ": " << std::strerror(progressLogEntry.errnum);
            } else {
                PLOG(WARNING) << progressLogEntry.message;
            }
        }
    }

} // namespace core
