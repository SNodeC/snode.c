#ifndef SNODEC_SEMANTICLOG_H
#define SNODEC_SEMANTICLOG_H

#include "core/socket/stream/SocketConnection.h"
#include "log/LogScopeOwner.h"
#include "log/Logger.h"

#include <optional>
#include <sstream>
#include <string>
#include <utility>

namespace snode::semantic {

    struct LogIdentity {
        std::optional<std::string> instance;
        std::optional<logger::LogRole> role;
        std::optional<std::string> connection;
    };

    inline LogIdentity logIdentity(const core::socket::stream::SocketConnection& connection) {
        return {connection.getInstanceName().empty() ? std::nullopt : std::optional<std::string>(connection.getInstanceName()),
                std::nullopt,
                connection.getConnectionName()};
    }

    inline logger::BoundaryLogger scopedLog(const logger::LogOrigin origin,
                                            const logger::LogBoundary boundary,
                                            const std::string& component,
                                            logger::BoundaryLogger::Sink sink,
                                            logger::BoundaryLogger::Clock clock = {}) {
        return logger::LogScopeOwner(origin, boundary, component).logger(std::move(sink), std::move(clock));
    }

    inline logger::BoundaryLogger scopedLog(const logger::LogOrigin origin,
                                            const logger::LogBoundary boundary,
                                            const std::string& component,
                                            logger::BoundaryLogger::Sink sink,
                                            const logger::LogLevel threshold,
                                            logger::BoundaryLogger::Clock clock = {}) {
        return logger::LogScopeOwner(origin, boundary, component).logger(std::move(sink), threshold, std::move(clock));
    }

    inline logger::BoundaryLogger scopedLog(const logger::LogOrigin origin,
                                            const logger::LogBoundary boundary,
                                            const std::string& component,
                                            LogIdentity identity,
                                            logger::BoundaryLogger::Sink sink,
                                            logger::BoundaryLogger::Clock clock = {}) {
        return logger::LogScopeOwner(
                   origin, boundary, component, std::move(identity.instance), identity.role, std::move(identity.connection))
            .logger(std::move(sink), std::move(clock));
    }

    inline logger::BoundaryLogger scopedLog(const logger::LogOrigin origin,
                                            const logger::LogBoundary boundary,
                                            const std::string& component,
                                            LogIdentity identity,
                                            logger::BoundaryLogger::Sink sink,
                                            const logger::LogLevel threshold,
                                            logger::BoundaryLogger::Clock clock = {}) {
        return logger::LogScopeOwner(
                   origin, boundary, component, std::move(identity.instance), identity.role, std::move(identity.connection))
            .logger(std::move(sink), threshold, std::move(clock));
    }

    inline logger::BoundaryLogger frameworkLog() {
        return scopedLog(logger::LogOrigin::Framework, logger::LogBoundary::System, "framework", logger::Logger::semanticSink());
    }

    inline logger::BoundaryLogger frameworkLog(logger::BoundaryLogger::Sink sink,
                                               const logger::LogLevel threshold = logger::LogLevel::Trace,
                                               logger::BoundaryLogger::Clock clock = {}) {
        return scopedLog(
            logger::LogOrigin::Framework, logger::LogBoundary::System, "framework", std::move(sink), threshold, std::move(clock));
    }

    inline logger::BoundaryLogger coreSystemLog() {
        return scopedLog(logger::LogOrigin::Framework, logger::LogBoundary::System, "core.system", logger::Logger::semanticSink());
    }

    inline logger::BoundaryLogger coreSystemLog(logger::BoundaryLogger::Sink sink,
                                                const logger::LogLevel threshold = logger::LogLevel::Trace,
                                                logger::BoundaryLogger::Clock clock = {}) {
        return scopedLog(
            logger::LogOrigin::Framework, logger::LogBoundary::System, "core.system", std::move(sink), threshold, std::move(clock));
    }

    inline logger::BoundaryLogger coreSocketLog() {
        return scopedLog(logger::LogOrigin::Framework, logger::LogBoundary::Connection, "core.socket", logger::Logger::semanticSink());
    }

