#include "core/socket/SocketAddress.h"
#include "core/socket/stream/SocketConnection.h"
#include "iot/mqtt/Mqtt.h"
#include "iot/mqtt/MqttContext.h"
#include "iot/mqtt/packets/Connect.h"
#include "iot/mqtt/server/Mqtt.h"
#include "iot/mqtt/server/broker/Broker.h"
#include "log/Logger.h"
#include "support/SemanticLogCapture.h"
#include "support/TestResult.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <nlohmann/json.hpp>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {
    class DummyAddress final : public core::socket::SocketAddress {
    public:
        std::string toString(bool = true) const override {
            return {};
        }
    };

    class FakeSocketConnection final : public core::socket::stream::SocketConnection {
    public:
        FakeSocketConnection(std::uint64_t id, const std::string& name)
            : SocketConnection(-1, id, name, nullptr) {
        }

        ~FakeSocketConnection() override = default;

        int getFd() const override {
            return -1;
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
            return address;
        }

        const core::socket::SocketAddress& getLocalAddress() const override {
            return address;
        }

        const core::socket::SocketAddress& getRemoteAddress() const override {
            return address;
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

    private:
        DummyAddress address;
    };

    class BufferContext final : public iot::mqtt::MqttContext {
    public:
        BufferContext(iot::mqtt::Mqtt* mqtt, FakeSocketConnection& socketConnection, std::vector<char> input)
            : MqttContext(mqtt)
            , socketConnection(socketConnection)
            , input(std::move(input)) {
        }

        std::size_t recv(char* chunk, std::size_t length) override {
            const std::size_t count = std::min(length, input.size() - offset);
            std::copy_n(input.data() + static_cast<std::ptrdiff_t>(offset), count, chunk);
            offset += count;
            return count;
        }

        void send(const char* chunk, std::size_t length) override {
            output.insert(output.end(), chunk, chunk + static_cast<std::ptrdiff_t>(length));
        }

        core::socket::stream::SocketConnection* getSocketConnection() const override {
            return &socketConnection;
        }

        void end() override {
            ended = true;
        }

        void close() override {
            closed = true;
        }

        void process() {
            while (offset < input.size() && onReceivedFromPeer() != 0) {
            }
        }

        bool ended = false;
        bool closed = false;
        std::vector<char> output;

    private:
        FakeSocketConnection& socketConnection;
        std::vector<char> input;
        std::size_t offset = 0;
    };

    std::vector<char> connectPacket(const std::string& clientId, bool cleanSession) {
        return iot::mqtt::packets::Connect(clientId, 0, cleanSession, "", "", 0, false, "", "", false).serialize();
    }

    int countMessage(const std::vector<nlohmann::json>& records, std::string_view message) {
        int count = 0;
        for (const nlohmann::json& record : records) {
            if (record.value("message", "") == message) {
                ++count;
            }
        }
        return count;
    }

    const nlohmann::json*
    findRecord(const std::vector<nlohmann::json>& records, std::string_view component, std::string_view messagePrefix) {
        for (const nlohmann::json& record : records) {
            if (record.value("component", "") == component && record.value("message", "").starts_with(messagePrefix)) {
                return &record;
            }
        }
        return nullptr;
    }

    bool matchingLevel(const std::vector<nlohmann::json>& records, std::string_view prefix, std::string_view level) {
        bool found = false;
        for (const nlohmann::json& record : records) {
            const std::string message = record.value("message", "");
            if (message.starts_with(prefix)) {
                found = true;
                if (record.value("level", "") != level || record.value("origin", "") != "framework" ||
                    record.value("boundary", "") != "connection" || record.value("component", "") != "iot.mqtt") {
                    return false;
                }
            }
        }
        return found;
    }
} // namespace

