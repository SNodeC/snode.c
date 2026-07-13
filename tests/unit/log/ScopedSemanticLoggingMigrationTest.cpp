#include "SemanticLog.h"
#include "core/socket/stream/SocketConnection.h"
#include "iot/mqtt/SemanticLog.h"
#include "log/Logger.h"
#include "tests/support/TestResult.h"
#include "utils/Timeval.h"
#include "web/websocket/SemanticLog.h"

#include <cstdlib>
#include <string>
#include <utility>
#include <vector>

namespace {
    class FakeSocketConnection : public core::socket::stream::SocketConnection {
    public:
        explicit FakeSocketConnection(int fd, std::string instance)
            : SocketConnection(fd, instance, nullptr) {
        }

        ~FakeSocketConnection() override = default;

        int getFd() const override {
            return 0;
        }

        void sendToPeer(const char*, std::size_t) override {
        }

        bool streamToPeer(core::pipe::Source*) override {
            return false;
        }

        void streamEof() override {
        }

        std::size_t readFromPeer(char*, std::size_t) override {
            return 0;
        }

        void shutdownRead() override {
        }

        void shutdownWrite() override {
        }

        const core::socket::SocketAddress& getBindAddress() const override {
            std::abort();
        }

        const core::socket::SocketAddress& getLocalAddress() const override {
            std::abort();
        }

        const core::socket::SocketAddress& getRemoteAddress() const override {
            std::abort();
        }

        void close() override {
        }

        void setTimeout(const utils::Timeval&) override {
        }

        void setReadTimeout(const utils::Timeval&) override {
        }

        void setWriteTimeout(const utils::Timeval&) override {
        }

        std::size_t getTotalSent() const override {
            return 0;
        }

        std::size_t getTotalQueued() const override {
            return 0;
        }

        std::size_t getTotalRead() const override {
            return 0;
        }

        std::size_t getTotalProcessed() const override {
            return 0;
        }
    };

    bool noAnsi(const std::string& message) {
        return message.find("\033[") == std::string::npos;
    }
} // namespace