    inline logger::BoundaryLogger coreSocketLog(logger::BoundaryLogger::Sink sink,
                                                const logger::LogLevel threshold = logger::LogLevel::Trace,
                                                logger::BoundaryLogger::Clock clock = {}) {
        return scopedLog(
            logger::LogOrigin::Framework, logger::LogBoundary::Connection, "core.socket", std::move(sink), threshold, std::move(clock));
    }

    inline logger::BoundaryLogger netConfigLog() {
        return scopedLog(logger::LogOrigin::Framework, logger::LogBoundary::Configuration, "net.config", logger::Logger::semanticSink());
    }

    inline logger::BoundaryLogger netConfigLog(logger::BoundaryLogger::Sink sink,
                                               const logger::LogLevel threshold = logger::LogLevel::Trace,
                                               logger::BoundaryLogger::Clock clock = {}) {
        return scopedLog(
            logger::LogOrigin::Framework, logger::LogBoundary::Configuration, "net.config", std::move(sink), threshold, std::move(clock));
    }

    inline logger::BoundaryLogger tlsConfigLog() {
        return scopedLog(
            logger::LogOrigin::Framework, logger::LogBoundary::Configuration, "net.config.tls", logger::Logger::semanticSink());
    }

    inline logger::BoundaryLogger tlsConfigLog(logger::BoundaryLogger::Sink sink,
                                               const logger::LogLevel threshold = logger::LogLevel::Trace,
                                               logger::BoundaryLogger::Clock clock = {}) {
        return scopedLog(logger::LogOrigin::Framework,
                         logger::LogBoundary::Configuration,
                         "net.config.tls",
                         std::move(sink),
                         threshold,
                         std::move(clock));
    }

    inline logger::BoundaryLogger mariaDbLog() {
        return scopedLog(logger::LogOrigin::Framework, logger::LogBoundary::Connection, "db.mariadb", logger::Logger::semanticSink());
    }

    inline logger::BoundaryLogger mariaDbLog(logger::BoundaryLogger::Sink sink,
                                             const logger::LogLevel threshold = logger::LogLevel::Trace,
                                             logger::BoundaryLogger::Clock clock = {}) {
        return scopedLog(
            logger::LogOrigin::Framework, logger::LogBoundary::Connection, "db.mariadb", std::move(sink), threshold, std::move(clock));
    }

    inline logger::BoundaryLogger webHttpLog() {
        return scopedLog(logger::LogOrigin::Framework, logger::LogBoundary::Connection, "web.http", logger::Logger::semanticSink());
    }

    inline logger::BoundaryLogger webHttpLog(logger::BoundaryLogger::Sink sink,
                                             const logger::LogLevel threshold = logger::LogLevel::Trace,
                                             logger::BoundaryLogger::Clock clock = {}) {
        return scopedLog(
            logger::LogOrigin::Framework, logger::LogBoundary::Connection, "web.http", std::move(sink), threshold, std::move(clock));
    }

    inline logger::BoundaryLogger expressLog() {
        return scopedLog(logger::LogOrigin::Framework, logger::LogBoundary::Application, "express", logger::Logger::semanticSink());
    }

    inline logger::BoundaryLogger expressLog(logger::BoundaryLogger::Sink sink,
                                             const logger::LogLevel threshold = logger::LogLevel::Trace,
                                             logger::BoundaryLogger::Clock clock = {}) {
        return scopedLog(
            logger::LogOrigin::Framework, logger::LogBoundary::Application, "express", std::move(sink), threshold, std::move(clock));
    }

    inline logger::BoundaryLogger appLog() {
        return scopedLog(logger::LogOrigin::Application, logger::LogBoundary::Application, "app", logger::Logger::semanticSink());
    }

    inline logger::BoundaryLogger appLog(logger::BoundaryLogger::Sink sink,
                                         const logger::LogLevel threshold = logger::LogLevel::Trace,
                                         logger::BoundaryLogger::Clock clock = {}) {
        return scopedLog(
            logger::LogOrigin::Application, logger::LogBoundary::Application, "app", std::move(sink), threshold, std::move(clock));
    }

    inline logger::BoundaryLogger frameworkLog(LogIdentity identity) {
        return scopedLog(
            logger::LogOrigin::Framework, logger::LogBoundary::System, "framework", std::move(identity), logger::Logger::semanticSink());
    }

