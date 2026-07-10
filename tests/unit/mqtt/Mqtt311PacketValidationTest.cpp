#include "iot/mqtt/MqttContext.h"
#include "iot/mqtt/Mqtt.h"
#include "iot/mqtt/ControlPacketDeserializer.h"
#include "iot/mqtt/packets/Publish.h"
#include "iot/mqtt/FixedHeader.h"
#include "iot/mqtt/server/packets/Connect.h"
#include "iot/mqtt/server/packets/Publish.h"
#include "iot/mqtt/server/packets/Pubrel.h"
#include "iot/mqtt/server/packets/Subscribe.h"
#include "iot/mqtt/server/packets/Unsubscribe.h"
#include "tests/support/TestResult.h"

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

namespace {

class DummyMqtt : public iot::mqtt::Mqtt {
public:
    DummyMqtt() : iot::mqtt::Mqtt("test") {}
    bool onSignal(int) override { return true; }
    iot::mqtt::ControlPacketDeserializer* createControlPacketDeserializer(iot::mqtt::FixedHeader&) const override { return nullptr; }
    void deliverPacket(iot::mqtt::ControlPacketDeserializer*) override {}
    void distributePublish(const iot::mqtt::packets::Publish&) override {}
};

class BufferContext : public iot::mqtt::MqttContext {
public:
    explicit BufferContext(const std::vector<char>& input)
        : iot::mqtt::MqttContext(new DummyMqtt())
        , input(input) {
    }

    std::size_t recv(char* chunk, std::size_t chunklen) override {
        const std::size_t available = input.size() - offset;
        const std::size_t count = std::min(chunklen, available);
        std::copy_n(input.data() + offset, count, chunk);
        offset += count;
        return count;
    }

    void send(const char*, std::size_t) override {}
    core::socket::stream::SocketConnection* getSocketConnection() const override { return nullptr; }
    void end() override {}
    void close() override { closed = true; }

    bool closed = false;

private:
    std::vector<char> input;
    std::size_t offset = 0;
};

void appendString(std::vector<char>& data, const std::string& value) {
    data.push_back(static_cast<char>((value.size() >> 8) & 0xff));
    data.push_back(static_cast<char>(value.size() & 0xff));
    data.insert(data.end(), value.begin(), value.end());
}

std::vector<char> connectVp(uint8_t level, uint8_t flags) {
    std::vector<char> data;
    appendString(data, "MQTT");
    data.push_back(static_cast<char>(level));
    data.push_back(static_cast<char>(flags));
    data.push_back(0);
    data.push_back(60);
    appendString(data, "client");
    return data;
}

std::vector<char> subscribeVp(std::initializer_list<std::pair<std::string, uint8_t>> topics) {
    std::vector<char> data{0, 1};
    for (const auto& [topic, qos] : topics) {
        appendString(data, topic);
        data.push_back(static_cast<char>(qos));
    }
    return data;
}

std::vector<char> publishVp(const std::string& topic, const std::string& payload, bool includePacketId = false, uint16_t pid = 1) {
    std::vector<char> data;
    appendString(data, topic);
    if (includePacketId) {
        data.push_back(static_cast<char>((pid >> 8) & 0xff));
        data.push_back(static_cast<char>(pid & 0xff));
    }
    data.insert(data.end(), payload.begin(), payload.end());
    return data;
}

template <typename Packet>
bool deserializePacket(Packet& packet, const std::vector<char>& data) {
    BufferContext context(data);
    while (!packet.isComplete() && !packet.isError()) {
        if (packet.deserialize(&context) == 0) {
            break;
        }
    }
    return packet.isComplete() && !packet.isError();
}

} // namespace

