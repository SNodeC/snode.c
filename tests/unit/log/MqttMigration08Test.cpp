#include "iot/mqtt/SemanticLog.h"
#include "log/Logger.h"
#include "tests/support/TestResult.h"

#include <cerrno>
#include <filesystem>
#include <fstream>
#include <ostream>
#include <string>
#include <utility>
#include <vector>

namespace {
    struct ExpensiveValue {
        int* evaluations;
    };

    std::ostream& operator<<(std::ostream& out, const ExpensiveValue& value) {
        ++*value.evaluations;
        return out << "expensive";
    }

    class LoggerStateGuard {
    public:
        explicit LoggerStateGuard(const std::string& logFile) {
            logger::Logger::init();
            logger::LogManager::init();
            logger::Logger::setLogLevel(6);
            logger::Logger::setVerboseLevel(0);
            logger::Logger::setQuiet(true);
            logger::Logger::setDisableColor(true);
            logger::Logger::setTickResolver([]() {
                return std::string("MIGRATION08TICK");
            });
            logger::Logger::logToFile(logFile);
        }

        ~LoggerStateGuard() {
            logger::Logger::disableLogToFile();
            logger::Logger::setTickResolver({});
            logger::Logger::init();
            logger::LogManager::init();
            errno = 0;
        }
    };

    std::filesystem::path tempLogPath(const std::string& name) {
        auto path = std::filesystem::temp_directory_path() / name;
        std::error_code error;
        std::filesystem::remove(path, error);
        std::filesystem::remove(path.string() + ".1", error);
        return path;
    }

    std::string readFile(const std::filesystem::path& path) {
        std::ifstream in(path);
        return std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
    }
} // namespace

