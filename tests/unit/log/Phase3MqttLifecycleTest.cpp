#include "core/socket/SocketAddress.h"
#include "core/socket/stream/SocketConnection.h"
#include "iot/mqtt/Mqtt.h"
#include "iot/mqtt/MqttContext.h"
#include "iot/mqtt/packets/Connect.h"
#include "iot/mqtt/server/Mqtt.h"
#include "iot/mqtt/server/broker/Broker.h"
#include "log/Logger.h"
#include "support/Phase3SemanticLogCapture.h"
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
    tests::support::Phase3SemanticLogCapture capture("snodec-phase3-mqtt-lifecycle");
    logger::Logger::setLogLevel(6);
    logger::LogManager::setGlobalLevel(logger::LogLevel::Debug);
    logger::LogManager::setFormat(logger::LogManager::Format::Json);
    logger::LogManager::freeze();

    const auto broker = std::make_shared<iot::mqtt::server::broker::Broker>(2, "");

    {
        FakeSocketConnection socket(1, "accepted");
        BufferContext context(new iot::mqtt::server::Mqtt("accepted", broker), socket, connectPacket("client-established", true));
        context.onConnected();
        context.onConnected();
        context.process();
        context.onDisconnected();
        context.onDisconnected();
    }

    broker->newSession("client-resumed", nullptr);
    broker->retainSession("client-resumed");
    {
        FakeSocketConnection socket(2, "resumed");
        BufferContext context(new iot::mqtt::server::Mqtt("resumed", broker), socket, connectPacket("client-resumed", false));
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

        FakeSocketConnection socket(3, "rejected");
        BufferContext context(new iot::mqtt::server::Mqtt("rejected", broker), socket, std::move(packet));
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

    return result.processResult();
}
