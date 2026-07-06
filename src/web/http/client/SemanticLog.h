#ifndef WEB_HTTP_CLIENT_SEMANTICLOG_H
#define WEB_HTTP_CLIENT_SEMANTICLOG_H

#include "log/LogScopeOwner.h"
#include "log/Logger.h"

#include <utility>

namespace web::http::client::semantic {

    inline const logger::LogScopeOwner& httpClientLogScope() {
        static const logger::LogScopeOwner scope(logger::LogOrigin::Framework, logger::LogBoundary::Connection, "web.http.client");
        return scope;
    }

    inline const logger::LogScopeOwner& httpClientUpgradeLogScope() {
        static const logger::LogScopeOwner scope(logger::LogOrigin::Framework, logger::LogBoundary::Connection, "web.http.client.upgrade");
        return scope;
    }

    inline const logger::LogScopeOwner& httpClientEventSourceLogScope() {
        static const logger::LogScopeOwner scope(
            logger::LogOrigin::Framework, logger::LogBoundary::Connection, "web.http.client.eventsource");
        return scope;
    }

    inline logger::BoundaryLogger httpClientLog() {
        return httpClientLogScope().logger(logger::Logger::semanticSink());
    }

    inline logger::BoundaryLogger httpClientLog(logger::BoundaryLogger::Sink sink,
                                                logger::LogLevel threshold = logger::LogLevel::Trace,
                                                logger::BoundaryLogger::Clock clock = {}) {
        return httpClientLogScope().logger(std::move(sink), threshold, std::move(clock));
    }

    inline logger::BoundaryLogger httpClientUpgradeLog() {
        return httpClientUpgradeLogScope().logger(logger::Logger::semanticSink());
    }

    inline logger::BoundaryLogger httpClientUpgradeLog(logger::BoundaryLogger::Sink sink,
                                                       logger::LogLevel threshold = logger::LogLevel::Trace,
                                                       logger::BoundaryLogger::Clock clock = {}) {
        return httpClientUpgradeLogScope().logger(std::move(sink), threshold, std::move(clock));
    }

    inline logger::BoundaryLogger httpClientEventSourceLog() {
        return httpClientEventSourceLogScope().logger(logger::Logger::semanticSink());
    }

    inline logger::BoundaryLogger httpClientEventSourceLog(logger::BoundaryLogger::Sink sink,
                                                           logger::LogLevel threshold = logger::LogLevel::Trace,
                                                           logger::BoundaryLogger::Clock clock = {}) {
        return httpClientEventSourceLogScope().logger(std::move(sink), threshold, std::move(clock));
    }

} // namespace web::http::client::semantic

#endif // WEB_HTTP_CLIENT_SEMANTICLOG_H
