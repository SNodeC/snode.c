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
#include <optional>
#include <spdlog/logger.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <unistd.h>
#include <utility>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace {
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
        void init() {
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
            semanticFileSink = std::make_shared<spdlog::sinks::rotating_file_sink_st>(logFile, maxSize, maxFiles);
            semanticFileLogger = std::make_shared<spdlog::logger>("snodec-semantic-file", semanticFileSink);
            semanticFileLogger->set_level(spdlog::level::trace);
            semanticFileLogger->set_pattern("%v");
        }

        void disableLogFile() {
            semanticFileLogger.reset();
            semanticFileSink.reset();
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

        bool semanticStdoutUsesColor() const {
            return !quietMode && semanticStdoutLogger && !disableColor;
        }

        void emitSemantic(const LogLevel level, const std::string& plainRecord, const std::string& coloredRecord) {
            const auto spdlogLevel = mapSemanticLevel(level);
            if (!spdlogLevel) {
                return;
            }
            if (!quietMode && semanticStdoutLogger) {
                semanticStdoutLogger->log(*spdlogLevel, disableColor ? plainRecord : coloredRecord);
            }
            if (semanticFileLogger) {
                semanticFileLogger->log(*spdlogLevel, plainRecord);
            }
        }

    private:
        std::shared_ptr<spdlog::sinks::stdout_color_sink_st> semanticStdoutSink;
        std::shared_ptr<spdlog::sinks::rotating_file_sink_st> semanticFileSink;
        std::shared_ptr<spdlog::logger> semanticStdoutLogger;
        std::shared_ptr<spdlog::logger> semanticFileLogger;

        Logger::TickResolver tickResolver;
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

    void SpdlogBackend::emitSemantic(const LogLevel level, const std::string& plainRecord, const std::string& coloredRecord) {
        impl_->emitSemantic(level, plainRecord, coloredRecord);
    }

    bool SpdlogBackend::semanticStdoutUsesColor() const {
        return impl_->semanticStdoutUsesColor();
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
