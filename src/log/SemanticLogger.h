#ifndef LOGGER_SEMANTICLOGGER_H
#define LOGGER_SEMANTICLOGGER_H

#include <chrono>
#include <functional>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

namespace logger {

    enum class LogLevel { Trace, Debug, Info, Warn, Error, Critical, Off };
    enum class LogOrigin { Framework, Application };
    enum class LogBoundary { Application, Configuration, Instance, Connection, Context, System };
    enum class LogRole { Unknown, Server, Client };

    struct LogScope {
        LogOrigin origin;
        LogBoundary boundary;
        std::string_view component;
        std::string_view instance;
        LogRole role;
        std::string_view connection;
    };

    struct LogError {
        int code;
        std::string text;
    };

    struct LogSource {
        std::string file;
        int line;
        std::string func;
    };

    struct LogRecord {
        int v = 1;
        std::chrono::system_clock::time_point ts;
        LogLevel level;
        LogOrigin origin;
        LogBoundary boundary;
        std::string component;
        std::optional<std::string> instance;
        std::optional<LogRole> role;
        std::optional<std::string> connection;
        std::optional<std::string> event;
        std::string message;
        std::optional<LogError> error;
        std::optional<LogSource> source;
    };

    struct LogRecordOptions {
        std::optional<std::string_view> event;
        std::optional<LogError> error;
        std::optional<LogSource> source;
        std::chrono::system_clock::time_point ts = {};
    };

    LogRecord materialize(const LogScope& scope, LogLevel level, std::string message, LogRecordOptions options = {});

    std::string toString(LogLevel level);
    std::string toString(LogOrigin origin);
    std::string toString(LogBoundary boundary);
    std::optional<std::string_view> toString(LogRole role);
    std::string formatTimestamp(std::chrono::system_clock::time_point ts);
    std::string formatJsonV1(const LogRecord& record);
    std::string formatText(const LogRecord& record);

    class BoundaryLogger;

    struct OwnedLogScope {
        LogOrigin origin;
        LogBoundary boundary;
        std::string component;
        std::optional<std::string> instance;
        std::optional<LogRole> role;
        std::optional<std::string> connection;
    };

    class LogStream {
    public:
        LogStream() = default;
        LogStream(const BoundaryLogger* logger, LogLevel level);
        LogStream(LogStream&& other) noexcept;
        LogStream& operator=(LogStream&& other) noexcept;
        LogStream(const LogStream&) = delete;
        LogStream& operator=(const LogStream&) = delete;
        ~LogStream();

        template <class T>
        LogStream& operator<<(const T& value) {
            if (enabled) {
                stream << value;
            }
            return *this;
        }

    private:
        const BoundaryLogger* logger = nullptr;
        LogLevel level = LogLevel::Off;
        bool enabled = false;
        bool flushed = true;
        std::ostringstream stream;
    };

    class BoundaryLogger {
    public:
        using Sink = std::function<void(LogRecord)>;
        using Clock = std::function<std::chrono::system_clock::time_point()>;

        static BoundaryLogger createForTest(LogScope scope, Sink sink, LogLevel threshold = LogLevel::Trace, Clock clock = {});

        bool enabled(LogLevel level) const noexcept;

        LogStream trace() const;
        LogStream debug() const;
        LogStream info() const;
        LogStream warn() const;
        LogStream error() const;
        LogStream critical() const;

        template <class... Args>
        void trace(std::string_view format, Args&&... args) const { emitFormatted(LogLevel::Trace, format, std::forward<Args>(args)...); }
        template <class... Args>
        void debug(std::string_view format, Args&&... args) const { emitFormatted(LogLevel::Debug, format, std::forward<Args>(args)...); }
        template <class... Args>
        void info(std::string_view format, Args&&... args) const { emitFormatted(LogLevel::Info, format, std::forward<Args>(args)...); }
        template <class... Args>
        void warn(std::string_view format, Args&&... args) const { emitFormatted(LogLevel::Warn, format, std::forward<Args>(args)...); }
        template <class... Args>
        void error(std::string_view format, Args&&... args) const { emitFormatted(LogLevel::Error, format, std::forward<Args>(args)...); }
        template <class... Args>
        void critical(std::string_view format, Args&&... args) const { emitFormatted(LogLevel::Critical, format, std::forward<Args>(args)...); }

        template <class... Args>
        void sysError(LogLevel level, int errnum, std::string_view format, Args&&... args) const {
            LogRecordOptions options;
            options.error = LogError{errnum, std::error_code(errnum, std::generic_category()).message()};
            emit(level, formatMessage(format, std::forward<Args>(args)...), std::move(options));
        }

        template <class... Args>
        void sysError(LogLevel level, std::error_code errorCode, std::string_view format, Args&&... args) const {
            LogRecordOptions options;
            options.error = LogError{errorCode.value(), errorCode.message()};
            emit(level, formatMessage(format, std::forward<Args>(args)...), std::move(options));
        }

        void emit(LogLevel level, std::string message, LogRecordOptions options = {}) const;

    private:
        BoundaryLogger(OwnedLogScope scope, Sink sink, LogLevel threshold, Clock clock);

        template <class... Args>
        void emitFormatted(LogLevel level, std::string_view format, Args&&... args) const {
            emit(level, formatMessage(format, std::forward<Args>(args)...));
        }

        static std::vector<std::string> collectArgs() { return {}; }
        template <class T, class... Args>
        static std::vector<std::string> collectArgs(T&& value, Args&&... args) {
            std::ostringstream out;
            out << std::forward<T>(value);
            auto values = collectArgs(std::forward<Args>(args)...);
            values.insert(values.begin(), out.str());
            return values;
        }
        template <class... Args>
        static std::string formatMessage(std::string_view format, Args&&... args) {
            const auto values = collectArgs(std::forward<Args>(args)...);
            std::string result;
            std::size_t arg = 0;
            for (std::size_t i = 0; i < format.size(); ++i) {
                if (format[i] == '{' && i + 1 < format.size() && format[i + 1] == '}' && arg < values.size()) {
                    result += values[arg++];
                    ++i;
                } else {
                    result += format[i];
                }
            }
            return result;
        }

        OwnedLogScope scope;
        Sink sink;
        LogLevel threshold;
        Clock clock;

        friend class LogStream;
    };

} // namespace logger

#endif // LOGGER_SEMANTICLOGGER_H
