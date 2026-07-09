#include "iot/mqtt/SemanticLog.h"
#include "log/Logger.h"
#include "log/SemanticLogger.h"
#include "tests/support/TestResult.h"

#include <filesystem>
#include <fstream>
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

    std::string readFile(const std::filesystem::path& path) {
        std::ifstream input(path);
        return std::string((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
    }

    bool containsRepeatedHelperGuard(const std::string& source) {
        return source.find("semantic::mqttBrokerLog().enabled") != std::string::npos ||
               source.find("semantic::mqttSessionLog().enabled") != std::string::npos;
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

    {
        const std::filesystem::path root = PROJECT_SOURCE_DIR;
        const std::string retainTree = readFile(root / "src/iot/mqtt/server/broker/RetainTree.cpp");
        const std::string subscriptionTree = readFile(root / "src/iot/mqtt/server/broker/SubscriptionTree.cpp");
        const std::string session = readFile(root / "src/iot/mqtt/server/broker/Session.cpp");

        result.expectTrue(!containsRepeatedHelperGuard(retainTree), "RetainTree broker diagnostics use local bind-once guards");
        result.expectTrue(!containsRepeatedHelperGuard(subscriptionTree), "SubscriptionTree broker diagnostics use local bind-once guards");
        result.expectTrue(!containsRepeatedHelperGuard(session), "Session broker diagnostics use local bind-once guards");
    }

    return result.processResult();
}
