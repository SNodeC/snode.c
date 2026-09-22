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

#include "utils/hexdump.h"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <mutex>
#include <optional>
#include <spdlog/async_logger.h>
#include <spdlog/details/thread_pool.h>
#include <spdlog/logger.h>
#include <spdlog/sinks/callback_sink.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <unistd.h>
#include <utility>
#include <vector>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace {
    constexpr std::string_view hexDumpMarker = "__SNODEC_HEX_DUMP_PAYLOAD__";

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

    void appendSize(std::string& target, const std::size_t value) {
        const auto size = static_cast<std::uint64_t>(value);
        target.append(reinterpret_cast<const char*>(&size), sizeof(size));
    }

    bool takeSize(std::string_view& source, std::size_t& value) {
        if (source.size() < sizeof(std::uint64_t)) {
            return false;
        }
        std::uint64_t size = 0;
        std::memcpy(&size, source.data(), sizeof(size));
        source.remove_prefix(sizeof(size));
        value = static_cast<std::size_t>(size);
        return static_cast<std::uint64_t>(value) == size;
    }

    bool takeString(std::string_view& source, const std::size_t size, std::string_view& value) {
        if (source.size() < size) {
            return false;
        }
        value = source.substr(0, size);
        source.remove_prefix(size);
        return true;
    }

    std::pair<std::string, std::string> splitHexDumpTemplate(std::string formatted) {
        const std::size_t marker = formatted.rfind(hexDumpMarker);
        if (marker == std::string::npos) {
            return {std::move(formatted), {}};
        }
        std::string suffix = formatted.substr(marker + hexDumpMarker.size());
        formatted.resize(marker);
        return {std::move(formatted), std::move(suffix)};
    }

    void appendJsonEscaped(std::string& target, const std::string_view value) {
        constexpr char digits[] = "0123456789abcdef";
        for (const char valueChar : value) {
            const auto ch = static_cast<unsigned char>(valueChar);
            switch (ch) {
                case '"':
                    target += "\\\"";
                    break;
                case '\\':
                    target += "\\\\";
                    break;
                case '\n':
                    target += "\\n";
                    break;
                case '\r':
                    target += "\\r";
                    break;
                case '\t':
                    target += "\\t";
                    break;
                default:
                    if (ch < 0x20) {
                        target += "\\u00";
                        target += digits[ch >> 4];
                        target += digits[ch & 0x0f];
                    } else {
                        target += static_cast<char>(ch);
                    }
                    break;
            }
        }
    }

    void appendContinued(std::string& target, const std::string_view value, const std::string_view continuation) {
        std::size_t begin = 0;
        while (begin < value.size()) {
            const std::size_t newline = value.find('\n', begin);
            if (newline == std::string_view::npos) {
                target.append(value.substr(begin));
                break;
            }
            target.append(value.substr(begin, newline - begin + 1));
            target.append(continuation);
            begin = newline + 1;
        }
    }
} // namespace

namespace logger::detail {

    class SpdlogBackend::Impl {
    public:
        void init() {
            const std::lock_guard<std::mutex> lock(mutex);
            threadPool.reset();
            semanticStdoutLogger.reset();
            semanticFileLogger.reset();
            semanticHexDumpLogger.reset();
            semanticStdoutSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
            semanticStdoutLogger = makeLogger("snodec-semantic-stdout", semanticStdoutSink);
            semanticFileSink.reset();
            pending.clear();
            deferred = false;
            asyncStarted = false;
            discard = false;
            quietMode = false;
            disableColor = ::isatty(::fileno(stdout)) == 0;
            configuredVerboseLevel = 0;
            configuredLogLevel = 0;
        }

        void defer() {
            const std::lock_guard<std::mutex> lock(mutex);
            deferred = true;
        }

        void startAsync() {
            const std::lock_guard<std::mutex> lock(mutex);
            if (asyncStarted || discard) {
                return;
            }

            constexpr std::size_t queueSize = 8192;
            threadPool = std::make_shared<spdlog::details::thread_pool>(queueSize, 1);

            for (const LogRecord& record : pending) {
                emitSemantic(record, semanticStdoutLogger, semanticFileLogger);
            }
            pending.clear();

            semanticStdoutLogger = makeAsyncLogger("snodec-semantic-stdout", semanticStdoutSink);
            if (semanticFileSink) {
                semanticFileLogger = makeAsyncLogger("snodec-semantic-file", semanticFileSink);
            }
            deferred = false;
            asyncStarted = true;
            updateHexDumpLogger();
        }

        void discardPending() {
            const std::lock_guard<std::mutex> lock(mutex);
            pending.clear();
            discard = true;
        }

        void setQuiet(const bool quiet) {
            const std::lock_guard<std::mutex> lock(mutex);
            quietMode = quiet;
            updateHexDumpLogger();
        }

