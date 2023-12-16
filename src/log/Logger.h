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

namespace Color {

    enum class Code {
        FG_DEFAULT = 39,
        FG_BLACK = 30,
        FG_RED = 31,
        FG_GREEN = 32,
        FG_YELLOW = 33,
        FG_BLUE = 34,
        FG_MAGENTA = 35,
        FG_CYAN = 36,
        FG_LIGHT_GRAY = 37,
        FG_DARK_GRAY = 90,
        FG_LIGHT_RED = 91,
        FG_LIGHT_GREEN = 92,
        FG_LIGHT_YELLOW = 93,
        FG_LIGHT_BLUE = 94,
        FG_LIGHT_MAGENTA = 95,
        FG_LIGHT_CYAN = 96,
        FG_WHITE = 97,
        BG_RED = 41,
        BG_GREEN = 42,
        BG_BLUE = 44,
        BG_DEFAULT = 49
    };

    std::ostream& operator<<(std::ostream& os, Code code);

} // namespace Color

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

        static void setQuiet(bool quiet = true);

        static void setCustomFormatSpec(const char* format, const el::FormatSpecifierValueResolver& resolver);

    protected:
        //        static el::Configurations conf;
    };

} // namespace logger

#endif // LOGGER_LOGGER_H
