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
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cstring>
#include <spdlog/logger.h>
#include <spdlog/pattern_formatter.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <unistd.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace {
    std::shared_ptr<spdlog::sinks::stdout_color_sink_st> stdoutSink;
    std::shared_ptr<spdlog::sinks::rotating_file_sink_st> fileSink;
    std::shared_ptr<spdlog::logger> stdoutLogger;
    std::shared_ptr<spdlog::logger> fileLogger;

    int configuredLogLevel = 0;
    int configuredVerboseLevel = 0;
    bool quietMode = false;
    logger::Logger::TickResolver tickResolver;

    using Clock = std::chrono::steady_clock;
    Clock::time_point startTime = Clock::now();

    std::string defaultTick();

    class TickFlagFormatter final : public spdlog::custom_flag_formatter {
    public:
        void format(const spdlog::details::log_msg&, const std::tm&, spdlog::memory_buf_t& dest) override {
            std::string tick = tickResolver ? tickResolver() : defaultTick();
            dest.append(tick.data(), tick.data() + static_cast<std::ptrdiff_t>(tick.size()));
        }

        std::unique_ptr<custom_flag_formatter> clone() const override {
            return spdlog::details::make_unique<TickFlagFormatter>();
        }
    };

    std::string defaultTick() {
        const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - startTime).count();
        std::string tick = std::to_string(elapsed);
        if (tick.size() < 13) {
            tick.insert(0, 13 - tick.size(), '0');
        }
        return tick;
    }

    std::string levelName(const logger::Level level) {
        std::string result;

        switch (level) {
            case logger::Level::TRACE:
                result = "TRACE  ";
                break;
            case logger::Level::DEBUG:
                result = "DEBUG  ";
                break;
            case logger::Level::INFO:
                result = "INFO   ";
                break;
            case logger::Level::WARNING:
                result = "WARNING";
                break;
            case logger::Level::ERROR:
                result = "ERROR  ";
                break;
            case logger::Level::FATAL:
                result = "FATAL  ";
                break;
            case logger::Level::VERBOSE:
                result = "VERBOSE";
                break;
        }

        return result;
    }

    Color::Code levelColor(const logger::Level level) {
        Color::Code color = Color::Code::FG_WHITE;

        switch (level) {
            case logger::Level::TRACE:
                color = Color::Code::FG_MAGENTA; // +
                break;
            case logger::Level::DEBUG:
                color = Color::Code::FG_LIGHT_GREEN; // +
                break;
            case logger::Level::INFO:
                color = Color::Code::FG_LIGHT_YELLOW; // +
                break;
            case logger::Level::WARNING:
                color = Color::Code::FG_YELLOW; // +
                break;
            case logger::Level::ERROR:
                color = Color::Code::FG_RED; // +
                break;
            case logger::Level::FATAL:
                color = Color::Code::FG_LIGHT_RED; // +
                break;
            case logger::Level::VERBOSE:
                color = Color::Code::FG_WHITE;
                break;
        }

        return color;
    }

    std::string colorizeLevel(const logger::Level level) {
        std::string label = levelName(level);

        if (!logger::Logger::getDisableColor()) {
            label = Color::Code::FG_DEFAULT + (levelColor(level) + label) + Color::Code::FG_DEFAULT;
        }

        return label;
    }

    bool shouldEmit(const logger::Level level) {
        if (level == logger::Level::VERBOSE) {
            return true;
        }

        switch (configuredLogLevel) {
            case 6:
                return true;
            case 5:
                return level != logger::Level::TRACE;
            case 4:
                return level == logger::Level::INFO || level == logger::Level::WARNING || level == logger::Level::ERROR ||
                       level == logger::Level::FATAL;
            case 3:
                return level == logger::Level::WARNING || level == logger::Level::ERROR || level == logger::Level::FATAL;
            case 2:
                return level == logger::Level::ERROR || level == logger::Level::FATAL;
            case 1:
                return level == logger::Level::FATAL;
            default:
                return false;
        }
    }
} // namespace

namespace logger {

