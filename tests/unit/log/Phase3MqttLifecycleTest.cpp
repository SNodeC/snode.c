#include "iot/mqtt/Mqtt.h"
#include "log/Logger.h"
#include "support/Phase3SemanticLogCapture.h"
#include "support/TestResult.h"

#include <nlohmann/json.hpp>
#include <string>
#include <string_view>
#include <vector>

namespace {
    class TestMqtt final : public iot::mqtt::Mqtt {
    public:
        TestMqtt(const std::string& connectionName, const std::string& clientId)
            : Mqtt(connectionName, clientId) {
        }

        void establish(bool resumed) {
            sessionEstablished(resumed);
        }

        void reject(const std::string& rejectedClientId) {
            sessionRejected(rejectedClientId);
        }

        bool onSignal(int) override {
            return true;
        }

    private:
        iot::mqtt::ControlPacketDeserializer* createControlPacketDeserializer(iot::mqtt::FixedHeader&) const override {
            return nullptr;
        }

        void deliverPacket(iot::mqtt::ControlPacketDeserializer*) override {
        }

        void distributePublish(const iot::mqtt::packets::Publish&) override {
        }
    };

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

    {
        TestMqtt established("accepted", "client-established");
        established.onConnected();
        established.onConnected();
        established.establish(false);
        established.establish(false);
        established.onDisconnected();
        established.onDisconnected();

        TestMqtt resumed("resumed", "client-resumed");
        resumed.onConnected();
        resumed.establish(true);
        resumed.onDisconnected();

        TestMqtt rejected("rejected", "client-rejected");
        rejected.onConnected();
        rejected.reject("client-rejected");
        rejected.reject("client-rejected");
        rejected.establish(false);
        rejected.onDisconnected();
        rejected.onDisconnected();
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
