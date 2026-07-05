#include "log/SemanticLogger.h"

#include <iomanip>
#include <map>
#include <stdexcept>
#include <sstream>
#include <utility>

namespace logger {
namespace {
    int rank(LogLevel level) { return static_cast<int>(level); }
    std::string escapeJson(const std::string& value) {
        std::ostringstream out;
        for (const unsigned char ch : value) {
            switch (ch) {
                case '"': out << "\\\""; break;
                case '\\': out << "\\\\"; break;
                case '\b': out << "\\b"; break;
                case '\f': out << "\\f"; break;
                case '\n': out << "\\n"; break;
                case '\r': out << "\\r"; break;
                case '\t': out << "\\t"; break;
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

}

LogRecord materialize(const LogScope& scope, LogLevel level, std::string message, LogRecordOptions options) {
    LogRecord record;
    record.v = 1;
    record.ts = options.ts;
    record.level = level;
    record.origin = scope.origin;
    record.boundary = scope.boundary;
    record.component = std::string(scope.component);
    if (!scope.instance.empty()) record.instance = std::string(scope.instance);
    if (knownLogRole(scope.role)) record.role = scope.role;
    if (!scope.connection.empty()) record.connection = std::string(scope.connection);
    if (options.event && !options.event->empty()) record.event = std::string(*options.event);
    record.message = std::move(message);
    record.error = std::move(options.error);
    record.source = std::move(options.source);
    return record;
}


void LogManager::init() {
    policy() = SemanticPolicy{};
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

LogLevel LogManager::effectiveLevel(const LogScope& scope) {
    const auto& current = policy();
    if (!scope.instance.empty()) {
        if (const auto it = current.instanceLevels.find(std::string(scope.instance)); it != current.instanceLevels.end()) return it->second;
    }
    if (!scope.component.empty()) {
        if (const auto it = current.componentLevels.find(std::string(scope.component)); it != current.componentLevels.end()) return it->second;
    }
    if (const auto it = current.boundaryLevels.find(scope.boundary); it != current.boundaryLevels.end()) return it->second;
    if (const auto it = current.originLevels.find(scope.origin); it != current.originLevels.end()) return it->second;
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
        case LogLevel::Trace: return "trace";
        case LogLevel::Debug: return "debug";
        case LogLevel::Info: return "info";
        case LogLevel::Warn: return "warn";
        case LogLevel::Error: return "error";
        case LogLevel::Critical: return "critical";
        case LogLevel::Off: return "off";
    }
    return "off";
}
std::string toString(LogOrigin origin) { return origin == LogOrigin::Framework ? "framework" : "application"; }
std::string toString(LogBoundary boundary) {
    switch (boundary) {
        case LogBoundary::Application: return "application";
        case LogBoundary::Configuration: return "configuration";
        case LogBoundary::Instance: return "instance";
        case LogBoundary::Connection: return "connection";
        case LogBoundary::Context: return "context";
        case LogBoundary::System: return "system";
    }
    return "system";
}
std::optional<std::string_view> toString(LogRole role) {
    if (role == LogRole::Server) return "server";
    if (role == LogRole::Client) return "client";
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
    if (record.instance) appendField(out, first, "instance", *record.instance);
    if (record.role) if (auto role = toString(*record.role)) appendField(out, first, "role", std::string(*role));
    if (record.connection) appendField(out, first, "connection", *record.connection);
    if (record.event) appendField(out, first, "event", *record.event);
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

std::string formatText(const LogRecord& record) {
    std::ostringstream out;
    out << formatTimestamp(record.ts) << ' ' << toString(record.level) << ' ' << toString(record.origin) << ' ' << toString(record.boundary) << ' '
        << record.component;
    if (record.instance) out << " instance=" << *record.instance;
    if (record.role) if (auto role = toString(*record.role)) out << " role=" << *role;
    if (record.connection) out << " connection=" << *record.connection;
    out << " - " << record.message;
    if (record.error) out << " error=" << record.error->code << ':' << record.error->text;
    return out.str();
}

LogStream::LogStream(const BoundaryLogger* logger, LogLevel level) : logger(logger), level(level), enabled(logger != nullptr && logger->enabled(level)), flushed(false) {}
LogStream::LogStream(LogStream&& other) noexcept : logger(other.logger), level(other.level), enabled(other.enabled), flushed(other.flushed), stream(std::move(other.stream)) { other.flushed = true; }
LogStream& LogStream::operator=(LogStream&& other) noexcept {
    if (this != &other) {
        if (!flushed && enabled && logger != nullptr) logger->emit(level, stream.str());
        logger = other.logger; level = other.level; enabled = other.enabled; flushed = other.flushed; stream = std::move(other.stream); other.flushed = true;
    }
    return *this;
}
LogStream::~LogStream() { if (!flushed && enabled && logger != nullptr) logger->emit(level, stream.str()); }

bool knownLogRole(LogRole role) noexcept { return role == LogRole::Server || role == LogRole::Client; }

OwnedLogScope copyLogScope(LogScope scope) {
    OwnedLogScope owned{scope.origin, scope.boundary, std::string(scope.component), std::nullopt, std::nullopt, std::nullopt};
    if (!scope.instance.empty()) owned.instance = std::string(scope.instance);
    if (knownLogRole(scope.role)) owned.role = scope.role;
    if (!scope.connection.empty()) owned.connection = std::string(scope.connection);
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

BoundaryLogger BoundaryLogger::createForTest(LogScope scope, Sink sink, LogLevel threshold, Clock clock) { return BoundaryLogger(copyLogScope(scope), std::move(sink), threshold, std::move(clock)); }
BoundaryLogger::BoundaryLogger(OwnedLogScope scope, Sink sink, LogLevel threshold, Clock clock) : scope(std::move(scope)), sink(std::move(sink)), threshold(threshold), clock(std::move(clock)) {}
bool BoundaryLogger::enabled(LogLevel level) const noexcept { return level != LogLevel::Off && rank(level) >= rank(threshold) && threshold != LogLevel::Off; }
LogStream BoundaryLogger::trace() const { return {this, LogLevel::Trace}; }
LogStream BoundaryLogger::debug() const { return {this, LogLevel::Debug}; }
LogStream BoundaryLogger::info() const { return {this, LogLevel::Info}; }
LogStream BoundaryLogger::warn() const { return {this, LogLevel::Warn}; }
LogStream BoundaryLogger::error() const { return {this, LogLevel::Error}; }
LogStream BoundaryLogger::critical() const { return {this, LogLevel::Critical}; }
void BoundaryLogger::emit(LogLevel level, std::string message, LogRecordOptions options) const {
    if (!enabled(level) || !sink) return;
    if (options.ts == std::chrono::system_clock::time_point{}) {
        options.ts = clock ? clock() : std::chrono::system_clock::now();
    }
    sink(materialize(viewLogScope(scope), level, std::move(message), std::move(options)));
}

} // namespace logger