    void Logger::init() {
        startTime = Clock::now();
        stdoutSink = std::make_shared<spdlog::sinks::stdout_color_sink_st>();
        stdoutLogger = std::make_shared<spdlog::logger>("snodec-stdout", stdoutSink);
        auto stdoutPattern = std::make_unique<spdlog::pattern_formatter>();
        stdoutPattern->add_flag<TickFlagFormatter>('*').set_pattern("%Y-%m-%d %H:%M:%S %* %v");
        stdoutLogger->set_formatter(std::move(stdoutPattern));

        fileSink.reset();
        fileLogger.reset();
        quietMode = false;
        disableColorLog = ::isatty(::fileno(stdout)) == 0;
        configuredVerboseLevel = 0;
        configuredLogLevel = 0;
    }

    void Logger::setTickResolver(TickResolver resolver) {
        tickResolver = std::move(resolver);
    }

    void Logger::setLogLevel(int level) {
        configuredLogLevel = level;
    }

    void Logger::setVerboseLevel(int level) {
        configuredVerboseLevel = std::max(0, level);
    }

    void Logger::logToFile(const std::string& logFile) {
        constexpr std::size_t maxSize = 2 * 1024 * 1024; // 2 MiB
        constexpr std::size_t maxFiles = 3;              // keep log, log.1, log.2, log.3
        fileSink = std::make_shared<spdlog::sinks::rotating_file_sink_st>(logFile, maxSize, maxFiles);
        fileLogger = std::make_shared<spdlog::logger>("snodec-file", fileSink);
        auto filePattern = std::make_unique<spdlog::pattern_formatter>();
        filePattern->add_flag<TickFlagFormatter>('*').set_pattern("%Y-%m-%d %H:%M:%S %* %v");
        fileLogger->set_formatter(std::move(filePattern));
    }

    void Logger::disableLogToFile() {
        fileLogger.reset();
        fileSink.reset();
    }

    void Logger::setQuiet(bool quiet) {
        quietMode = quiet;
    }

    void Logger::setDisableColor(bool disableColor) {
        disableColorLog = disableColor;
    }

    bool Logger::getDisableColor() {
        return disableColorLog;
    }

    bool Logger::shouldLog(Level level) {
        return shouldEmit(level);
    }

    bool Logger::shouldVerbose(int verboseLevel) {
        return verboseLevel >= 0 && verboseLevel <= configuredVerboseLevel;
    }

    static void emitLine(Level level, std::string message, const bool withErrno, const int errnoValue) {
        if (!shouldEmit(level)) {
            return;
        }

        if (withErrno) {
            message += ": ";
            message += std::strerror(errnoValue);
        }

        if (level != Level::VERBOSE) {
            message = colorizeLevel(level) + " " + message;
        }

        if (!quietMode && stdoutLogger) {
            stdoutLogger->log(spdlog::level::info, message);
        }
        if (fileLogger) {
            fileLogger->log(spdlog::level::info, message);
        }
    }

    bool Logger::disableColorLog = false;

    LogMessage::LogMessage(const Level level, const int verboseLevel, const bool withErrno)
        : level(level)
        , verboseLevel(verboseLevel)
        , withErrno(withErrno)
        , enabled(true)
        , errnoValue(errno) {
    }

    LogMessage::~LogMessage() {
        if (enabled) {
            emitLine(level, message.str(), withErrno, errnoValue);
        }
    }

    std::ostringstream& LogMessage::stream() {
        return message;
    }

} // namespace logger

std::ostream& Color::operator<<(std::ostream& os, const Code& code) {
    return os << (!logger::Logger::disableColorLog ? ("\033[" + std::to_string(static_cast<int>(code)) + "m") : "");
}

std::string Color::operator+(const std::string& string, const Code& code) {
    return string + (!logger::Logger::disableColorLog ? ("\033[" + std::to_string(static_cast<int>(code)) + "m") : "");
}

std::string Color::operator+(const Code& code, const std::string& string) {
    return (!logger::Logger::disableColorLog ? ("\033[" + std::to_string(static_cast<int>(code)) + "m") : "") + string;
}