    inline logger::BoundaryLogger frameworkLog(LogIdentity identity,
                                               logger::BoundaryLogger::Sink sink,
                                               const logger::LogLevel threshold = logger::LogLevel::Trace,
                                               logger::BoundaryLogger::Clock clock = {}) {
        return scopedLog(logger::LogOrigin::Framework,
                         logger::LogBoundary::System,
                         "framework",
                         std::move(identity),
                         std::move(sink),
                         threshold,
                         std::move(clock));
    }

    inline logger::BoundaryLogger coreSystemLog(LogIdentity identity) {
        return scopedLog(
            logger::LogOrigin::Framework, logger::LogBoundary::System, "core.system", std::move(identity), logger::Logger::semanticSink());
    }

    inline logger::BoundaryLogger coreSystemLog(LogIdentity identity,
                                                logger::BoundaryLogger::Sink sink,
                                                const logger::LogLevel threshold = logger::LogLevel::Trace,
                                                logger::BoundaryLogger::Clock clock = {}) {
        return scopedLog(logger::LogOrigin::Framework,
                         logger::LogBoundary::System,
                         "core.system",
                         std::move(identity),
                         std::move(sink),
                         threshold,
                         std::move(clock));
    }

    inline logger::BoundaryLogger coreSocketLog(LogIdentity identity) {
        return scopedLog(logger::LogOrigin::Framework,
                         logger::LogBoundary::Connection,
                         "core.socket",
                         std::move(identity),
                         logger::Logger::semanticSink());
    }

    inline logger::BoundaryLogger coreSocketLog(LogIdentity identity,
                                                logger::BoundaryLogger::Sink sink,
                                                const logger::LogLevel threshold = logger::LogLevel::Trace,
                                                logger::BoundaryLogger::Clock clock = {}) {
        return scopedLog(logger::LogOrigin::Framework,
                         logger::LogBoundary::Connection,
                         "core.socket",
                         std::move(identity),
                         std::move(sink),
                         threshold,
                         std::move(clock));
    }

    inline logger::BoundaryLogger netConfigLog(LogIdentity identity) {
        return scopedLog(logger::LogOrigin::Framework,
                         logger::LogBoundary::Configuration,
                         "net.config",
                         std::move(identity),
                         logger::Logger::semanticSink());
    }

    inline logger::BoundaryLogger netConfigLog(LogIdentity identity,
                                               logger::BoundaryLogger::Sink sink,
                                               const logger::LogLevel threshold = logger::LogLevel::Trace,
                                               logger::BoundaryLogger::Clock clock = {}) {
        return scopedLog(logger::LogOrigin::Framework,
                         logger::LogBoundary::Configuration,
                         "net.config",
                         std::move(identity),
                         std::move(sink),
                         threshold,
                         std::move(clock));
    }

    inline logger::BoundaryLogger tlsConfigLog(LogIdentity identity) {
        return scopedLog(logger::LogOrigin::Framework,
                         logger::LogBoundary::Configuration,
                         "net.config.tls",
                         std::move(identity),
                         logger::Logger::semanticSink());
    }

    inline logger::BoundaryLogger tlsConfigLog(LogIdentity identity,
                                               logger::BoundaryLogger::Sink sink,
                                               const logger::LogLevel threshold = logger::LogLevel::Trace,
                                               logger::BoundaryLogger::Clock clock = {}) {
        return scopedLog(logger::LogOrigin::Framework,
                         logger::LogBoundary::Configuration,
                         "net.config.tls",
                         std::move(identity),
                         std::move(sink),
                         threshold,
                         std::move(clock));
    }

    inline logger::BoundaryLogger mariaDbLog(LogIdentity identity) {
        return scopedLog(logger::LogOrigin::Framework,
                         logger::LogBoundary::Connection,
                         "db.mariadb",
                         std::move(identity),
                         logger::Logger::semanticSink());
    }

    inline logger::BoundaryLogger mariaDbLog(LogIdentity identity,
                                             logger::BoundaryLogger::Sink sink,
                                             const logger::LogLevel threshold = logger::LogLevel::Trace,
                                             logger::BoundaryLogger::Clock clock = {}) {
        return scopedLog(logger::LogOrigin::Framework,
                         logger::LogBoundary::Connection,
                         "db.mariadb",
                         std::move(identity),
                         std::move(sink),
                         threshold,
                         std::move(clock));
    }

