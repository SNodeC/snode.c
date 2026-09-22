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
#include <limits>
#include <mutex>
#include <optional>
#include <spdlog/async_logger.h>
#include <spdlog/details/thread_pool.h>
#include <spdlog/logger.h>
#include <spdlog/sinks/callback_sink.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <type_traits>
#include <unistd.h>
#include <utility>
#include <vector>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace {
    enum class RenderMode : std::uint8_t { Plain, Color, Json };

    enum PayloadFlag : std::uint16_t {
        HasInstance = 1U << 0U,
        HasRole = 1U << 1U,
        HasConnection = 1U << 2U,
        HasEvent = 1U << 3U,
        HasTerminalMessage = 1U << 4U,
        HasError = 1U << 5U,
        HasSource = 1U << 6U,
        HasHexDump = 1U << 7U
    };

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

    template <typename Value>
    void appendValue(std::string& target, const Value value) {
        static_assert(std::is_trivially_copyable_v<Value>);
        target.append(reinterpret_cast<const char*>(&value), sizeof(value));
    }

    template <typename Value>
    bool takeValue(std::string_view& source, Value& value) {
        static_assert(std::is_trivially_copyable_v<Value>);
        if (source.size() < sizeof(value)) {
            return false;
        }
        std::memcpy(&value, source.data(), sizeof(value));
        source.remove_prefix(sizeof(value));
        return true;
    }

    void appendString(std::string& target, const std::string_view value) {
        appendValue(target, static_cast<std::uint64_t>(value.size()));
        target.append(value);
    }

    bool takeString(std::string_view& source, std::string& value) {
        std::uint64_t encodedSize = 0;
        if (!takeValue(source, encodedSize)) {
            return false;
        }
        const auto size = static_cast<std::size_t>(encodedSize);
        if (static_cast<std::uint64_t>(size) != encodedSize || source.size() < size) {
            return false;
        }
        value.assign(source.data(), size);
        source.remove_prefix(size);
        return true;
    }
} // namespace

namespace logger::detail {

    class SpdlogBackend::Impl {
    public:
        void init() {
            const std::lock_guard<std::mutex> lock(mutex);
            semanticWorkerLogger.reset();
            semanticStdoutLogger.reset();
            semanticFileLogger.reset();
            threadPool.reset();
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

            deferred = false;
            asyncStarted = true;
            updateWorkerLogger();
        }

        void discardPending() {
            const std::lock_guard<std::mutex> lock(mutex);
            pending.clear();
            discard = true;
        }

        void setQuiet(const bool quiet) {
            const std::lock_guard<std::mutex> lock(mutex);
            quietMode = quiet;
            updateWorkerLogger();
        }

        void setDisableColor(const bool disableColorValue) {
            const std::lock_guard<std::mutex> lock(mutex);
            disableColor = disableColorValue;
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
            semanticFileLogger = makeLogger("snodec-semantic-file", semanticFileSink);
            updateWorkerLogger();
        }