        void setDisableColor(const bool disableColorValue) {
            const std::lock_guard<std::mutex> lock(mutex);
            disableColor = disableColorValue;
            updateHexDumpLogger();
        }

        bool getDisableColor() const {
            const std::lock_guard<std::mutex> lock(mutex);
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
            const std::lock_guard<std::mutex> lock(mutex);
            constexpr std::size_t maxSize = 2 * 1024 * 1024;
            constexpr std::size_t maxFiles = 3;
            semanticFileSink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(logFile, maxSize, maxFiles);
            if (asyncStarted) {
                semanticFileLogger = makeAsyncLogger("snodec-semantic-file", semanticFileSink);
            } else {
                semanticFileLogger = makeLogger("snodec-semantic-file", semanticFileSink);
            }
            updateHexDumpLogger();
        }

        void disableLogFile() {
            const std::lock_guard<std::mutex> lock(mutex);
            semanticFileLogger.reset();
            semanticFileSink.reset();
            updateHexDumpLogger();
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
            const std::lock_guard<std::mutex> lock(mutex);
            return !quietMode && semanticStdoutSink && !disableColor;
        }

        void emitSemantic(const LogRecord& record) {
            const std::lock_guard<std::mutex> lock(mutex);
            if (discard) {
                return;
            }
            if (deferred) {
                pending.push_back(record);
                return;
            }
            if (record.hexDump && semanticHexDumpLogger) {
                const auto spdlogLevel = mapSemanticLevel(record.level);
                if (spdlogLevel) {
                    semanticHexDumpLogger->log(*spdlogLevel, encodeHexDump(record));
                }
                return;
            }
            emitSemantic(record, semanticStdoutLogger, semanticFileLogger);
        }

    private:
        template <typename Sink>
        static std::shared_ptr<spdlog::logger> makeLogger(const std::string& name, const std::shared_ptr<Sink>& sink) {
            auto logger = std::make_shared<spdlog::logger>(name, sink);
            logger->set_level(spdlog::level::trace);
            logger->set_pattern("%v");
            return logger;
        }

        template <typename Sink>
        std::shared_ptr<spdlog::logger> makeAsyncLogger(const std::string& name, const std::shared_ptr<Sink>& sink) {
            auto logger = std::make_shared<spdlog::async_logger>(name, sink, threadPool, spdlog::async_overflow_policy::block);
            logger->set_level(spdlog::level::trace);
            logger->set_pattern("%v");
            return logger;
        }

        std::string encodeHexDump(const LogRecord& record) const {
            LogRecord templateRecord = record;
            templateRecord.message += '\n';
            templateRecord.message += hexDumpMarker;
            templateRecord.hexDump.reset();

            const bool json = LogManager::format() == LogManager::Format::Json;
            std::pair<std::string, std::string> stdoutTemplate;
            std::pair<std::string, std::string> fileTemplate;
            if (!quietMode && semanticStdoutSink) {
                stdoutTemplate = splitHexDumpTemplate(json ? formatJsonV1(templateRecord) : formatText(templateRecord, !disableColor));
            }
            if (semanticFileSink) {
                fileTemplate = splitHexDumpTemplate(json ? formatJsonV1(templateRecord) : formatText(templateRecord));
            }

            std::string payload;
            appendSize(payload, stdoutTemplate.first.size());
            appendSize(payload, stdoutTemplate.second.size());
            appendSize(payload, fileTemplate.first.size());
            appendSize(payload, fileTemplate.second.size());
            appendSize(payload, record.hexDump->size());
            payload += json ? '\1' : disableColor ? '\0' : '\2';
            payload += stdoutTemplate.first;
            payload += stdoutTemplate.second;
            payload += fileTemplate.first;
            payload += fileTemplate.second;
            payload += *record.hexDump;
            return payload;
        }

        void updateHexDumpLogger() {
            if (!asyncStarted || (!semanticStdoutSink && !semanticFileSink)) {
                semanticHexDumpLogger.reset();
                return;
            }

            auto callbackSink = std::make_shared<spdlog::sinks::callback_sink_mt>(
                [stdoutSink = semanticStdoutSink, fileSink = semanticFileSink](const spdlog::details::log_msg& message) {
                    emitHexDump(message, stdoutSink, fileSink);
                });
            semanticHexDumpLogger = makeAsyncLogger("snodec-semantic-hexdump", callbackSink);
        }

