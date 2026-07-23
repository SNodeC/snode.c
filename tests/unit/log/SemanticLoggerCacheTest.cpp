#include "core/EventLoop.h"
#include "iot/mqtt/SemanticLog.h"
#include "log/LogScopeOwner.h"
#include "log/Logger.h"
#include "tests/support/TestResult.h"

#include <optional>
#include <ostream>
#include <string>

namespace {
    struct ExpensiveArgument {
        int* counter;
    };

    std::ostream& operator<<(std::ostream& output, const ExpensiveArgument& value) {
        ++*value.counter;
        return output << "expensive";
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

    logger::LogScopeOwner scope(std::optional<std::string> instance = std::string("cache-instance")) {
        return {logger::LogOrigin::Framework,
                logger::LogBoundary::Connection,
                "core.socket.stream",
                std::move(instance),
                std::nullopt,
                "cache-connection"};
    }
} // namespace

int main() {
    tests::support::TestResult result;
    SemanticStateGuard guard;

    logger::LogManager::setGlobalLevel(logger::LogLevel::Info);
    logger::LogManager::freeze();
    auto globalLog = scope().logger(logger::Logger::semanticSink());
    result.expectTrue(!globalLog.enabled(logger::LogLevel::Debug), "cached logger honors the resolved global threshold");
    int evaluations = 0;
    globalLog.debug("{}", ExpensiveArgument{&evaluations});
    result.expectEqual(0, evaluations, "cached logger suppresses expensive arguments");

    logger::LogManager::init();
    logger::LogManager::setGlobalLevel(logger::LogLevel::Warn);
    logger::LogManager::setComponentLevel("core.socket.stream", logger::LogLevel::Debug);
    logger::LogManager::freeze();
    auto componentLog = scope(std::nullopt).logger(logger::Logger::semanticSink());
    result.expectTrue(componentLog.enabled(logger::LogLevel::Debug), "cached logger honors a component override");
    componentLog.debug("{}", ExpensiveArgument{&evaluations});
    result.expectEqual(1, evaluations, "enabled cached logger evaluates an expensive argument exactly once");

    logger::LogManager::init();
    logger::LogManager::setGlobalLevel(logger::LogLevel::Warn);
    logger::LogManager::setInstanceLevel("cache-instance", logger::LogLevel::Trace);
    logger::LogManager::freeze();
    auto instanceLog = scope().logger(logger::Logger::semanticSink());
    result.expectTrue(instanceLog.enabled(logger::LogLevel::Trace), "cached logger honors an instance override");
    const auto copiedLog = instanceLog;
    result.expectTrue(copiedLog.enabled(logger::LogLevel::Trace), "copied cached logger remains valid with its resolved threshold");

    logger::LogManager::init();
    (void) core::EventLoop::instance().log();
    logger::LogManager::setGlobalLevel(logger::LogLevel::Info);
    logger::LogManager::freeze();
    result.expectTrue(!core::EventLoop::instance().log().enabled(logger::LogLevel::Debug),
                      "a logger requested before freeze does not retain an invalid permissive threshold");

    logger::LogManager::init();
    logger::LogManager::setGlobalLevel(logger::LogLevel::Debug);
    logger::LogManager::freeze();
    result.expectTrue(core::EventLoop::instance().log().enabled(logger::LogLevel::Debug),
                      "production cache refreshes after a LogManager generation change");

    logger::LogManager::init();
    logger::LogManager::setGlobalLevel(logger::LogLevel::Info);
    logger::LogManager::setComponentLevel("iot.mqtt", logger::LogLevel::Debug);
    logger::LogManager::freeze();
    const auto mqttLog = iot::mqtt::semantic::mqttLog();
    const auto copiedMqttLog = mqttLog;
    int mqttEvaluations = 0;
    copiedMqttLog.debug("{}", ExpensiveArgument{&mqttEvaluations});
    result.expectTrue(copiedMqttLog.enabled(logger::LogLevel::Debug),
                      "representative production logger honors its component override after copying");
    result.expectEqual(1, mqttEvaluations, "representative production logger evaluates enabled arguments exactly once");

    logger::LogManager::init();
    logger::LogManager::setGlobalLevel(logger::LogLevel::Info);
    logger::LogManager::freeze();
    const auto suppressedMqttLog = iot::mqtt::semantic::mqttLog();
    suppressedMqttLog.debug("{}", ExpensiveArgument{&mqttEvaluations});
    result.expectEqual(1, mqttEvaluations, "representative production logger suppresses expensive disabled arguments");

    return result.processResult();
}