int main() {
    tests::support::TestResult result;
    logger::Logger::init();
    logger::LogManager::init();

    FakeSocketConnection first(101, "inst-a");
    FakeSocketConnection second(202, "inst-b");

    std::vector<logger::LogRecord> records;
    const auto collect = [&](logger::LogRecord record) {
        records.push_back(std::move(record));
    };

    constexpr std::size_t webSocketScoped = 0;
    constexpr std::size_t webSocketIndependentScoped = 1;
    constexpr std::size_t mqttScoped = 2;
    constexpr std::size_t mqttMultilineScoped = 3;
    constexpr std::size_t expressDispatcherScoped = 4;
    constexpr std::size_t expressMiddlewareScoped = 5;
    constexpr std::size_t webSocketUnscoped = 6;
    constexpr std::size_t mqttUnscoped = 7;
    constexpr std::size_t expressUnscoped = 8;
    constexpr std::size_t retainedExpressScoped = 9;
    constexpr std::size_t retainedWebSocketScoped = 10;

    web::websocket::semantic::webSocketSubProtocolLog(first, collect).debug() << "Subprotocol 'chat': start";
    web::websocket::semantic::webSocketSubProtocolLog(second, collect).debug()
        << "WebSocketContext: Subprotocol 'mqtt' close confirmed from peer";
    iot::mqtt::semantic::mqttWebSocketLog(first, collect).info() << "WsMqtt: connected:";
    iot::mqtt::semantic::mqttWebSocketLog(first, collect).debug() << "WsMqtt: Frame Data:\n" << std::string(32, ' ').append("payload-hex");
    snode::semantic::expressLog(second, collect).trace() << "Router dispatch";
    snode::semantic::expressLog(second, collect).debug() << "Express VerboseMiddleware: GET / HTTP/1.1";
    web::websocket::semantic::webSocketSubProtocolLog(collect).debug() << "websocket unscoped compatibility";
    iot::mqtt::semantic::mqttWebSocketLog(collect).debug() << "mqtt websocket unscoped compatibility";
    snode::semantic::expressLog(collect).debug() << "express unscoped compatibility";

    std::string retainedExpressConnectionName;
    auto retainedExpressLogger = [&collect, &retainedExpressConnectionName]() {
        FakeSocketConnection temporary(303, "temporary-instance");
        retainedExpressConnectionName = temporary.getConnectionName();
        return snode::semantic::expressLog(temporary, collect);
    }();
    retainedExpressLogger.info() << "after connection destruction";

    std::string retainedWebSocketConnectionName;
    auto retainedWebSocketLogger = [&collect, &retainedWebSocketConnectionName]() {
        FakeSocketConnection temporary(404, "temporary-websocket-instance");
        retainedWebSocketConnectionName = temporary.getConnectionName();
        return web::websocket::semantic::webSocketSubProtocolLog(temporary, collect);
    }();
    retainedWebSocketLogger.info() << "websocket after connection destruction";

    result.expectEqual(
        static_cast<std::size_t>(11), records.size(), "all scoped, unscoped compatibility, and retained log records emitted");

    result.expectTrue(records[webSocketScoped].origin == logger::LogOrigin::Framework &&
                          records[webSocketScoped].boundary == logger::LogBoundary::Connection &&
                          records[webSocketScoped].component == "web.websocket.subprotocol" &&
                          records[webSocketScoped].instance == "inst-a" &&
                          records[webSocketScoped].connection == first.getConnectionName() &&
                          records[webSocketScoped].message == "Subprotocol 'chat': start" &&
                          records[webSocketScoped].message.rfind(first.getConnectionName(), 0) != 0,
                      "WebSocket scoped helper preserves full category and adds active inst/conn before removing text identity");
    result.expectTrue(records[webSocketIndependentScoped].origin == logger::LogOrigin::Framework &&
                          records[webSocketIndependentScoped].boundary == logger::LogBoundary::Connection &&
                          records[webSocketIndependentScoped].component == "web.websocket.subprotocol" &&
                          records[webSocketIndependentScoped].instance == "inst-b" &&
                          records[webSocketIndependentScoped].connection == second.getConnectionName() &&
                          records[webSocketIndependentScoped].message == "WebSocketContext: Subprotocol 'mqtt' close confirmed from peer",
                      "independent WebSocket connections do not cross-contaminate scoped identity");
    result.expectTrue(
        records[mqttScoped].origin == logger::LogOrigin::Framework && records[mqttScoped].boundary == logger::LogBoundary::Connection &&
            records[mqttScoped].component == "iot.mqtt.websocket" && records[mqttScoped].instance == "inst-a" &&
            records[mqttScoped].connection == first.getConnectionName() && records[mqttScoped].message == "WsMqtt: connected:",
        "MQTT-over-WebSocket scoped helper preserves full category and message content without duplicated identity");
    result.expectTrue(records[mqttMultilineScoped].origin == logger::LogOrigin::Framework &&
                          records[mqttMultilineScoped].boundary == logger::LogBoundary::Connection &&
                          records[mqttMultilineScoped].component == "iot.mqtt.websocket" &&
                          records[mqttMultilineScoped].instance == "inst-a" &&
                          records[mqttMultilineScoped].connection == first.getConnectionName() &&
                          records[mqttMultilineScoped].message == "WsMqtt: Frame Data:\n                                payload-hex",
                      "MQTT-over-WebSocket multiline frame-data content is preserved after identity removal");
    result.expectTrue(
        records[expressDispatcherScoped].origin == logger::LogOrigin::Framework &&
            records[expressDispatcherScoped].boundary == logger::LogBoundary::Application &&
            records[expressDispatcherScoped].component == "express" && records[expressDispatcherScoped].instance == "inst-b" &&
            records[expressDispatcherScoped].connection == second.getConnectionName() &&
            records[expressDispatcherScoped].message == "Router dispatch",
        "Express dispatcher scoped helper adds active inst/conn and replaces identity-only message with non-empty operation text");
    result.expectTrue(records[expressMiddlewareScoped].origin == logger::LogOrigin::Framework &&
                          records[expressMiddlewareScoped].boundary == logger::LogBoundary::Application &&
                          records[expressMiddlewareScoped].component == "express" &&
                          records[expressMiddlewareScoped].instance == "inst-b" &&
                          records[expressMiddlewareScoped].connection == second.getConnectionName() &&
                          records[expressMiddlewareScoped].message == "Express VerboseMiddleware: GET / HTTP/1.1" &&
                          records[expressMiddlewareScoped].message.rfind(second.getConnectionName(), 0) != 0,
                      "Express middleware scoped helper preserves full category and exact message without duplicated identity");

    result.expectTrue(records[webSocketUnscoped].origin == logger::LogOrigin::Framework &&
                          records[webSocketUnscoped].boundary == logger::LogBoundary::Connection &&
                          records[webSocketUnscoped].component == "web.websocket.subprotocol" && !records[webSocketUnscoped].instance &&
                          !records[webSocketUnscoped].connection &&
                          records[webSocketUnscoped].message == "websocket unscoped compatibility",
                      "zero-argument WebSocket subprotocol helper remains identity-free and preserves message exactly");
    result.expectTrue(records[mqttUnscoped].origin == logger::LogOrigin::Framework &&
                          records[mqttUnscoped].boundary == logger::LogBoundary::Connection &&
                          records[mqttUnscoped].component == "iot.mqtt.websocket" && !records[mqttUnscoped].instance &&
                          !records[mqttUnscoped].connection && records[mqttUnscoped].message == "mqtt websocket unscoped compatibility",
                      "zero-argument MQTT-over-WebSocket helper remains identity-free and preserves message exactly");
    result.expectTrue(records[expressUnscoped].origin == logger::LogOrigin::Framework &&
                          records[expressUnscoped].boundary == logger::LogBoundary::Application &&
                          records[expressUnscoped].component == "express" && !records[expressUnscoped].instance &&
                          !records[expressUnscoped].connection && records[expressUnscoped].message == "express unscoped compatibility",
                      "zero-argument Express helper remains identity-free and preserves message exactly");

    result.expectTrue(records[retainedExpressScoped].origin == logger::LogOrigin::Framework &&
                          records[retainedExpressScoped].boundary == logger::LogBoundary::Application &&
                          records[retainedExpressScoped].component == "express" &&
                          records[retainedExpressScoped].instance == "temporary-instance" &&
                          records[retainedExpressScoped].connection == retainedExpressConnectionName &&
                          records[retainedExpressScoped].message == "after connection destruction",
                      "Express scoped logger owns copied identity and remains valid after source connection destruction");
    result.expectTrue(records[retainedWebSocketScoped].origin == logger::LogOrigin::Framework &&
                          records[retainedWebSocketScoped].boundary == logger::LogBoundary::Connection &&
                          records[retainedWebSocketScoped].component == "web.websocket.subprotocol" &&
                          records[retainedWebSocketScoped].instance == "temporary-websocket-instance" &&
                          records[retainedWebSocketScoped].connection == retainedWebSocketConnectionName &&
                          records[retainedWebSocketScoped].message == "websocket after connection destruction",
                      "WebSocket scoped logger owns copied identity and remains valid after source connection destruction");

    const std::string json = logger::formatJsonV1(records[webSocketScoped]);
    result.expectTrue(json.find("\"instance\":\"inst-a\"") != std::string::npos &&
                          json.find("\"connection\":\"" + first.getConnectionName() + "\"") != std::string::npos,
                      "existing JSON path contains structured scoped identity");
    result.expectTrue(noAnsi(logger::formatText(records[webSocketScoped])) && noAnsi(json),
                      "scoped semantic logging introduces no ANSI color text");

    return result.processResult();
}
