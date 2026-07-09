#include "iot/mqtt/SemanticLog.h"
#include "log/Logger.h"
#include "log/SemanticLogger.h"
#include "tests/support/TestResult.h"
#include "tests/unit/log/SourcePolicyTestRoot.h"

#include <filesystem>
#include <ostream>
#include <string>
#include <string_view>

namespace {
    struct ExpensiveArgument {
        int* counter;
    };

    std::ostream& operator<<(std::ostream& os, const ExpensiveArgument& value) {
        ++*value.counter;
        return os << "expensive";
    }

    class SemanticStateGuard {
    public:
        SemanticStateGuard() {
            logger::Logger::init();
            logger::LogManager::init();
            logger::Logger::setQuiet(true);
            logger::Logger::setDisableColor(true);
        }

        ~SemanticStateGuard() {
            logger::Logger::init();
            logger::LogManager::init();
        }
    };

    bool containsRepeatedHelperGuard(const std::string& source) {
        return source_policy::contains(source, "semantic::mqttBrokerLog().enabled") ||
               source_policy::contains(source, "semantic::mqttSessionLog().enabled");
    }

    bool containsDirectMqttLogSeverity(const std::string& source) {
        constexpr std::string_view severities[] = {"trace", "debug", "info", "warn", "error", "critical"};
        for (const std::string_view severity : severities) {
            const std::string needle = std::string("iot::mqtt::semantic::mqttLog().") + std::string(severity) + "()";
            if (source_policy::contains(source, needle)) {
                return true;
            }
        }
        return false;
    }
} // namespace

int main() {
    tests::support::TestResult result;

    {
        SemanticStateGuard guard;
        logger::LogManager::setGlobalLevel(logger::LogLevel::Info);
        logger::LogManager::freeze();

        const auto cachedLog = iot::mqtt::semantic::mqttLog();
        result.expectTrue(!cachedLog.enabled(logger::LogLevel::Debug), "cached MQTT logger honors global info threshold");

        int counter = 0;
        cachedLog.debug("value={}", ExpensiveArgument{&counter});
        result.expectEqual(0, counter, "disabled cached MQTT logger does not format expensive arguments");
    }

    {
        SemanticStateGuard guard;
        logger::LogManager::setGlobalLevel(logger::LogLevel::Info);
        logger::LogManager::setComponentLevel("iot.mqtt", logger::LogLevel::Debug);
        logger::LogManager::freeze();

        const auto cachedLog = iot::mqtt::semantic::mqttLog();
        result.expectTrue(cachedLog.enabled(logger::LogLevel::Debug), "cached MQTT logger honors component debug override");

        int counter = 0;
        cachedLog.debug("value={}", ExpensiveArgument{&counter});
        result.expectEqual(1, counter, "enabled cached MQTT logger formats expensive arguments exactly once");
    }

    {
        SemanticStateGuard guard;
        logger::LogManager::setGlobalLevel(logger::LogLevel::Warn);
        logger::LogManager::freeze();

        const auto cachedLog = iot::mqtt::semantic::mqttLog();
        const auto copiedLog = cachedLog;
        result.expectTrue(!copiedLog.enabled(logger::LogLevel::Info),
                          "copied cached logger keeps resolved threshold without re-resolution");
    }

    const std::filesystem::path root = source_policy::sourcePolicyProjectRoot();
    if (root.empty()) {
        result.expectTrue(false, "sourcePolicyProjectRoot() must not return an empty path");
        return result.processResult();
    }

    {
        const std::string mqttHeader = source_policy::readSourcePolicyFile(root / "src/iot/mqtt/Mqtt.h");
        const std::string mqttSource = source_policy::readSourcePolicyFile(root / "src/iot/mqtt/Mqtt.cpp");

        result.expectTrue(source_policy::contains(mqttHeader, "logger::BoundaryLogger log_"),
                          "Mqtt.h contains cached BoundaryLogger member log_");
        result.expectTrue(source_policy::contains(mqttSource, "log_(iot::mqtt::semantic::mqttLog())"),
                          "Mqtt.cpp constructors initialize cached MQTT logger from mqttLog()");
        result.expectTrue(!containsDirectMqttLogSeverity(mqttSource),
                          "Mqtt.cpp uses cached log_ instead of direct same-scope mqttLog severity calls");
        result.expectTrue(source_policy::contains(mqttSource, "log_.debug()"), "Mqtt.cpp contains cached debug logger usage");
        result.expectTrue(source_policy::contains(mqttSource, "log_.info()"), "Mqtt.cpp contains cached info logger usage");
        result.expectTrue(source_policy::contains(mqttSource, "log_.warn()"), "Mqtt.cpp contains cached warn logger usage");
        result.expectTrue(source_policy::contains(mqttSource, "log_.error()"), "Mqtt.cpp contains cached error logger usage");
        result.expectTrue(source_policy::contains(mqttSource, "log_.trace()"), "Mqtt.cpp contains cached trace logger usage");
    }

    {
        const std::string retainTree = source_policy::readSourcePolicyFile(root / "src/iot/mqtt/server/broker/RetainTree.cpp");
        const std::string subscriptionTree = source_policy::readSourcePolicyFile(root / "src/iot/mqtt/server/broker/SubscriptionTree.cpp");
        const std::string session = source_policy::readSourcePolicyFile(root / "src/iot/mqtt/server/broker/Session.cpp");

        result.expectTrue(!containsRepeatedHelperGuard(retainTree), "RetainTree broker diagnostics use local bind-once guards");
        result.expectTrue(!containsRepeatedHelperGuard(subscriptionTree), "SubscriptionTree broker diagnostics use local bind-once guards");
        result.expectTrue(!containsRepeatedHelperGuard(session), "Session broker diagnostics use local bind-once guards");
    }

    return result.processResult();
}