int main() {
    tests::support::TestResult result;
    tests::support::SemanticLogCapture capture("snodec-mqtt-lifecycle");
    logger::Logger::setLogLevel(6);
    logger::LogManager::setGlobalLevel(logger::LogLevel::Debug);
    logger::LogManager::setFormat(logger::LogManager::Format::Json);
    logger::LogManager::freeze();

    const auto broker = std::make_shared<iot::mqtt::server::broker::Broker>(2, "");

    {
        FakeSocketConnection socket(1, "socket-accepted");
        BufferContext context(new iot::mqtt::server::Mqtt("display-accepted", broker), socket, connectPacket("client-established", true));
        context.onConnected();
        context.onConnected();
        context.process();
        context.onDisconnected();
        context.onDisconnected();
    }

    broker->newSession("client-resumed", nullptr);
    broker->retainSession("client-resumed");
    {
        FakeSocketConnection socket(2, "socket-resumed");
        BufferContext context(new iot::mqtt::server::Mqtt("display-resumed", broker), socket, connectPacket("client-resumed", false));
        context.onConnected();
        context.process();
        context.onDisconnected();
    }

    {
        std::vector<char> packet = connectPacket("client-rejected", true);
        constexpr std::array<char, 4> protocol{'M', 'Q', 'T', 'T'};
        const auto protocolBegin = std::search(packet.begin(), packet.end(), protocol.begin(), protocol.end());
        result.expectTrue(protocolBegin != packet.end(), "serialized MQTT CONNECT contains the protocol name");
        if (protocolBegin != packet.end()) {
            *protocolBegin = 'X';
        }

        FakeSocketConnection socket(3, "socket-rejected");
        BufferContext context(new iot::mqtt::server::Mqtt("display-rejected", broker), socket, std::move(packet));
        context.onConnected();
        context.onConnected();
        context.process();
        result.expectTrue(context.closed, "invalid MQTT CONNECT reaches the existing session-rejection transition");
        context.onDisconnected();
        context.onDisconnected();
    }

    const std::vector<nlohmann::json> records = capture.finish();
    result.expectTrue(countMessage(records, "mqtt session established: client=client-established") == 1,
                      "accepted MQTT session is established exactly once");
    result.expectTrue(countMessage(records, "mqtt session resumed: client=client-resumed") == 1,
                      "persistent MQTT session resume is distinct from establishment");
    result.expectTrue(countMessage(records, "mqtt session rejected: client=client-rejected") == 1,
                      "rejected MQTT session has one rejection terminal");
    result.expectTrue(countMessage(records, "mqtt session established: client=client-rejected") == 0,
                      "rejected MQTT session cannot later establish");
    result.expectTrue(countMessage(records, "mqtt session ended: client=client-rejected") == 0,
                      "rejected MQTT session has no fabricated end");
    result.expectTrue(countMessage(records, "mqtt session ended: client=client-established") == 1,
                      "established MQTT session ends exactly once");
    result.expectTrue(countMessage(records, "mqtt session ended: client=client-resumed") == 1, "resumed MQTT session ends exactly once");
    result.expectTrue(countMessage(records, "mqtt protocol started") == 3, "each MQTT protocol owner starts once");
    result.expectTrue(countMessage(records, "mqtt protocol stopped") == 3, "each MQTT protocol owner stops once");
    result.expectTrue(matchingLevel(records, "mqtt protocol ", "info"), "MQTT protocol lifecycle uses Info");
    result.expectTrue(matchingLevel(records, "mqtt session established", "info"), "MQTT session establishment uses Info");
    result.expectTrue(matchingLevel(records, "mqtt session resumed", "info"), "MQTT session resume uses Info");
    result.expectTrue(matchingLevel(records, "mqtt session ended", "info"), "MQTT session end uses Info");
    result.expectTrue(matchingLevel(records, "mqtt session rejected", "debug"), "MQTT session rejection uses Debug");

    const nlohmann::json* commonDiagnostic = findRecord(records, "iot.mqtt", "Fixed Header: PacketType:");
    result.expectTrue(commonDiagnostic != nullptr, "common MQTT diagnostic is captured after connection binding");
    if (commonDiagnostic != nullptr) {
        const std::string message = commonDiagnostic->value("message", "");
        result.expectTrue(commonDiagnostic->value("origin", "") == "framework", "common MQTT diagnostic uses framework origin");
        result.expectTrue(commonDiagnostic->value("boundary", "") == "connection", "common MQTT diagnostic uses connection boundary");
        result.expectTrue(commonDiagnostic->value("instance", "") == "socket-accepted", "common MQTT diagnostic carries socket instance");
        result.expectTrue(commonDiagnostic->value("connection", "") == "1", "common MQTT diagnostic carries connection ID");
        result.expectTrue(!message.starts_with("display-accepted"), "common MQTT message does not repeat display connection identity");
        result.expectTrue(message.find("MQTT:") == std::string::npos, "common MQTT message does not repeat its component label");
        result.expectTrue(message.find("CONNECT") != std::string::npos, "common MQTT message retains packet-type domain data");

        const std::string component = commonDiagnostic->value("component", "");
        const std::string instance = commonDiagnostic->value("instance", "");
        const std::string connection = commonDiagnostic->value("connection", "");
        const logger::LogRecord textRecord = logger::materialize(
            {logger::LogOrigin::Framework, logger::LogBoundary::Connection, component, instance, logger::LogRole::Unknown, connection},
            logger::LogLevel::Debug,
            message);
        const std::string text = logger::formatText(textRecord, false);
        const std::string_view formatterSeparator = " — ";
        const std::size_t separator = text.find(formatterSeparator);
        result.expectTrue(separator != std::string::npos, "text MQTT output contains the formatter separator");
        if (separator != std::string::npos) {
            const std::string metadata = text.substr(0, separator);
            const std::string payload = text.substr(separator + formatterSeparator.size());
            result.expectTrue(metadata.find("iot.mqtt") != std::string::npos &&
                                  metadata.find("inst=socket-accepted") != std::string::npos &&
                                  metadata.find("conn=1") != std::string::npos,
                              "text MQTT output renders structured architectural identity before the payload separator");
            result.expectTrue(payload.starts_with("Fixed Header: PacketType:"),
                              "text MQTT payload starts with the diagnostic rather than architectural identity");
            result.expectTrue(!payload.starts_with("display-accepted") && payload.find("MQTT:") == std::string::npos,
                              "text MQTT payload does not repeat connection or component identity");
        }
    }

    const nlohmann::json* serverDiagnostic = findRecord(records, "iot.mqtt.server", "ClientID: client-established");
    result.expectTrue(serverDiagnostic != nullptr, "role-specific server diagnostic is captured after connection binding");
    if (serverDiagnostic != nullptr) {
        const std::string message = serverDiagnostic->value("message", "");
        result.expectTrue(serverDiagnostic->value("origin", "") == "framework", "server diagnostic uses framework origin");
        result.expectTrue(serverDiagnostic->value("boundary", "") == "connection", "server diagnostic uses connection boundary");
        result.expectTrue(serverDiagnostic->value("component", "") == "iot.mqtt.server", "server diagnostic uses server component");
        result.expectTrue(serverDiagnostic->value("role", "") == "server", "server diagnostic carries server role");
        result.expectTrue(serverDiagnostic->value("instance", "") == "socket-accepted", "server diagnostic carries socket instance");
        result.expectTrue(serverDiagnostic->value("connection", "") == "1", "server diagnostic carries connection ID");
        result.expectTrue(!message.starts_with("display-accepted") && message.find("MQTT Broker:") == std::string::npos,
                          "server message does not repeat connection or component identity");
        result.expectTrue(message.find("client-established") != std::string::npos, "server message retains MQTT client ID domain data");
    }

    return result.processResult();
}
