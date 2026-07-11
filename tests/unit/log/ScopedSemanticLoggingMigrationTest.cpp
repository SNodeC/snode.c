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

    web::websocket::semantic::webSocketSubProtocolLog(first, collect).debug() << "Subprotocol 'chat': start";
    web::websocket::semantic::webSocketSubProtocolLog(second, collect).debug()
        << "WebSocketContext: Subprotocol 'mqtt' close confirmed from peer";
    iot::mqtt::semantic::mqttWebSocketLog(first, collect).info() << "WsMqtt: connected:";
    iot::mqtt::semantic::mqttWebSocketLog(first, collect).debug() << "WsMqtt: Frame Data:\n" << std::string(32, ' ').append("payload-hex");
    snode::semantic::expressLog(second, collect).trace() << "Router dispatch";
    snode::semantic::expressLog(second, collect).debug() << "Express VerboseMiddleware: GET / HTTP/1.1";
    web::websocket::semantic::webSocketSubProtocolLog(collect).debug() << "unscoped compatibility";

    result.expectEqual(static_cast<std::size_t>(7), records.size(), "all scoped and compatibility records emitted");

    result.expectTrue(records[0].origin == logger::LogOrigin::Framework && records[0].boundary == logger::LogBoundary::Connection &&
                          records[0].component == "web.websocket.subprotocol" && records[0].instance == "inst-a" &&
                          records[0].connection == first.getConnectionName() && records[0].message == "Subprotocol 'chat': start" &&
                          records[0].message.rfind(first.getConnectionName(), 0) != 0,
                      "WebSocket scoped helper preserves category and adds active inst/conn before removing text identity");
    result.expectTrue(records[1].instance == "inst-b" && records[1].connection == second.getConnectionName() &&
                          records[1].message.find("close confirmed from peer") != std::string::npos,
                      "independent WebSocket connections do not cross-contaminate scoped identity");
    result.expectTrue(records[2].component == "iot.mqtt.websocket" && records[2].instance == "inst-a" &&
                          records[2].connection == first.getConnectionName() && records[2].message == "WsMqtt: connected:",
                      "MQTT-over-WebSocket scoped helper preserves message content without duplicated identity");
    result.expectTrue(records[3].message == "WsMqtt: Frame Data:\n                                payload-hex",
                      "MQTT-over-WebSocket multiline frame-data content is preserved after identity removal");
    result.expectTrue(records[4].boundary == logger::LogBoundary::Application && records[4].component == "express" &&
                          records[4].instance == "inst-b" && records[4].connection == second.getConnectionName() &&
                          records[4].message == "Router dispatch",
                      "Express scoped helper adds active inst/conn and replaces identity-only message with non-empty operation text");
    result.expectTrue(!records[5].message.empty() && records[5].message.rfind(second.getConnectionName(), 0) != 0,
                      "Express middleware messages are non-empty and do not duplicate connection identity");
    result.expectTrue(!records[6].instance && !records[6].connection && records[6].message == "unscoped compatibility",
                      "zero-argument static helpers remain unscoped and do not inherit stale identity");

    const std::string json = logger::formatJsonV1(records[0]);
    result.expectTrue(json.find("\"instance\":\"inst-a\"") != std::string::npos &&
                          json.find("\"connection\":\"" + first.getConnectionName() + "\"") != std::string::npos,
                      "existing JSON path contains structured scoped identity");
    result.expectTrue(noAnsi(logger::formatText(records[0])) && noAnsi(json), "scoped semantic logging introduces no ANSI color text");

    return result.processResult();
}
