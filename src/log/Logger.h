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

#ifndef LOGGER_LOGGER_H
#define LOGGER_LOGGER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#ifdef __GNUC__
#pragma GCC diagnostic push
#ifdef __has_warning
#if __has_warning("-Wdocumentation-unknown-command")
#pragma GCC diagnostic ignored "-Wdocumentation-unknown-command"
#endif
#endif
#endif
#include <easylogging++.h> // IWYU pragma: export
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace logger {

    class Logger {
    public:
        Logger() = delete;
        ~Logger() = delete;

        // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays, hicpp-avoid-c-arrays, modernize-avoid-c-arrays)
        static void init();

        static void setLogLevel(int level);

        static void setVerboseLevel(int level);

        static void logToFile(const std::string& logFile);

        static void quiet();

        static void setCustomFormatSpec(const char* format, const el::FormatSpecifierValueResolver& resolver);

    protected:
        //        static el::Configurations conf;
    };

} // namespace logger

#endif // LOGGER_LOGGER_H