int main() {
    tests::support::TestResult result;

    {
        iot::mqtt::packets::Connect normal("client", 60, true, "", "", 0, false, "", "", false);
        const auto wire = normal.serialize();
        result.expectEqual(0x04, static_cast<uint8_t>(wire[8]), "client CONNECT without loop prevention sends level 0x04");
    }
    {
        iot::mqtt::packets::Connect bridge("client", 60, true, "", "", 0, false, "", "", true);
        const auto wire = bridge.serialize();
        result.expectEqual(0x84, static_cast<uint8_t>(wire[8]), "client CONNECT with loop prevention sends private bridge level 0x84");
    }

    {
        const auto data = connectVp(0x04, 0x02);
        iot::mqtt::server::packets::Connect connect(data.size(), 0x00);
        result.expectTrue(deserializePacket(connect, data), "normal MQTT 3.1.1 CONNECT parses");
        result.expectEqual(0x04, connect.getLevel(), "normal CONNECT level remains 0x04");
        result.expectTrue(connect.getReflect(), "normal CONNECT reflects publications");
    }
    {
        const auto data = connectVp(0x84, 0x02);
        iot::mqtt::server::packets::Connect connect(data.size(), 0x00);
        result.expectTrue(deserializePacket(connect, data), "private bridge CONNECT parses");
        result.expectEqual(0x04, connect.getLevel(), "private bridge CONNECT masks level to 0x04");
        result.expectTrue(!connect.getReflect(), "private bridge CONNECT disables reflection");
    }
    {
        const auto data = connectVp(0x84, 0x03);
        iot::mqtt::server::packets::Connect connect(data.size(), 0x00);
        result.expectTrue(deserializePacket(connect, data), "CONNECT parser preserves reserved flag for server validation");
        result.expectEqual(0x03, connect.getConnectFlags(), "CONNECT reserved flag remains visible");
    }

    for (uint8_t qos = 0; qos <= 2; ++qos) {
        const auto data = subscribeVp({{"a/b", qos}});
        iot::mqtt::server::packets::Subscribe subscribe(data.size(), 0x02);
        result.expectTrue(deserializePacket(subscribe, data), "SUBSCRIBE QoS 0/1/2 parses");
    }
    for (uint8_t qos : {uint8_t{0x03}, uint8_t{0x08}, uint8_t{0x0c}, uint8_t{0x80}}) {
        const auto data = subscribeVp({{"a/b", qos}});
        iot::mqtt::server::packets::Subscribe subscribe(data.size(), 0x02);
        result.expectTrue(!deserializePacket(subscribe, data), "SUBSCRIBE invalid QoS/options rejected");
    }
    {
        const auto data = subscribeVp({{"a/b", 0}, {"c/d", 0x80}});
        iot::mqtt::server::packets::Subscribe subscribe(data.size(), 0x02);
        result.expectTrue(!deserializePacket(subscribe, data), "SUBSCRIBE rejects invalid QoS on second topic");
    }
    {
        iot::mqtt::server::packets::Subscribe subscribe(5, 0x00);
        result.expectTrue(subscribe.isError(), "SUBSCRIBE fixed-header flags other than 0x02 rejected");
        iot::mqtt::server::packets::Unsubscribe unsubscribe(5, 0x00);
        result.expectTrue(unsubscribe.isError(), "UNSUBSCRIBE fixed-header flags other than 0x02 rejected");
        iot::mqtt::server::packets::Pubrel pubrel(2, 0x00);
        result.expectTrue(pubrel.isError(), "PUBREL fixed-header flags other than 0x02 rejected");
    }

    {
        const auto data = publishVp("a/b", "payload");
        iot::mqtt::server::packets::Publish publish(data.size(), 0x00);
        result.expectTrue(deserializePacket(publish, data), "PUBLISH QoS0 DUP0 parses");
        result.expectTrue(!publish.getRetain(), "PUBLISH retain false preserved");
    }
    {
        const auto data = publishVp("a/b", "payload");
        iot::mqtt::server::packets::Publish publish(data.size(), 0x01);
        result.expectTrue(deserializePacket(publish, data), "PUBLISH retain true parses");
        result.expectTrue(publish.getRetain(), "PUBLISH retain true preserved");
    }
    {
        const auto data = publishVp("a/b", "payload");
        iot::mqtt::server::packets::Publish publish(data.size(), 0x08);
        result.expectTrue(publish.isError(), "PUBLISH QoS0 DUP1 rejected at fixed header");
    }
    {
        const auto data = publishVp("a/b", "payload", true, 1);
        iot::mqtt::server::packets::Publish publish(data.size(), 0x06);
        result.expectTrue(publish.isError(), "PUBLISH QoS3 rejected at fixed header");
    }
    {
        const auto data = publishVp("a/b", "payload", true, 0);
        iot::mqtt::server::packets::Publish publish(data.size(), 0x02);
        result.expectTrue(!deserializePacket(publish, data), "PUBLISH QoS1 packet id zero rejected");
    }
    {
        const auto data = publishVp("", "payload");
        iot::mqtt::server::packets::Publish publish(data.size(), 0x00);
        result.expectTrue(!deserializePacket(publish, data), "PUBLISH empty topic rejected");
    }
    {
        const auto data = publishVp("a/b", std::string(70000, 'x'));
        iot::mqtt::server::packets::Publish publish(data.size(), 0x00);
        result.expectTrue(deserializePacket(publish, data), "PUBLISH payload larger than 65535 is not truncated");
        result.expectEqual(70000, static_cast<int>(publish.getMessage().size()), "large PUBLISH payload size preserved");
    }

    {
        BufferContext context({char(0x30), char(0xff), char(0xff), char(0xff), char(0xff), char(0x7f)});
        iot::mqtt::FixedHeader fixedHeader;
        fixedHeader.deserialize(&context);
        result.expectTrue(fixedHeader.isError(), "Remaining Length overlong sequence rejected");
    }
    {
        BufferContext context({char(0x30), char(0xff), char(0xff), char(0xff), char(0xff)});
        iot::mqtt::FixedHeader fixedHeader;
        fixedHeader.deserialize(&context);
        result.expectTrue(fixedHeader.isError(), "Remaining Length continuation on fourth byte rejected");
    }

    return result.processResult();
}
