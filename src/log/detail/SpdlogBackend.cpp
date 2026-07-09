/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/detail/SpdlogBackend.h"

#include <algorithm>
#include <chrono>
#include <cstring>
#include <optional>
#include <spdlog/logger.h>
#include <spdlog/pattern_formatter.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <unistd.h>
#include <utility>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace {
    using Clock = std::chrono::steady_clock;

    std::string levelName(const ::logger::Level level) {
        switch (level) {
            case ::logger::Level::TRACE:
                return "TRACE  ";
            case ::logger::Level::DEBUG:
                return "DEBUG  ";
            case ::logger::Level::INFO:
                return "INFO   ";
            case ::logger::Level::WARNING:
                return "WARNING";
            case ::logger::Level::ERROR:
                return "ERROR  ";
            case ::logger::Level::FATAL:
                return "FATAL  ";
            case ::logger::Level::VERBOSE:
                return "VERBOSE";
        }
        return "";
    }

    Color::Code levelColor(const ::logger::Level level) {
        switch (level) {
            case ::logger::Level::TRACE:
                return Color::Code::FG_MAGENTA;
            case ::logger::Level::DEBUG:
                return Color::Code::FG_LIGHT_GREEN;
            case ::logger::Level::INFO:
                return Color::Code::FG_LIGHT_YELLOW;
            case ::logger::Level::WARNING:
                return Color::Code::FG_YELLOW;
            case ::logger::Level::ERROR:
                return Color::Code::FG_RED;
            case ::logger::Level::FATAL:
                return Color::Code::FG_LIGHT_RED;
            case ::logger::Level::VERBOSE:
                return Color::Code::FG_WHITE;
        }
        return Color::Code::FG_WHITE;
    }

    spdlog::level::level_enum mapLegacyLevel(const ::logger::Level level) {
        switch (level) {
            case ::logger::Level::TRACE:
                return spdlog::level::trace;
            case ::logger::Level::DEBUG:
                return spdlog::level::debug;
            case ::logger::Level::INFO:
                return spdlog::level::info;
            case ::logger::Level::WARNING:
                return spdlog::level::warn;
            case ::logger::Level::ERROR:
                return spdlog::level::err;
            case ::logger::Level::FATAL:
                return spdlog::level::critical;
            case ::logger::Level::VERBOSE:
                return spdlog::level::info;
        }
        return spdlog::level::info;
    }

    std::optional<spdlog::level::level_enum> mapSemanticLevel(const ::logger::LogLevel level) {
        switch (level) {
            case ::logger::LogLevel::Trace:
                return spdlog::level::trace;
            case ::logger::LogLevel::Debug:
                return spdlog::level::debug;
            case ::logger::LogLevel::Info:
                return spdlog::level::info;
            case ::logger::LogLevel::Warn:
                return spdlog::level::warn;
            case ::logger::LogLevel::Error:
                return spdlog::level::err;
            case ::logger::LogLevel::Critical:
                return spdlog::level::critical;
            case ::logger::LogLevel::Off:
                return std::nullopt;
        }
        return std::nullopt;
    }
} // namespace

namespace logger::detail {

    class SpdlogBackend::Impl {
    public:
        class TickFlagFormatter final : public spdlog::custom_flag_formatter {
        public:
            explicit TickFlagFormatter(const Impl& backend)
                : backend(backend) {
            }

            void format(const spdlog::details::log_msg&, const std::tm&, spdlog::memory_buf_t& dest) override {
                const std::string tick = backend.tick();
                dest.append(tick.data(), tick.data() + static_cast<std::ptrdiff_t>(tick.size()));
            }

            std::unique_ptr<custom_flag_formatter> clone() const override {
                return spdlog::details::make_unique<TickFlagFormatter>(backend);
            }

        private:
            const Impl& backend;
        };

        void init() {
            startTime = Clock::now();
            stdoutSink = std::make_shared<spdlog::sinks::stdout_color_sink_st>();
            stdoutLogger = std::make_shared<spdlog::logger>("snodec-stdout", stdoutSink);
            stdoutLogger->set_level(spdlog::level::trace);
            stdoutLogger->set_formatter(legacyFormatter());

            semanticStdoutSink = std::make_shared<spdlog::sinks::stdout_color_sink_st>();
            semanticStdoutLogger = std::make_shared<spdlog::logger>("snodec-semantic-stdout", semanticStdoutSink);
            semanticStdoutLogger->set_level(spdlog::level::trace);
            semanticStdoutLogger->set_pattern("%v");

            disableLogFile();
            quietMode = false;
            disableColor = ::isatty(::fileno(stdout)) == 0;
            configuredVerboseLevel = 0;
            configuredLogLevel = 0;
        }

        void setQuiet(const bool quiet) {
            quietMode = quiet;
        }

        void setDisableColor(const bool disableColorValue) {
            disableColor = disableColorValue;
        }

        bool getDisableColor() const {
            return disableColor;
        }

        void setTickResolver(Logger::TickResolver resolver) {
            tickResolver = std::move(resolver);
        }

        void setLogLevel(const int level) {
            configuredLogLevel = level;
        }

        void setVerboseLevel(const int level) {
            configuredVerboseLevel = std::max(0, level);
        }

        void setLogFile(const std::string& logFile) {
            constexpr std::size_t maxSize = 2 * 1024 * 1024;
            constexpr std::size_t maxFiles = 3;
            fileSink = std::make_shared<spdlog::sinks::rotating_file_sink_st>(logFile, maxSize, maxFiles);
            fileLogger = std::make_shared<spdlog::logger>("snodec-file", fileSink);
            fileLogger->set_level(spdlog::level::trace);
            fileLogger->set_formatter(legacyFormatter());

            semanticFileSink = std::make_shared<spdlog::sinks::rotating_file_sink_st>(logFile, maxSize, maxFiles);
            semanticFileLogger = std::make_shared<spdlog::logger>("snodec-semantic-file", semanticFileSink);
            semanticFileLogger->set_level(spdlog::level::trace);
            semanticFileLogger->set_pattern("%v");
        }