        static void emitHexDump(const spdlog::details::log_msg& message,
                                const std::shared_ptr<spdlog::sinks::stdout_color_sink_mt>& stdoutSink,
                                const std::shared_ptr<spdlog::sinks::rotating_file_sink_mt>& fileSink) {
            std::string_view payload(message.payload.data(), message.payload.size());
            std::size_t stdoutPrefixSize = 0;
            std::size_t stdoutSuffixSize = 0;
            std::size_t filePrefixSize = 0;
            std::size_t fileSuffixSize = 0;
            std::size_t bytesSize = 0;
            if (!takeSize(payload, stdoutPrefixSize) || !takeSize(payload, stdoutSuffixSize) || !takeSize(payload, filePrefixSize) ||
                !takeSize(payload, fileSuffixSize) || !takeSize(payload, bytesSize) || payload.empty()) {
                return;
            }

            const unsigned char mode = static_cast<unsigned char>(payload.front());
            payload.remove_prefix(1);
            std::string_view stdoutPrefix;
            std::string_view stdoutSuffix;
            std::string_view filePrefix;
            std::string_view fileSuffix;
            std::string_view bytes;
            if (!takeString(payload, stdoutPrefixSize, stdoutPrefix) || !takeString(payload, stdoutSuffixSize, stdoutSuffix) ||
                !takeString(payload, filePrefixSize, filePrefix) || !takeString(payload, fileSuffixSize, fileSuffix) ||
                !takeString(payload, bytesSize, bytes) || !payload.empty()) {
                return;
            }

            const std::string plainDump = utils::hexDump(bytes.data(), bytes.size());
            if (stdoutSink && !stdoutPrefix.empty()) {
                std::string output(stdoutPrefix);
                if (mode == 1) {
                    appendJsonEscaped(output, plainDump);
                } else if (mode == 2) {
                    appendContinued(
                        output, utils::hexDump(bytes.data(), bytes.size(), 0, false, utils::terminalHexDumpPalette), "\033[2m│ \033[0m");
                } else {
                    appendContinued(output, plainDump, "│ ");
                }
                output += stdoutSuffix;
                stdoutSink->log({message.time, message.source, message.logger_name, message.level, output});
            }
            if (fileSink && !filePrefix.empty()) {
                std::string output(filePrefix);
                if (mode == 1) {
                    appendJsonEscaped(output, plainDump);
                } else {
                    appendContinued(output, plainDump, "│ ");
                }
                output += fileSuffix;
                fileSink->log({message.time, message.source, message.logger_name, message.level, output});
            }
        }

        void emitSemantic(const LogRecord& record,
                          const std::shared_ptr<spdlog::logger>& stdoutLogger,
                          const std::shared_ptr<spdlog::logger>& fileLogger) const {
            LogRecord renderedRecord = record;
            if (record.hexDump) {
                renderedRecord.message += '\n';
                renderedRecord.message += utils::hexDump(*record.hexDump);
                if (!disableColor) {
                    renderedRecord.terminalMessage =
                        record.message + '\n' +
                        utils::hexDump(record.hexDump->data(), record.hexDump->size(), 0, false, utils::terminalHexDumpPalette);
                }
            }

            const auto spdlogLevel = mapSemanticLevel(renderedRecord.level);
            if (!spdlogLevel || ((!stdoutLogger || quietMode) && !fileLogger)) {
                return;
            }
            const bool json = LogManager::format() == LogManager::Format::Json;
            const bool color = !json && !quietMode && stdoutLogger && !disableColor;
            const std::string plain = json                     ? formatJsonV1(renderedRecord)
                                      : (!color || fileLogger) ? formatText(renderedRecord)
                                                               : std::string();
            if (!quietMode && stdoutLogger) {
                stdoutLogger->log(*spdlogLevel, color ? formatText(renderedRecord, true) : plain);
            }
            if (fileLogger) {
                fileLogger->log(*spdlogLevel, plain);
            }
        }

        std::shared_ptr<spdlog::details::thread_pool> threadPool;
        std::shared_ptr<spdlog::sinks::stdout_color_sink_mt> semanticStdoutSink;
        std::shared_ptr<spdlog::sinks::rotating_file_sink_mt> semanticFileSink;
        std::shared_ptr<spdlog::logger> semanticStdoutLogger;
        std::shared_ptr<spdlog::logger> semanticFileLogger;
        std::shared_ptr<spdlog::logger> semanticHexDumpLogger;
        std::vector<LogRecord> pending;

        Logger::TickResolver tickResolver;
        mutable std::mutex mutex;
        int configuredLogLevel = 0;
        int configuredVerboseLevel = 0;
        bool deferred = false;
        bool asyncStarted = false;
        bool discard = false;
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

    void SpdlogBackend::defer() {
        impl_->defer();
    }

    void SpdlogBackend::startAsync() {
        impl_->startAsync();
    }

    void SpdlogBackend::discardPending() {
        impl_->discardPending();
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

    void SpdlogBackend::emitSemantic(const LogRecord& record) {
        impl_->emitSemantic(record);
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
