#ifndef SNODEC_SEMANTICLOG_H
#define SNODEC_SEMANTICLOG_H

#include "log/LogScopeOwner.h"
#include "log/Logger.h"

#include <cerrno>
#include <optional>
#include <sstream>
#include <string>
#include <utility>

namespace snode::semantic {

    inline logger::BoundaryLogger scopedLog(const logger::LogOrigin origin,
                                            const logger::LogBoundary boundary,
                                            const std::string& component,
                                            logger::BoundaryLogger::Sink sink = logger::Logger::semanticSink(),
                                            const logger::LogLevel threshold = logger::LogLevel::Trace,
                                            logger::BoundaryLogger::Clock clock = {}) {
        return logger::LogScopeOwner(origin, boundary, component).logger(std::move(sink), threshold, std::move(clock));
    }

    inline logger::BoundaryLogger frameworkLog(logger::BoundaryLogger::Sink sink = logger::Logger::semanticSink(),
                                               const logger::LogLevel threshold = logger::LogLevel::Trace,
                                               logger::BoundaryLogger::Clock clock = {}) {
        return scopedLog(
            logger::LogOrigin::Framework, logger::LogBoundary::System, "framework", std::move(sink), threshold, std::move(clock));
    }

    inline logger::BoundaryLogger coreSystemLog(logger::BoundaryLogger::Sink sink = logger::Logger::semanticSink(),
                                                const logger::LogLevel threshold = logger::LogLevel::Trace,
                                                logger::BoundaryLogger::Clock clock = {}) {
        return scopedLog(
            logger::LogOrigin::Framework, logger::LogBoundary::System, "core.system", std::move(sink), threshold, std::move(clock));
    }

    inline logger::BoundaryLogger coreSocketLog(logger::BoundaryLogger::Sink sink = logger::Logger::semanticSink(),
                                                const logger::LogLevel threshold = logger::LogLevel::Trace,
                                                logger::BoundaryLogger::Clock clock = {}) {
        return scopedLog(
            logger::LogOrigin::Framework, logger::LogBoundary::Connection, "core.socket", std::move(sink), threshold, std::move(clock));
    }

    inline logger::BoundaryLogger netConfigLog(logger::BoundaryLogger::Sink sink = logger::Logger::semanticSink(),
                                               const logger::LogLevel threshold = logger::LogLevel::Trace,
                                               logger::BoundaryLogger::Clock clock = {}) {
        return scopedLog(
            logger::LogOrigin::Framework, logger::LogBoundary::Configuration, "net.config", std::move(sink), threshold, std::move(clock));
    }

    inline logger::BoundaryLogger tlsConfigLog(logger::BoundaryLogger::Sink sink = logger::Logger::semanticSink(),
                                               const logger::LogLevel threshold = logger::LogLevel::Trace,
                                               logger::BoundaryLogger::Clock clock = {}) {
        return scopedLog(logger::LogOrigin::Framework,
                         logger::LogBoundary::Configuration,
                         "net.config.tls",
                         std::move(sink),
                         threshold,
                         std::move(clock));
    }

    inline logger::BoundaryLogger mariaDbLog(logger::BoundaryLogger::Sink sink = logger::Logger::semanticSink(),
                                             const logger::LogLevel threshold = logger::LogLevel::Trace,
                                             logger::BoundaryLogger::Clock clock = {}) {
        return scopedLog(
            logger::LogOrigin::Framework, logger::LogBoundary::Connection, "db.mariadb", std::move(sink), threshold, std::move(clock));
    }

    inline logger::BoundaryLogger webHttpLog(logger::BoundaryLogger::Sink sink = logger::Logger::semanticSink(),
                                             const logger::LogLevel threshold = logger::LogLevel::Trace,
                                             logger::BoundaryLogger::Clock clock = {}) {
        return scopedLog(
            logger::LogOrigin::Framework, logger::LogBoundary::Connection, "web.http", std::move(sink), threshold, std::move(clock));
    }

    inline logger::BoundaryLogger expressLog(logger::BoundaryLogger::Sink sink = logger::Logger::semanticSink(),
                                             const logger::LogLevel threshold = logger::LogLevel::Trace,
                                             logger::BoundaryLogger::Clock clock = {}) {
        return scopedLog(
            logger::LogOrigin::Framework, logger::LogBoundary::Application, "express", std::move(sink), threshold, std::move(clock));
    }

    inline logger::BoundaryLogger appLog(logger::BoundaryLogger::Sink sink = logger::Logger::semanticSink(),
                                         const logger::LogLevel threshold = logger::LogLevel::Trace,
                                         logger::BoundaryLogger::Clock clock = {}) {
        return scopedLog(
            logger::LogOrigin::Application, logger::LogBoundary::Application, "app", std::move(sink), threshold, std::move(clock));
    }

    class SysErrorStream {
    public:
        SysErrorStream(logger::BoundaryLogger logger, const logger::LogLevel level, const int errnum = errno)
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

    inline SysErrorStream sysError(logger::BoundaryLogger logger, const logger::LogLevel level, const int errnum = errno) {
        return SysErrorStream(std::move(logger), level, errnum);
    }

} // namespace snode::semantic

#endif // SNODEC_SEMANTICLOG_H
