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

#include "log/SemanticLogger.h"

#include <cctype>
#include <iomanip>
#include <map>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace logger {
    namespace {
        int rank(LogLevel level) {
            return static_cast<int>(level);
        }
        std::string escapeJson(const std::string& value) {
            std::ostringstream out;
            for (const unsigned char ch : value) {
                switch (ch) {
                    case '"':
                        out << "\\\"";
                        break;
                    case '\\':
                        out << "\\\\";
                        break;
                    case '\b':
                        out << "\\b";
                        break;
                    case '\f':
                        out << "\\f";
                        break;
                    case '\n':
                        out << "\\n";
                        break;
                    case '\r':
                        out << "\\r";
                        break;
                    case '\t':
                        out << "\\t";
                        break;
                    default:
                        if (ch < 0x20) {
                            out << "\\u00" << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(ch) << std::dec;
                        } else {
                            out << static_cast<char>(ch);
                        }
                        break;
                }
            }
            return out.str();
        }
        void appendField(std::ostringstream& out, bool& first, std::string_view name, const std::string& value) {
            out << (first ? "" : ",") << "\"" << name << "\":\"" << escapeJson(value) << "\"";
            first = false;
        }
        void appendInt(std::ostringstream& out, bool& first, std::string_view name, int value) {
            out << (first ? "" : ",") << "\"" << name << "\":" << value;
            first = false;
        }

        struct SemanticPolicy {
            LogLevel globalLevel = LogLevel::Trace;
            std::map<LogOrigin, LogLevel> originLevels;
            std::map<LogBoundary, LogLevel> boundaryLevels;
            std::map<std::string, LogLevel> componentLevels;
            std::map<std::string, LogLevel> instanceLevels;
            LogManager::Format format = LogManager::Format::Text;
            bool frozen = false;
            unsigned long generation = 0;
        };

        SemanticPolicy& policy() {
            static SemanticPolicy current;
            return current;
        }

        void ensureMutable() {
            if (policy().frozen) {
                throw std::logic_error("logger::LogManager semantic policy is frozen");
            }
        }

        void ensureNonEmptyKey(const std::string& key, const char* name) {
            if (key.empty()) {
                throw std::invalid_argument(std::string("logger::LogManager ") + name + " key must not be empty");
            }
        }

    } // namespace

    LogRecord materialize(const LogScope& scope, LogLevel level, std::string message, LogRecordOptions options) {
        LogRecord record;
        record.v = 1;
        record.ts = options.ts;
        record.level = level;
        record.origin = scope.origin;
        record.boundary = scope.boundary;
        record.component = std::string(scope.component);
        if (!scope.instance.empty())
            record.instance = std::string(scope.instance);
        if (knownLogRole(scope.role))
            record.role = scope.role;
        if (!scope.connection.empty())
            record.connection = std::string(scope.connection);
        if (options.event && !options.event->empty())
            record.event = std::string(*options.event);
        record.message = std::move(message);
        record.error = std::move(options.error);
        record.source = std::move(options.source);
        return record;
    }

    void LogManager::init() {
        const unsigned long nextGeneration = policy().generation + 1;
        policy() = SemanticPolicy{};
        policy().generation = nextGeneration;
    }

    void LogManager::setGlobalLevel(LogLevel level) {
        ensureMutable();
        policy().globalLevel = level;
    }

    void LogManager::setOriginLevel(LogOrigin origin, LogLevel level) {
        ensureMutable();
        policy().originLevels[origin] = level;
    }

    void LogManager::setBoundaryLevel(LogBoundary boundary, LogLevel level) {
        ensureMutable();
        policy().boundaryLevels[boundary] = level;
    }

    void LogManager::setComponentLevel(std::string component, LogLevel level) {
        ensureMutable();
        ensureNonEmptyKey(component, "component");
        policy().componentLevels[std::move(component)] = level;
    }

    void LogManager::setInstanceLevel(std::string instance, LogLevel level) {
        ensureMutable();
        ensureNonEmptyKey(instance, "instance");
        policy().instanceLevels[std::move(instance)] = level;
    }

    void LogManager::setFormat(Format format) {
        ensureMutable();
        policy().format = format;
    }

    void LogManager::freeze() {
        policy().frozen = true;
    }

    bool LogManager::isFrozen() {
        return policy().frozen;
    }

    unsigned long LogManager::generation() {
        return policy().generation;
    }

    LogLevel LogManager::effectiveLevel(const LogScope& scope) {
        const auto& current = policy();
        if (!scope.instance.empty()) {
            if (const auto it = current.instanceLevels.find(std::string(scope.instance)); it != current.instanceLevels.end())
                return it->second;
        }
        if (!scope.component.empty()) {
            if (const auto it = current.componentLevels.find(std::string(scope.component)); it != current.componentLevels.end())
                return it->second;
        }
        if (const auto it = current.boundaryLevels.find(scope.boundary); it != current.boundaryLevels.end())
            return it->second;
        if (const auto it = current.originLevels.find(scope.origin); it != current.originLevels.end())
            return it->second;
        return current.globalLevel;
    }

    LogLevel LogManager::effectiveLevel(const LogRecord& record) {
        LogScope scope{record.origin,
                       record.boundary,
                       record.component,
                       record.instance ? std::string_view(*record.instance) : std::string_view(),
                       record.role.value_or(LogRole::Unknown),
                       record.connection ? std::string_view(*record.connection) : std::string_view()};
        return effectiveLevel(scope);
    }

    bool LogManager::shouldEmit(const LogRecord& record) {
        const LogLevel threshold = effectiveLevel(record);
        return record.level != LogLevel::Off && threshold != LogLevel::Off && rank(record.level) >= rank(threshold);
    }

    LogManager::Format LogManager::format() {
        return policy().format;
    }

    std::string LogManager::formatRecord(const LogRecord& record) {
        return format() == Format::Json ? formatJsonV1(record) : formatText(record);
    }

    std::string toString(LogLevel level) {
        switch (level) {
            case LogLevel::Trace:
                return "trace";
            case LogLevel::Debug:
                return "debug";
            case LogLevel::Info:
                return "info";
            case LogLevel::Warn:
                return "warn";
            case LogLevel::Error:
                return "error";
            case LogLevel::Critical:
                return "critical";
            case LogLevel::Off:
                return "off";
        }
        return "off";
    }
    std::string toString(LogOrigin origin) {
        return origin == LogOrigin::Framework ? "framework" : "application";
    }
    std::string toString(LogBoundary boundary) {
        switch (boundary) {
            case LogBoundary::Application:
                return "application";
            case LogBoundary::Configuration:
                return "configuration";
            case LogBoundary::Instance:
                return "instance";
            case LogBoundary::Connection:
                return "connection";
            case LogBoundary::Context:
                return "context";
            case LogBoundary::System:
                return "system";
        }
        return "system";
    }
    std::optional<std::string_view> toString(LogRole role) {
        if (role == LogRole::Server)
            return "server";
        if (role == LogRole::Client)
            return "client";
        return std::nullopt;
    }

    std::string formatTimestamp(std::chrono::system_clock::time_point ts) {
        const auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(ts.time_since_epoch()) % 1000;
        const auto time = std::chrono::system_clock::to_time_t(ts);
        std::tm tm{};
        gmtime_r(&time, &tm);
        std::ostringstream out;
        out << std::put_time(&tm, "%Y-%m-%dT%H:%M:%S") << '.' << std::setw(3) << std::setfill('0') << millis.count() << 'Z';
        return out.str();
    }

    std::string formatJsonV1(const LogRecord& record) {
        std::ostringstream out;
        bool first = true;
        out << "{";
        appendInt(out, first, "v", record.v);
        appendField(out, first, "ts", formatTimestamp(record.ts));
        appendField(out, first, "level", toString(record.level));
        appendField(out, first, "origin", toString(record.origin));
        appendField(out, first, "boundary", toString(record.boundary));
        appendField(out, first, "component", record.component);
        if (record.instance)
            appendField(out, first, "instance", *record.instance);
        if (record.role)
            if (auto role = toString(*record.role))
                appendField(out, first, "role", std::string(*role));
        if (record.connection)
            appendField(out, first, "connection", *record.connection);
        if (record.event)
            appendField(out, first, "event", *record.event);
        appendField(out, first, "message", record.message);
        if (record.error) {
            out << (first ? "" : ",") << "\"error\":{";
            bool errorFirst = true;
            appendInt(out, errorFirst, "code", record.error->code);
            appendField(out, errorFirst, "text", record.error->text);
            out << "}";
            first = false;
        }
        if (record.source) {
            out << (first ? "" : ",") << "\"source\":{";
            bool sourceFirst = true;
            appendField(out, sourceFirst, "file", record.source->file);
            appendInt(out, sourceFirst, "line", record.source->line);
            appendField(out, sourceFirst, "func", record.source->func);
            out << "}";
            first = false;
        }
        out << "}";
        return out.str();
    }

    namespace {
        std::string toLevelLabel(LogLevel level) {
            switch (level) {
                case LogLevel::Trace:
                    return "TRC";
                case LogLevel::Debug:
                    return "DBG";
                case LogLevel::Info:
                    return "INF";
                case LogLevel::Warn:
                    return "WRN";
                case LogLevel::Error:
                    return "ERR";
                case LogLevel::Critical:
                    return "CRT";
                case LogLevel::Off:
                    return "OFF";
            }
            return "OFF";
        }

        std::string ansiForLevel(LogLevel level) {
            switch (level) {
                case LogLevel::Trace:
                    return "\033[35m";
                case LogLevel::Debug:
                    return "\033[36m";
                case LogLevel::Info:
                    return "\033[0m";
                case LogLevel::Warn:
                    return "\033[33m";
                case LogLevel::Error:
                    return "\033[31m";
                case LogLevel::Critical:
                    return "\033[1;31m";
                case LogLevel::Off:
                    return "\033[0m";
            }
            return "\033[0m";
        }

        bool needsQuoting(std::string_view value) {
            if (value.empty()) {
                return true;
            }
            for (const unsigned char ch : value) {
                if (std::isspace(ch) || ch == '"' || ch == '\\' || ch == '=' || ch == '|' || ch == ',' || ch == ';') {
                    return true;
                }
            }
            return false;
        }

        std::string quoteFieldValue(std::string_view value) {
            if (!needsQuoting(value)) {
                return std::string(value);
            }
            std::string quoted;
            quoted.reserve(value.size() + 2);
            quoted.push_back('"');
            for (const char ch : value) {
                switch (ch) {
                    case '\\':
                        quoted += "\\\\";
                        break;
                    case '"':
                        quoted += "\\\"";
                        break;
                    case '\n':
                        quoted += "\\n";
                        break;
                    case '\r':
                        quoted += "\\r";
                        break;
                    case '\t':
                        quoted += "\\t";
                        break;
                    default:
                        quoted.push_back(ch);
                        break;
                }
            }
            quoted.push_back('"');
            return quoted;
        }

        void appendKeyValue(std::ostringstream& out, std::string_view key, std::string_view value, bool colorEnabled) {
            out << ' ';
            if (colorEnabled) {
                out << "\033[2m" << key << "=\033[0m";
            } else {
                out << key << '=';
            }
            out << quoteFieldValue(value);
        }

        std::string sourceValue(const LogSource& source) {
            std::ostringstream out;
            out << source.file << ':' << source.line;
            if (!source.func.empty()) {
                out << ':' << source.func;
            }
            return out.str();
        }

        std::string errorValue(const LogError& error) {
            std::ostringstream out;
            out << error.code << ':' << error.text;
            return out.str();
        }

        std::pair<std::string, std::string> splitFirstLine(std::string_view message) {
            const auto newline = message.find('\n');
            if (newline == std::string_view::npos) {
                return {std::string(message), {}};
            }
            return {std::string(message.substr(0, newline)), std::string(message.substr(newline + 1))};
        }

        void appendContinuation(std::ostringstream& out, std::string_view rest, bool colorEnabled) {
            std::size_t start = 0;
            while (start <= rest.size()) {
                const auto newline = rest.find('\n', start);
                const auto end = newline == std::string_view::npos ? rest.size() : newline;
                out << '\n';
                if (colorEnabled) {
                    out << "\033[2m│\033[0m ";
                } else {
                    out << "│ ";
                }
                out << rest.substr(start, end - start);
                if (newline == std::string_view::npos) {
                    break;
                }
                start = newline + 1;
            }
        }
        std::string formatTextRecord(const LogRecord& record, const bool colorEnabled) {
            std::ostringstream out;
            const std::string timestamp = formatTimestamp(record.ts);
            const std::string level = toLevelLabel(record.level);
            const std::string origin = toString(record.origin);
            const std::string boundary = toString(record.boundary);
            const auto [firstMessageLine, continuation] = splitFirstLine(record.message);

            if (colorEnabled) {
                out << "\033[2m" << timestamp << "\033[0m " << ansiForLevel(record.level) << level << "\033[0m ";
                if (origin == boundary) {
                    out << "\033[2m" << origin << "\033[0m";
                } else {
                    out << "\033[2m" << origin << '/' << boundary << "\033[0m";
                }
                out << ' ' << "\033[36m" << record.component << "\033[0m";
            } else {
                out << timestamp << ' ' << level << ' ';
                if (origin == boundary) {
                    out << origin;
                } else {
                    out << origin << '/' << boundary;
                }
                out << ' ' << record.component;
            }

            if (record.role) {
                if (auto role = toString(*record.role)) {
                    appendKeyValue(out, "role", *role, colorEnabled);
                }
            }
            if (record.instance) {
                appendKeyValue(out, "inst", *record.instance, colorEnabled);
            }
            if (record.connection) {
                appendKeyValue(out, "conn", *record.connection, colorEnabled);
            }
            if (record.event) {
                appendKeyValue(out, "event", *record.event, colorEnabled);
            }
            if (record.error) {
                appendKeyValue(out, "error", errorValue(*record.error), colorEnabled);
            }
            if (record.source) {
                appendKeyValue(out, "src", sourceValue(*record.source), colorEnabled);
            }
            out << " — " << firstMessageLine;
            if (!continuation.empty()) {
                appendContinuation(out, continuation, colorEnabled);
            }
            return out.str();
        }

    } // namespace

    std::string formatText(const LogRecord& record) {
        return formatTextRecord(record, false);
    }

    namespace detail {
        std::string formatTextColored(const LogRecord& record) {
            return formatTextRecord(record, true);
        }
    } // namespace detail

    LogStream::LogStream(const BoundaryLogger* logger, LogLevel level)
        : logger(logger)
        , level(level)
        , enabled(logger != nullptr && logger->enabled(level))
        , flushed(false) {
    }
    LogStream::LogStream(LogStream&& other) noexcept
        : logger(other.logger)
        , level(other.level)
        , enabled(other.enabled)
        , flushed(other.flushed)
        , stream(std::move(other.stream)) {
        other.flushed = true;
    }
    LogStream& LogStream::operator=(LogStream&& other) noexcept {
        if (this != &other) {
            if (!flushed && enabled && logger != nullptr)
                logger->emit(level, stream.str());
            logger = other.logger;
            level = other.level;
            enabled = other.enabled;
            flushed = other.flushed;
            stream = std::move(other.stream);
            other.flushed = true;
        }
        return *this;
    }
    LogStream::~LogStream() {
        if (!flushed && enabled && logger != nullptr)
            logger->emit(level, stream.str());
    }

    bool knownLogRole(LogRole role) noexcept {
        return role == LogRole::Server || role == LogRole::Client;
    }

    OwnedLogScope copyLogScope(LogScope scope) {
        OwnedLogScope owned{scope.origin, scope.boundary, std::string(scope.component), std::nullopt, std::nullopt, std::nullopt};
        if (!scope.instance.empty())
            owned.instance = std::string(scope.instance);
        if (knownLogRole(scope.role))
            owned.role = scope.role;
        if (!scope.connection.empty())
            owned.connection = std::string(scope.connection);
        return owned;
    }

    LogScope viewLogScope(const OwnedLogScope& scope) noexcept {
        return LogScope{scope.origin,
                        scope.boundary,
                        scope.component,
                        scope.instance ? std::string_view(*scope.instance) : std::string_view(),
                        scope.role.value_or(LogRole::Unknown),
                        scope.connection ? std::string_view(*scope.connection) : std::string_view()};
    }

    BoundaryLogger BoundaryLogger::createForTest(LogScope scope, Sink sink, LogLevel threshold, Clock clock) {
        return BoundaryLogger(copyLogScope(scope), std::move(sink), threshold, std::move(clock));
    }
    BoundaryLogger::BoundaryLogger(OwnedLogScope scope, Sink sink, LogLevel threshold, Clock clock)
        : scope(std::move(scope))
        , sink(std::move(sink))
        , threshold(threshold)
        , clock(std::move(clock)) {
    }
    bool BoundaryLogger::enabled(LogLevel level) const noexcept {
        return level != LogLevel::Off && rank(level) >= rank(threshold) && threshold != LogLevel::Off;
    }
    LogStream BoundaryLogger::trace() const {
        return {this, LogLevel::Trace};
    }
    LogStream BoundaryLogger::debug() const {
        return {this, LogLevel::Debug};
    }
    LogStream BoundaryLogger::info() const {
        return {this, LogLevel::Info};
    }
    LogStream BoundaryLogger::warn() const {
        return {this, LogLevel::Warn};
    }
    LogStream BoundaryLogger::error() const {
        return {this, LogLevel::Error};
    }
    LogStream BoundaryLogger::critical() const {
        return {this, LogLevel::Critical};
    }
    void BoundaryLogger::emit(LogLevel level, std::string message, LogRecordOptions options) const {
        if (!enabled(level) || !sink)
            return;
        if (options.ts == std::chrono::system_clock::time_point{}) {
            options.ts = clock ? clock() : std::chrono::system_clock::now();
        }
        sink(materialize(viewLogScope(scope), level, std::move(message), std::move(options)));
    }

} // namespace logger
