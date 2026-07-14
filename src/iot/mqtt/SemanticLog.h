#ifndef IOT_MQTT_SEMANTICLOG_H
#define IOT_MQTT_SEMANTICLOG_H

#include "core/socket/stream/SocketConnection.h"
#include "log/LogScopeOwner.h"
#include "log/Logger.h"

#include <optional>
#include <string>
#include <utility>

namespace iot::mqtt::semantic {

    inline const logger::LogScopeOwner& mqttLogScope() {
        static const logger::LogScopeOwner scope(logger::LogOrigin::Framework, logger::LogBoundary::Connection, "iot.mqtt");
        return scope;
    }

    inline const logger::LogScopeOwner& mqttClientLogScope() {
        static const logger::LogScopeOwner scope(logger::LogOrigin::Framework, logger::LogBoundary::Connection, "iot.mqtt.client");
        return scope;
    }

    inline const logger::LogScopeOwner& mqttServerLogScope() {
        static const logger::LogScopeOwner scope(logger::LogOrigin::Framework, logger::LogBoundary::Connection, "iot.mqtt.server");
        return scope;
    }

    inline const logger::LogScopeOwner& mqttBrokerLogScope() {
        static const logger::LogScopeOwner scope(logger::LogOrigin::Framework, logger::LogBoundary::Connection, "iot.mqtt.broker");
        return scope;
    }

    inline const logger::LogScopeOwner& mqttSessionLogScope() {
        static const logger::LogScopeOwner scope(logger::LogOrigin::Framework, logger::LogBoundary::Connection, "iot.mqtt.session");
        return scope;
    }

    inline const logger::LogScopeOwner& mqttPacketLogScope() {
        static const logger::LogScopeOwner scope(logger::LogOrigin::Framework, logger::LogBoundary::Connection, "iot.mqtt.packet");
        return scope;
    }

    inline const logger::LogScopeOwner& mqttTopicLogScope() {
        static const logger::LogScopeOwner scope(logger::LogOrigin::Framework, logger::LogBoundary::Connection, "iot.mqtt.topic");
        return scope;
    }

    inline const logger::LogScopeOwner& mqttWebSocketLogScope() {
        static const logger::LogScopeOwner scope(logger::LogOrigin::Framework, logger::LogBoundary::Connection, "iot.mqtt.websocket");
        return scope;
    }

    inline logger::LogScopeOwner mqttWebSocketLogScope(const core::socket::stream::SocketConnection& connection) {
        return logger::LogScopeOwner(logger::LogOrigin::Framework,
                                     logger::LogBoundary::Connection,
                                     "iot.mqtt.websocket",
                                     connection.getInstanceName().empty() ? std::nullopt
                                                                          : std::optional<std::string>(connection.getInstanceName()),
                                     std::nullopt,
                                     std::to_string(connection.getConnectionId()));
    }

#define IOT_MQTT_SEMANTIC_HELPER(name)                                                                                                     \
    inline logger::BoundaryLogger name() {                                                                                                 \
        return name##Scope().logger(logger::Logger::semanticSink());                                                                       \
    }                                                                                                                                      \
    inline logger::BoundaryLogger name(logger::BoundaryLogger::Sink sink,                                                                  \
                                       logger::LogLevel threshold = logger::LogLevel::Trace,                                               \
                                       logger::BoundaryLogger::Clock clock = {}) {                                                         \
        return name##Scope().logger(std::move(sink), threshold, std::move(clock));                                                         \
    }

    IOT_MQTT_SEMANTIC_HELPER(mqttLog)
    IOT_MQTT_SEMANTIC_HELPER(mqttClientLog)
    IOT_MQTT_SEMANTIC_HELPER(mqttServerLog)
    IOT_MQTT_SEMANTIC_HELPER(mqttBrokerLog)
    IOT_MQTT_SEMANTIC_HELPER(mqttSessionLog)
    IOT_MQTT_SEMANTIC_HELPER(mqttPacketLog)
    IOT_MQTT_SEMANTIC_HELPER(mqttTopicLog)
    IOT_MQTT_SEMANTIC_HELPER(mqttWebSocketLog)

    inline logger::BoundaryLogger mqttWebSocketLog(const core::socket::stream::SocketConnection& connection) {
        return mqttWebSocketLogScope(connection).logger(logger::Logger::semanticSink());
    }

    inline logger::BoundaryLogger mqttWebSocketLog(const core::socket::stream::SocketConnection& connection,
                                                   logger::BoundaryLogger::Sink sink,
                                                   logger::LogLevel threshold = logger::LogLevel::Trace,
                                                   logger::BoundaryLogger::Clock clock = {}) {
        return mqttWebSocketLogScope(connection).logger(std::move(sink), threshold, std::move(clock));
    }

#undef IOT_MQTT_SEMANTIC_HELPER

} // namespace iot::mqtt::semantic

#endif // IOT_MQTT_SEMANTICLOG_H
