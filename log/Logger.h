/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021 Volker Christian <me@vchrist.at>
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

#ifndef LOGGER_H
#define LOGGER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <easylogging++.h> // IWYU pragma: export
#include <string>          // for allocator, string

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

class Logger {
    Logger() = delete;
    ~Logger() = delete;

public:
    enum Level { INFO, DEBUG, WARNING, ERROR, FATAL };

    // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays, hicpp-avoid-c-arrays, modernize-avoid-c-arrays)
    static void init(int argc, char* argv[]);

    static void setLogLevel(int level);

    static void setVerboseLevel(unsigned short level);

    static void logToFile(const std::string& logFile = "");

    static void quiet();

    static void setCustomFormatSpec(const char* format, const el::FormatSpecifierValueResolver& resolver);

protected:
    static el::Configurations conf;
};

#endif // LOGGER_H
