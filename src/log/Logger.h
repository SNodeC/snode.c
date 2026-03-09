/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
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

/*
 * MIT License
 */

#ifndef LOGGER_LOGGER_H
#define LOGGER_LOGGER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>
#include <memory>
#include <ostream> // IWYU pragma: export
#include <sstream>
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

    std::ostream& operator<<(std::ostream& os, const Code& code);

    std::string operator+(const std::string& string, const Code& code);
    std::string operator+(const Code& code, const std::string& string);

} // namespace Color

namespace logger {

    enum class Level { TRACE, DEBUG, INFO, WARNING, ERROR, FATAL, VERBOSE };

    class Logger {
    public:
        using TickResolver = std::function<std::string()>;

        Logger() = delete;
        ~Logger() = delete;

        static void init();
        static void setLogLevel(int level);
        static void setVerboseLevel(int level);
        static void logToFile(const std::string& logFile);
        static void disableLogToFile();
        static void setQuiet(bool quiet = true);
        static void setTickResolver(TickResolver resolver);
        static void setDisableColor(bool disableColorLog = true);
        static bool getDisableColor();

        static bool shouldLog(Level level);
        static bool shouldVerbose(int verboseLevel);

    protected:
        static bool disableColorLog;

        friend std::ostream& Color::operator<<(std::ostream& os, const Color::Code& code);
        friend std::string Color::operator+(const std::string& string, const Color::Code& code);
        friend std::string Color::operator+(const Color::Code& code, const std::string& string);
    };

    class LogMessage {
    public:
        LogMessage(Level level, int verboseLevel = -1, bool withErrno = false);
        ~LogMessage();

        std::ostringstream& stream();

    private:
        Level level;
        int verboseLevel;
        bool withErrno;
        bool enabled;
        int errnoValue;
        std::ostringstream message;
    };

} // namespace logger

#ifdef SNODEC_DISABLE_LOGLEVEL_LOGGING
#define LOG(level)                                                                                                                  \
    if (true) {                                                                                                                            \
    } else                                                                                                                                 \
        ::logger::LogMessage(::logger::Level::level).stream()
#define PLOG(level)                                                                                                                 \
    if (true) {                                                                                                                            \
    } else                                                                                                                                 \
        ::logger::LogMessage(::logger::Level::level, -1, true).stream()
#else
#define LOG(level)                                                                                                                  \
    if (!::logger::Logger::shouldLog(::logger::Level::level)) {                                                                            \
    } else                                                                                                                                 \
        ::logger::LogMessage(::logger::Level::level).stream()
#define PLOG(level)                                                                                                                 \
    if (!::logger::Logger::shouldLog(::logger::Level::level)) {                                                                            \
    } else                                                                                                                                 \
        ::logger::LogMessage(::logger::Level::level, -1, true).stream()
#endif

#ifdef SNODEC_DISABLE_VERBOSE_LOGGING
#define VLOG(level)                                                                                                                 \
    if (true) {                                                                                                                            \
    } else                                                                                                                                 \
        ::logger::LogMessage(::logger::Level::VERBOSE, level).stream()
#else
#define VLOG(level)                                                                                                                 \
    if (!::logger::Logger::shouldVerbose(level)) {                                                                                         \
    } else                                                                                                                                 \
        ::logger::LogMessage(::logger::Level::VERBOSE, level).stream()
#endif

#endif // LOGGER_LOGGER_H
