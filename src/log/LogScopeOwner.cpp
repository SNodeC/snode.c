#include "log/LogScopeOwner.h"

#include <utility>

namespace logger {

    LogScopeOwner::LogScopeOwner(LogOrigin origin,
                                 LogBoundary boundary,
                                 std::string component,
                                 std::optional<std::string> instance,
                                 std::optional<LogRole> role,
                                 std::optional<std::string> connection)
        : ownedScope{origin, boundary, std::move(component), std::move(instance), std::move(role), std::move(connection)} {
        if (ownedScope.role && !knownLogRole(*ownedScope.role)) {
            ownedScope.role = std::nullopt;
        }
        if (ownedScope.instance && ownedScope.instance->empty()) {
            ownedScope.instance = std::nullopt;
        }
        if (ownedScope.connection && ownedScope.connection->empty()) {
            ownedScope.connection = std::nullopt;
        }
    }

    LogScopeOwner LogScopeOwner::fromScope(LogScope scope) {
        return LogScopeOwner(copyLogScope(scope));
    }

    LogScope LogScopeOwner::scope() const noexcept {
        return viewLogScope(ownedScope);
    }

    BoundaryLogger LogScopeOwner::logger(BoundaryLogger::Sink sink, LogLevel threshold, BoundaryLogger::Clock clock) const {
        return BoundaryLogger(ownedScope, std::move(sink), threshold, std::move(clock));
    }

    LogScopeOwner::LogScopeOwner(OwnedLogScope scope) : ownedScope(std::move(scope)) {}

} // namespace logger
