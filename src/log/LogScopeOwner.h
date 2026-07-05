#ifndef LOGGER_LOGSCOPEOWNER_H
#define LOGGER_LOGSCOPEOWNER_H

#include "log/SemanticLogger.h"

namespace logger {

    class LogScopeOwner {
    public:
        LogScopeOwner(LogOrigin origin,
                      LogBoundary boundary,
                      std::string component,
                      std::optional<std::string> instance = std::nullopt,
                      std::optional<LogRole> role = std::nullopt,
                      std::optional<std::string> connection = std::nullopt);

        static LogScopeOwner fromScope(LogScope scope);

        LogScopeOwner(const LogScopeOwner&) = default;
        LogScopeOwner& operator=(const LogScopeOwner&) = default;
        LogScopeOwner(LogScopeOwner&&) noexcept = default;
        LogScopeOwner& operator=(LogScopeOwner&&) noexcept = default;
        ~LogScopeOwner() = default;

        LogScope scope() const noexcept;

        BoundaryLogger logger(BoundaryLogger::Sink sink, LogLevel threshold = LogLevel::Trace, BoundaryLogger::Clock clock = {}) const;

    private:
        explicit LogScopeOwner(OwnedLogScope scope);

        OwnedLogScope ownedScope;
    };

} // namespace logger

#endif // LOGGER_LOGSCOPEOWNER_H
