#ifndef WEB_HTTP_SERVER_SEMANTICLOG_H
#define WEB_HTTP_SERVER_SEMANTICLOG_H

#include "core/socket/stream/SocketConnection.h"
#include "log/LogScopeOwner.h"
#include "log/Logger.h"

#include <optional>
#include <string>
#include <utility>

namespace web::http::server::semantic {

    inline const logger::LogScopeOwner& httpServerLogScope() {
        static const logger::LogScopeOwner scope(logger::LogOrigin::Framework, logger::LogBoundary::Connection, "web.http.server");
        return scope;
    }

    inline const logger::LogScopeOwner& httpServerUpgradeLogScope() {
        static const logger::LogScopeOwner scope(logger::LogOrigin::Framework, logger::LogBoundary::Connection, "web.http.server.upgrade");
        return scope;
    }

    inline logger::BoundaryLogger httpServerLog() {
        return httpServerLogScope().logger(logger::Logger::semanticSink());
    }

    inline logger::BoundaryLogger httpServerLog(const core::socket::stream::SocketConnection& connection) {
        return logger::LogScopeOwner(logger::LogOrigin::Framework,
                                     logger::LogBoundary::Connection,
                                     "web.http.server",
                                     connection.getInstanceName().empty() ? std::nullopt
                                                                          : std::optional<std::string>(connection.getInstanceName()),
                                     logger::LogRole::Server,
                                     std::to_string(connection.getConnectionId()))
            .logger(logger::Logger::semanticSink());
    }

    inline logger::BoundaryLogger httpServerLog(logger::BoundaryLogger::Sink sink,
                                                logger::LogLevel threshold = logger::LogLevel::Trace,
                                                logger::BoundaryLogger::Clock clock = {}) {
        return httpServerLogScope().logger(std::move(sink), threshold, std::move(clock));
    }

    inline logger::BoundaryLogger httpServerUpgradeLog() {
        return httpServerUpgradeLogScope().logger(logger::Logger::semanticSink());
    }

    inline logger::BoundaryLogger httpServerUpgradeLog(logger::BoundaryLogger::Sink sink,
                                                       logger::LogLevel threshold = logger::LogLevel::Trace,
                                                       logger::BoundaryLogger::Clock clock = {}) {
        return httpServerUpgradeLogScope().logger(std::move(sink), threshold, std::move(clock));
    }

} // namespace web::http::server::semantic

#endif // WEB_HTTP_SERVER_SEMANTICLOG_H