    inline logger::BoundaryLogger webHttpLog(LogIdentity identity) {
        return scopedLog(
            logger::LogOrigin::Framework, logger::LogBoundary::Connection, "web.http", std::move(identity), logger::Logger::semanticSink());
    }

    inline logger::BoundaryLogger webHttpLog(LogIdentity identity,
                                             logger::BoundaryLogger::Sink sink,
                                             const logger::LogLevel threshold = logger::LogLevel::Trace,
                                             logger::BoundaryLogger::Clock clock = {}) {
        return scopedLog(logger::LogOrigin::Framework,
                         logger::LogBoundary::Connection,
                         "web.http",
                         std::move(identity),
                         std::move(sink),
                         threshold,
                         std::move(clock));
    }

    inline logger::BoundaryLogger expressLog(const core::socket::stream::SocketConnection& connection) {
        return scopedLog(logger::LogOrigin::Framework,
                         logger::LogBoundary::Application,
                         "express",
                         logIdentity(connection),
                         logger::Logger::semanticSink());
    }

    inline logger::BoundaryLogger expressLog(const core::socket::stream::SocketConnection& connection,
                                             logger::BoundaryLogger::Sink sink,
                                             const logger::LogLevel threshold = logger::LogLevel::Trace,
                                             logger::BoundaryLogger::Clock clock = {}) {
        return scopedLog(logger::LogOrigin::Framework,
                         logger::LogBoundary::Application,
                         "express",
                         logIdentity(connection),
                         std::move(sink),
                         threshold,
                         std::move(clock));
    }

    inline logger::BoundaryLogger expressLog(LogIdentity identity) {
        return scopedLog(
            logger::LogOrigin::Framework, logger::LogBoundary::Application, "express", std::move(identity), logger::Logger::semanticSink());
    }

    inline logger::BoundaryLogger expressLog(LogIdentity identity,
                                             logger::BoundaryLogger::Sink sink,
                                             const logger::LogLevel threshold = logger::LogLevel::Trace,
                                             logger::BoundaryLogger::Clock clock = {}) {
        return scopedLog(logger::LogOrigin::Framework,
                         logger::LogBoundary::Application,
                         "express",
                         std::move(identity),
                         std::move(sink),
                         threshold,
                         std::move(clock));
    }

    inline logger::BoundaryLogger appLog(LogIdentity identity) {
        return scopedLog(
            logger::LogOrigin::Application, logger::LogBoundary::Application, "app", std::move(identity), logger::Logger::semanticSink());
    }

    inline logger::BoundaryLogger appLog(LogIdentity identity,
                                         logger::BoundaryLogger::Sink sink,
                                         const logger::LogLevel threshold = logger::LogLevel::Trace,
                                         logger::BoundaryLogger::Clock clock = {}) {
        return scopedLog(logger::LogOrigin::Application,
                         logger::LogBoundary::Application,
                         "app",
                         std::move(identity),
                         std::move(sink),
                         threshold,
                         std::move(clock));
    }

    class SysErrorStream {
    public:
        SysErrorStream(logger::BoundaryLogger logger, const logger::LogLevel level, const int errnum)
            : logger(std::move(logger))
            , level(level)
            , errnum(errnum)
            , enabled(this->logger.enabled(level)) {
        }

        SysErrorStream(SysErrorStream&&) noexcept = default;
        SysErrorStream(const SysErrorStream&) = delete;
        SysErrorStream& operator=(const SysErrorStream&) = delete;
        SysErrorStream& operator=(SysErrorStream&&) = delete;

        ~SysErrorStream() {
            if (enabled) {
                logger.sysError(level, errnum, "{}", message.str());
            }
        }

        template <class T>
        SysErrorStream& operator<<(const T& value) {
            if (enabled) {
                message << value;
            }
            return *this;
        }

    private:
        logger::BoundaryLogger logger;
        logger::LogLevel level;
        int errnum;
        bool enabled;
        std::ostringstream message;
    };

    inline SysErrorStream sysError(logger::BoundaryLogger logger, const logger::LogLevel level, const int errnum) {
        return SysErrorStream(std::move(logger), level, errnum);
    }

} // namespace snode::semantic

#endif // SNODEC_SEMANTICLOG_H