int main() {
    tests::support::TestResult result;

    const auto enabledPath = tempLogPath("snodec-migration08-enabled.log");
    {
        LoggerStateGuard guard(enabledPath.string());
        logger::LogManager::setGlobalLevel(logger::LogLevel::Trace);
        logger::LogManager::freeze();
        iot::mqtt::semantic::mqttLog().debug("mqtt semantic logger emitted");
    }
    const auto enabledLog = readFile(enabledPath);
    result.expectTrue(enabledLog.find("mqtt semantic logger emitted") != std::string::npos, "MQTT semantic logger emits when enabled");
    result.expectTrue(enabledLog.find("framework/connection iot.mqtt ") != std::string::npos,
                      "records carry framework iot.mqtt component scope");

    const auto managerFilterPath = tempLogPath("snodec-migration08-manager-filter.log");
    {
        LoggerStateGuard guard(managerFilterPath.string());
        logger::LogManager::setGlobalLevel(logger::LogLevel::Error);
        logger::LogManager::freeze();
        iot::mqtt::semantic::mqttClientLog().debug("mqtt client suppressed by manager");
    }
    result.expectTrue(readFile(managerFilterPath).find("suppressed by manager") == std::string::npos,
                      "LogManager filtering suppresses migrated MQTT semantic calls");

    const auto backendFilterPath = tempLogPath("snodec-migration08-backend-filter.log");
    {
        LoggerStateGuard guard(backendFilterPath.string());
        logger::LogManager::setGlobalLevel(logger::LogLevel::Trace);
        logger::LogManager::freeze();
        logger::Logger::setLogLevel(3);
        iot::mqtt::semantic::mqttServerLog().debug("mqtt server suppressed by backend");
    }
    result.expectTrue(readFile(backendFilterPath).find("suppressed by backend") != std::string::npos,
                      "semantic output accepted by LogManager is not suppressed by Logger::setLogLevel");

    const auto jsonPath = tempLogPath("snodec-migration08-json.log");
    {
        LoggerStateGuard guard(jsonPath.string());
        logger::LogManager::setGlobalLevel(logger::LogLevel::Debug);
        logger::LogManager::setFormat(logger::LogManager::Format::Json);
        logger::LogManager::freeze();
        iot::mqtt::semantic::mqttPacketLog().debug("mqtt packet json format respected");
    }
    const auto jsonLog = readFile(jsonPath);
    result.expectTrue(jsonLog.find("{\"v\":1") != std::string::npos &&
                          jsonLog.find("\"message\":\"mqtt packet json format respected\"") != std::string::npos &&
                          jsonLog.find("\"origin\":\"framework\"") != std::string::npos &&
                          jsonLog.find("\"component\":\"iot.mqtt.packet\"") != std::string::npos,
                      "JSON format selection is respected");

    std::vector<logger::LogRecord> sysErrorRecords;
    iot::mqtt::semantic::mqttBrokerLog([&](logger::LogRecord record) {
        sysErrorRecords.push_back(std::move(record));
    }).sysError(logger::LogLevel::Error, ENOENT, "MQTT Broker: Could not read session store '{}'", "missing-session.json");
    result.expectTrue(sysErrorRecords.size() == 1 && sysErrorRecords[0].error && sysErrorRecords[0].error->code == ENOENT &&
                          sysErrorRecords[0].error->text.find("No such file") != std::string::npos,
                      "sysError errno code/text is preserved by MQTT helper");

    std::vector<logger::LogRecord> sinkRecords;
    iot::mqtt::semantic::mqttTopicLog([&](logger::LogRecord record) {
        sinkRecords.push_back(std::move(record));
    }).info("MQTT Broker:   Topic filter: '{}' QoS={} ", "sensors/+/temp", 1);
    result.expectTrue(sinkRecords.size() == 1 && sinkRecords[0].component == "iot.mqtt.topic" &&
                          sinkRecords[0].message.find("sensors/+/temp") != std::string::npos,
                      "sink-taking overload remains compatible and topic/filter payload is preserved");

    const auto suppressedPath = tempLogPath("snodec-migration08-suppressed.log");
    {
        LoggerStateGuard guard(suppressedPath.string());
        logger::LogManager::setGlobalLevel(logger::LogLevel::Error);
        logger::LogManager::freeze();
        int evaluations = 0;
        iot::mqtt::semantic::mqttLog().debug("{}", ExpensiveValue{&evaluations});
        result.expectEqual(0, evaluations, "suppressed production formatting does not evaluate ExpensiveValue after PR #151");
    }

    std::vector<logger::LogRecord> records;
    const auto collect = [&](logger::LogRecord record) {
        records.push_back(std::move(record));
    };
    iot::mqtt::semantic::mqttClientLog(collect).info("{} MQTT Client: CONNECT send: {}", "conn08", "client08");
    iot::mqtt::semantic::mqttServerLog(collect).info("{} MQTT Broker:   Protocol: {}", "conn08", "MQTT");
    iot::mqtt::semantic::mqttBrokerLog(collect).info("MQTT Broker: Send PUBLISH: {}", "client08");
    iot::mqtt::semantic::mqttSessionLog(collect).debug("MQTT Broker:   OriginClientId: {}", "origin08");
    iot::mqtt::semantic::mqttPacketLog(collect).debug("{} MQTT: Fixed Header: PacketType: 0x{} ({})", "conn08", "03", "PUBLISH");
    iot::mqtt::semantic::mqttWebSocketLog(collect).info("{} WsMqtt: connected:", "conn08");

    result.expectTrue(records.size() == 6, "representative MQTT client/server/broker/session/packet/WebSocket payloads emitted");
    result.expectTrue(records[0].message.find("CONNECT send: client08") != std::string::npos,
                      "representative MQTT client lifecycle payload preservation is covered");
    result.expectTrue(records[1].message.find("Protocol: MQTT") != std::string::npos,
                      "representative MQTT server lifecycle payload preservation is covered");
    result.expectTrue(records[2].message.find("Send PUBLISH: client08") != std::string::npos,
                      "representative MQTT broker payload preservation is covered");
    result.expectTrue(records[3].message.find("OriginClientId: origin08") != std::string::npos,
                      "representative MQTT session payload preservation is covered");
    result.expectTrue(records[4].message.find("Fixed Header: PacketType: 0x03 (PUBLISH)") != std::string::npos,
                      "representative MQTT packet/control-packet payload preservation is covered");
    result.expectTrue(records[5].message.find("WsMqtt: connected") != std::string::npos,
                      "representative MQTT-over-WebSocket payload preservation is covered");

    bool expensiveDumpBuilt = false;
    const auto packetLog = iot::mqtt::semantic::mqttPacketLog(
        [](logger::LogRecord) {
        },
        logger::LogLevel::Error);
    if (packetLog.enabled(logger::LogLevel::Trace)) {
        expensiveDumpBuilt = true;
        packetLog.trace("expensive dump {}", "payload");
    }
    result.expectTrue(!expensiveDumpBuilt, "expensive payload/dump guard suppresses construction when disabled");

    bool webSocketDumpBuilt = false;
    const auto webSocketLog = iot::mqtt::semantic::mqttWebSocketLog(
        [](logger::LogRecord) {
        },
        logger::LogLevel::Error);
    if (webSocketLog.enabled(logger::LogLevel::Debug)) {
        webSocketDumpBuilt = true;
        webSocketLog.debug("WsMqtt: Frame Data:\n{}", "expensive hex dump");
    }
    result.expectTrue(!webSocketDumpBuilt, "WsMqtt frame dump guard suppresses construction when debug is disabled");

    return result.processResult();
}
