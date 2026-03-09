/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
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
#include <spdlog/sinks/stdout_color_sinks.h>
#include <unistd.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace {
    std::shared_ptr<spdlog::sinks::stdout_color_sink_mt> stdoutSink;
    std::shared_ptr<spdlog::sinks::basic_file_sink_mt> fileSink;
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
                color = Color::Code::FG_CYAN;
                break;
            case logger::Level::DEBUG:
                color = Color::Code::FG_GREEN;
                break;
            case logger::Level::INFO:
                color = Color::Code::FG_LIGHT_GRAY;
                break;
            case logger::Level::WARNING:
                color = Color::Code::FG_YELLOW;
                break;
            case logger::Level::ERROR:
                color = Color::Code::FG_RED;
                break;
            case logger::Level::FATAL:
                color = Color::Code::FG_LIGHT_RED;
                break;
            case logger::Level::VERBOSE:
                color = Color::Code::FG_LIGHT_BLUE;
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
        stdoutSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
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
        fileSink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(logFile, true);
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