        void disableLogFile() {
            semanticFileLogger.reset();
            semanticFileSink.reset();
            fileLogger.reset();
            fileSink.reset();
        }

        bool shouldLog(const Level level) const {
            if (level == Level::VERBOSE) {
                return true;
            }
            switch (configuredLogLevel) {
                case 6:
                    return true;
                case 5:
                    return level != Level::TRACE;
                case 4:
                    return level == Level::INFO || level == Level::WARNING || level == Level::ERROR || level == Level::FATAL;
                case 3:
                    return level == Level::WARNING || level == Level::ERROR || level == Level::FATAL;
                case 2:
                    return level == Level::ERROR || level == Level::FATAL;
                case 1:
                    return level == Level::FATAL;
                default:
                    return false;
            }
        }

        bool shouldVerbose(const int verboseLevel) const {
            return verboseLevel >= 0 && verboseLevel <= configuredVerboseLevel;
        }

        void emitLegacy(const Level level, std::string message, const bool withErrno, const int errnoValue) {
            if (!shouldLog(level)) {
                return;
            }
            if (withErrno) {
                message += ": ";
                message += std::strerror(errnoValue);
            }
            if (level != Level::VERBOSE) {
                std::string label = levelName(level);
                if (!disableColor) {
                    label = Color::Code::FG_DEFAULT + (levelColor(level) + label) + Color::Code::FG_DEFAULT;
                }
                message = label + " " + message;
            }
            const auto spdlogLevel = mapLegacyLevel(level);
            if (!quietMode && stdoutLogger) {
                stdoutLogger->log(spdlogLevel, message);
            }
            if (fileLogger) {
                fileLogger->log(spdlogLevel, message);
            }
        }

        void emitSemantic(const LogLevel level, const std::string& formattedRecord) {
            const auto spdlogLevel = mapSemanticLevel(level);
            if (!spdlogLevel) {
                return;
            }
            if (!quietMode && semanticStdoutLogger) {
                semanticStdoutLogger->log(*spdlogLevel, formattedRecord);
            }
            if (semanticFileLogger) {
                semanticFileLogger->log(*spdlogLevel, formattedRecord);
            }
        }

        std::string tick() const {
            if (tickResolver) {
                return tickResolver();
            }

            const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - startTime).count();
            std::string tick = std::to_string(elapsed);
            if (tick.size() < 13) {
                tick.insert(0, 13 - tick.size(), '0');
            }
            return tick;
        }

        std::unique_ptr<spdlog::formatter> legacyFormatter() const {
            auto formatter = std::make_unique<spdlog::pattern_formatter>();
            formatter->add_flag<TickFlagFormatter>('*', *this).set_pattern("%Y-%m-%d %H:%M:%S %* %v");
            return formatter;
        }

    private:
        std::shared_ptr<spdlog::sinks::stdout_color_sink_st> stdoutSink;
        std::shared_ptr<spdlog::sinks::stdout_color_sink_st> semanticStdoutSink;
        std::shared_ptr<spdlog::sinks::rotating_file_sink_st> fileSink;
        std::shared_ptr<spdlog::sinks::rotating_file_sink_st> semanticFileSink;
        std::shared_ptr<spdlog::logger> stdoutLogger;
        std::shared_ptr<spdlog::logger> fileLogger;
        std::shared_ptr<spdlog::logger> semanticStdoutLogger;
        std::shared_ptr<spdlog::logger> semanticFileLogger;

        Logger::TickResolver tickResolver;
        Clock::time_point startTime = Clock::now();
        int configuredLogLevel = 0;
        int configuredVerboseLevel = 0;
        bool quietMode = false;
        bool disableColor = false;
    };

    SpdlogBackend::SpdlogBackend()
        : impl_(std::make_unique<Impl>()) {
    }

    SpdlogBackend::~SpdlogBackend() = default;

    void SpdlogBackend::init() {
        impl_->init();
    }

    void SpdlogBackend::setQuiet(const bool quiet) {
        impl_->setQuiet(quiet);
    }

    void SpdlogBackend::setDisableColor(const bool disableColor) {
        impl_->setDisableColor(disableColor);
    }

    bool SpdlogBackend::getDisableColor() const {
        return impl_->getDisableColor();
    }

    void SpdlogBackend::setTickResolver(Logger::TickResolver resolver) {
        impl_->setTickResolver(std::move(resolver));
    }

    void SpdlogBackend::setLogFile(const std::string& logFile) {
        impl_->setLogFile(logFile);
    }

    void SpdlogBackend::disableLogFile() {
        impl_->disableLogFile();
    }

    void SpdlogBackend::emitLegacy(const Level level, std::string message, const bool withErrno, const int errnoValue) {
        impl_->emitLegacy(level, std::move(message), withErrno, errnoValue);
    }

    void SpdlogBackend::emitSemantic(const LogLevel level, const std::string& formattedRecord) {
        impl_->emitSemantic(level, formattedRecord);
    }

    bool SpdlogBackend::shouldLog(const Level level) const {
        return impl_->shouldLog(level);
    }

    bool SpdlogBackend::shouldVerbose(const int verboseLevel) const {
        return impl_->shouldVerbose(verboseLevel);
    }

    void SpdlogBackend::setLogLevel(const int level) {
        impl_->setLogLevel(level);
    }

    void SpdlogBackend::setVerboseLevel(const int level) {
        impl_->setVerboseLevel(level);
    }

} // namespace logger::detail
