#ifndef WEB_WEBSOCKET_SEMANTICLOG_H
#define WEB_WEBSOCKET_SEMANTICLOG_H

#include "core/socket/stream/SocketConnection.h"
#include "log/LogScopeOwner.h"
#include "log/Logger.h"

#include <optional>
#include <string>
#include <utility>

namespace web::websocket::semantic {

    inline const logger::LogScopeOwner& webSocketLogScope() {
        static const logger::LogScopeOwner scope(logger::LogOrigin::Framework, logger::LogBoundary::Connection, "web.websocket");
        return scope;
    }

    inline const logger::LogScopeOwner& webSocketFrameLogScope() {
        static const logger::LogScopeOwner scope(logger::LogOrigin::Framework, logger::LogBoundary::Connection, "web.websocket.frame");
        return scope;
    }

    inline const logger::LogScopeOwner& webSocketSubProtocolLogScope() {
        static const logger::LogScopeOwner scope(
            logger::LogOrigin::Framework, logger::LogBoundary::Connection, "web.websocket.subprotocol");
        return scope;
    }

    inline logger::LogScopeOwner webSocketSubProtocolLogScope(const core::socket::stream::SocketConnection& connection) {
        return logger::LogScopeOwner(logger::LogOrigin::Framework,
                                     logger::LogBoundary::Connection,
                                     "web.websocket.subprotocol",
                                     connection.getInstanceName().empty() ? std::nullopt
                                                                          : std::optional<std::string>(connection.getInstanceName()),
                                     std::nullopt,
                                     connection.getConnectionName());
    }

    inline const logger::LogScopeOwner& webSocketFactoryLogScope() {
        static const logger::LogScopeOwner scope(logger::LogOrigin::Framework, logger::LogBoundary::Connection, "web.websocket.factory");
        return scope;
    }

    inline logger::BoundaryLogger webSocketLog() {
        return webSocketLogScope().logger(logger::Logger::semanticSink());
    }

    inline logger::BoundaryLogger webSocketLog(logger::BoundaryLogger::Sink sink,
                                               logger::LogLevel threshold = logger::LogLevel::Trace,
                                               logger::BoundaryLogger::Clock clock = {}) {
        return webSocketLogScope().logger(std::move(sink), threshold, std::move(clock));
    }

    inline logger::BoundaryLogger webSocketFrameLog() {
        return webSocketFrameLogScope().logger(logger::Logger::semanticSink());
    }

    inline logger::BoundaryLogger webSocketFrameLog(logger::BoundaryLogger::Sink sink,
                                                    logger::LogLevel threshold = logger::LogLevel::Trace,
                                                    logger::BoundaryLogger::Clock clock = {}) {
        return webSocketFrameLogScope().logger(std::move(sink), threshold, std::move(clock));
    }

    inline logger::BoundaryLogger webSocketSubProtocolLog() {
        return webSocketSubProtocolLogScope().logger(logger::Logger::semanticSink());
    }

    inline logger::BoundaryLogger webSocketSubProtocolLog(logger::BoundaryLogger::Sink sink,
                                                          logger::LogLevel threshold = logger::LogLevel::Trace,
                                                          logger::BoundaryLogger::Clock clock = {}) {
        return webSocketSubProtocolLogScope().logger(std::move(sink), threshold, std::move(clock));
    }

    inline logger::BoundaryLogger webSocketSubProtocolLog(const core::socket::stream::SocketConnection& connection) {
        return webSocketSubProtocolLogScope(connection).logger(logger::Logger::semanticSink());
    }

    inline logger::BoundaryLogger webSocketSubProtocolLog(const core::socket::stream::SocketConnection& connection,
                                                          logger::BoundaryLogger::Sink sink,
                                                          logger::LogLevel threshold = logger::LogLevel::Trace,
                                                          logger::BoundaryLogger::Clock clock = {}) {
        return webSocketSubProtocolLogScope(connection).logger(std::move(sink), threshold, std::move(clock));
    }

    inline logger::BoundaryLogger webSocketFactoryLog() {
        return webSocketFactoryLogScope().logger(logger::Logger::semanticSink());
    }

    inline logger::BoundaryLogger webSocketFactoryLog(logger::BoundaryLogger::Sink sink,
                                                      logger::LogLevel threshold = logger::LogLevel::Trace,
                                                      logger::BoundaryLogger::Clock clock = {}) {
        return webSocketFactoryLogScope().logger(std::move(sink), threshold, std::move(clock));
    }

} // namespace web::websocket::semantic

#endif // WEB_WEBSOCKET_SEMANTICLOG_H