        void disableLogFile() {
            const std::lock_guard<std::mutex> lock(mutex);
            semanticFileLogger.reset();
            semanticFileSink.reset();
            updateWorkerLogger();
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
            if (asyncStarted) {
                const auto spdlogLevel = mapSemanticLevel(record.level);
                if (spdlogLevel && semanticWorkerLogger) {
                    semanticWorkerLogger->log(*spdlogLevel, encodeRecord(record));
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

        struct DecodedRecord {
            LogRecord record;
            RenderMode mode;
        };

        struct RenderedRecord {
            spdlog::level::level_enum level;
            std::string plain;
            std::string terminal;
        };

        std::string encodeRecord(const LogRecord& record) const {
            const RenderMode mode = LogManager::format() == LogManager::Format::Json ? RenderMode::Json
                                    : disableColor                                   ? RenderMode::Plain
                                                                                     : RenderMode::Color;
            std::uint16_t flags = 0;
            if (record.instance)
                flags |= HasInstance;
            if (record.role)
                flags |= HasRole;
            if (record.connection)
                flags |= HasConnection;
            if (record.event)
                flags |= HasEvent;
            if (record.terminalMessage)
                flags |= HasTerminalMessage;
            if (record.error)
                flags |= HasError;
            if (record.source)
                flags |= HasSource;
            if (record.hexDump)
                flags |= HasHexDump;

            std::string payload;
            appendValue(payload, std::uint8_t{1});
            appendValue(payload, static_cast<std::int64_t>(record.v));
            appendValue(
                payload,
                static_cast<std::int64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(record.ts.time_since_epoch()).count()));
            appendValue(payload, static_cast<std::uint8_t>(record.level));
            appendValue(payload, static_cast<std::uint8_t>(record.origin));
            appendValue(payload, static_cast<std::uint8_t>(record.boundary));
            appendValue(payload, static_cast<std::uint8_t>(record.messageFormat));
            appendValue(payload, static_cast<std::uint8_t>(mode));
            appendValue(payload, flags);
            appendValue(payload, static_cast<std::uint8_t>(record.role.value_or(LogRole::Unknown)));
            appendValue(payload, static_cast<std::int64_t>(record.error ? record.error->code : 0));
            appendValue(payload, static_cast<std::int64_t>(record.source ? record.source->line : 0));
            appendString(payload, record.component);
            appendString(payload, record.message);
            if (record.instance)
                appendString(payload, *record.instance);
            if (record.connection)
                appendString(payload, *record.connection);
            if (record.event)
                appendString(payload, *record.event);
            if (record.terminalMessage)
                appendString(payload, *record.terminalMessage);
            if (record.error)
                appendString(payload, record.error->text);
            if (record.source) {
                appendString(payload, record.source->file);
                appendString(payload, record.source->func);
            }
            if (record.hexDump)
                appendString(payload, *record.hexDump);
            appendValue(payload, static_cast<std::uint64_t>(record.messageArguments.size()));
            for (const std::string& argument : record.messageArguments)
                appendString(payload, argument);
            return payload;
        }

        static std::optional<DecodedRecord> decodeRecord(const spdlog::details::log_msg& message) {
            std::string_view payload(message.payload.data(), message.payload.size());
            std::uint8_t payloadVersion = 0;
            std::int64_t recordVersion = 0;
            std::int64_t timestamp = 0;
            std::uint8_t level = 0;
            std::uint8_t origin = 0;
            std::uint8_t boundary = 0;
            std::uint8_t messageFormat = 0;
            std::uint8_t mode = 0;
            std::uint16_t flags = 0;
            std::uint8_t role = 0;
            std::int64_t errorCode = 0;
            std::int64_t sourceLine = 0;
            if (!takeValue(payload, payloadVersion) || !takeValue(payload, recordVersion) || !takeValue(payload, timestamp) ||
                !takeValue(payload, level) || !takeValue(payload, origin) || !takeValue(payload, boundary) ||
                !takeValue(payload, messageFormat) || !takeValue(payload, mode) || !takeValue(payload, flags) ||
                !takeValue(payload, role) || !takeValue(payload, errorCode) || !takeValue(payload, sourceLine) || payloadVersion != 1 ||
                recordVersion < std::numeric_limits<int>::min() || recordVersion > std::numeric_limits<int>::max() ||
                level > static_cast<std::uint8_t>(LogLevel::Off) || origin > static_cast<std::uint8_t>(LogOrigin::Application) ||
                boundary > static_cast<std::uint8_t>(LogBoundary::System) ||
                messageFormat > static_cast<std::uint8_t>(LogMessageFormat::Strict) || mode > static_cast<std::uint8_t>(RenderMode::Json) ||
                role > static_cast<std::uint8_t>(LogRole::Client) || errorCode < std::numeric_limits<int>::min() ||
                errorCode > std::numeric_limits<int>::max() || sourceLine < std::numeric_limits<int>::min() ||
                sourceLine > std::numeric_limits<int>::max()) {
                return std::nullopt;
            }

            DecodedRecord decoded;
            LogRecord& record = decoded.record;
            record.v = static_cast<int>(recordVersion);
            record.ts = std::chrono::system_clock::time_point(std::chrono::nanoseconds(timestamp));
            record.level = static_cast<LogLevel>(level);
            record.origin = static_cast<LogOrigin>(origin);
            record.boundary = static_cast<LogBoundary>(boundary);
            record.messageFormat = static_cast<LogMessageFormat>(messageFormat);
            decoded.mode = static_cast<RenderMode>(mode);
            if (!takeString(payload, record.component) || !takeString(payload, record.message))
                return std::nullopt;
            if ((flags & HasInstance) != 0) {
                record.instance.emplace();
                if (!takeString(payload, *record.instance))
                    return std::nullopt;
            }
            if ((flags & HasRole) != 0)
                record.role = static_cast<LogRole>(role);
            if ((flags & HasConnection) != 0) {
                record.connection.emplace();
                if (!takeString(payload, *record.connection))
                    return std::nullopt;
            }
            if ((flags & HasEvent) != 0) {
                record.event.emplace();
                if (!takeString(payload, *record.event))
                    return std::nullopt;
            }
            if ((flags & HasTerminalMessage) != 0) {
                record.terminalMessage.emplace();
                if (!takeString(payload, *record.terminalMessage))
                    return std::nullopt;
            }
            if ((flags & HasError) != 0) {
                record.error = LogError{static_cast<int>(errorCode), {}};
                if (!takeString(payload, record.error->text))
                    return std::nullopt;
            }
            if ((flags & HasSource) != 0) {
                record.source = LogSource{{}, static_cast<int>(sourceLine), {}};
                if (!takeString(payload, record.source->file) || !takeString(payload, record.source->func))
                    return std::nullopt;
            }
            if ((flags & HasHexDump) != 0) {
                record.hexDump.emplace();
                if (!takeString(payload, *record.hexDump))
                    return std::nullopt;
            }
            std::uint64_t argumentCount = 0;
            if (!takeValue(payload, argumentCount) || argumentCount > payload.size() / sizeof(std::uint64_t))
                return std::nullopt;
            record.messageArguments.reserve(static_cast<std::size_t>(argumentCount));
            for (std::uint64_t argument = 0; argument < argumentCount; ++argument) {
                record.messageArguments.emplace_back();
                if (!takeString(payload, record.messageArguments.back()))
                    return std::nullopt;
            }
            if (!payload.empty())
                return std::nullopt;
            return decoded;
        }

        static std::optional<RenderedRecord> renderRecord(LogRecord record, const RenderMode mode) {
            const auto spdlogLevel = mapSemanticLevel(record.level);
            if (!spdlogLevel)
                return std::nullopt;
            if (record.messageFormat != LogMessageFormat::None) {
                record.message = formatMessage(record.message, record.messageArguments, record.messageFormat);
                record.messageArguments.clear();
                record.messageFormat = LogMessageFormat::None;
            }
            if (record.hexDump) {
                const std::string heading = record.message;
                record.message += '\n';
                record.message += utils::hexDump(*record.hexDump);
                if (mode == RenderMode::Color) {
                    record.terminalMessage =
                        heading + '\n' +
                        utils::hexDump(record.hexDump->data(), record.hexDump->size(), 0, false, utils::terminalHexDumpPalette);
                }
            }
            const bool json = mode == RenderMode::Json;
            return RenderedRecord{*spdlogLevel,
                                  json ? formatJsonV1(record) : formatText(record),
                                  mode == RenderMode::Color ? formatText(record, true) : std::string()};
        }

        void updateWorkerLogger() {
            const auto stdoutSink = quietMode ? nullptr : semanticStdoutSink;
            if (!asyncStarted || (!stdoutSink && !semanticFileSink)) {
                semanticWorkerLogger.reset();
                return;
            }
            auto callbackSink = std::make_shared<spdlog::sinks::callback_sink_mt>(
                [stdoutSink, fileSink = semanticFileSink](const spdlog::details::log_msg& message) {
                    const auto decoded = decodeRecord(message);
                    if (!decoded)
                        return;
                    const auto rendered = renderRecord(std::move(decoded->record), decoded->mode);
                    if (!rendered)
                        return;
                    if (stdoutSink) {
                        stdoutSink->log({message.time,
                                         message.source,
                                         message.logger_name,
                                         message.level,
                                         rendered->terminal.empty() ? rendered->plain : rendered->terminal});
                    }
                    if (fileSink)
                        fileSink->log({message.time, message.source, message.logger_name, message.level, rendered->plain});
                });
            semanticWorkerLogger = makeAsyncLogger("snodec-semantic-worker", callbackSink);
        }

        void emitSemantic(const LogRecord& record,
                          const std::shared_ptr<spdlog::logger>& stdoutLogger,
                          const std::shared_ptr<spdlog::logger>& fileLogger) const {
            const RenderMode mode = LogManager::format() == LogManager::Format::Json ? RenderMode::Json
                                    : disableColor                                   ? RenderMode::Plain
                                                                                     : RenderMode::Color;
            const auto rendered = renderRecord(record, mode);
            if (!rendered)
                return;
            if (!quietMode && stdoutLogger)
                stdoutLogger->log(rendered->level, rendered->terminal.empty() ? rendered->plain : rendered->terminal);
            if (fileLogger)
                fileLogger->log(rendered->level, rendered->plain);
        }

        std::shared_ptr<spdlog::details::thread_pool> threadPool;
        std::shared_ptr<spdlog::sinks::stdout_color_sink_mt> semanticStdoutSink;
        std::shared_ptr<spdlog::sinks::rotating_file_sink_mt> semanticFileSink;
        std::shared_ptr<spdlog::logger> semanticStdoutLogger;
        std::shared_ptr<spdlog::logger> semanticFileLogger;
        std::shared_ptr<spdlog::logger> semanticWorkerLogger;
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
