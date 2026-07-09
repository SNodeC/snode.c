#include "SemanticLog.h"
#include "iot/mqtt/SemanticLog.h"
#include "log/Logger.h"
#include "log/SemanticLogger.h"
#include "tests/support/TestResult.h"

#include <chrono>
#include <ostream>
#include <string_view>

namespace {
    struct ExpensiveArgument {
        int* counter;
    };

    std::ostream& operator<<(std::ostream& os, const ExpensiveArgument& value) {
        ++*value.counter;
        return os << "expensive";
    }

    logger::LogScope testScope(std::string_view component = "overhead.test") {
        return {logger::LogOrigin::Framework, logger::LogBoundary::System, component, {}, logger::LogRole::Unknown, {}};
    }

    std::chrono::system_clock::time_point fixedTimestamp() {
        return std::chrono::system_clock::time_point{std::chrono::seconds{1783254896}};
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
} // namespace

int main() {
    tests::support::TestResult result;

    {
        SemanticStateGuard guard;
        int sinkCalls = 0;
        auto logger = logger::BoundaryLogger::createForTest(
            testScope(),
            [&](logger::LogRecord) {
                ++sinkCalls;
            },
            logger::LogLevel::Info,
            fixedTimestamp);

        int counter = 0;
        logger.debug("value={}", ExpensiveArgument{&counter});
        result.expectEqual(0, counter, "disabled fmt-style semantic debug does not format an expensive argument");
        result.expectEqual(0, sinkCalls, "disabled fmt-style semantic debug does not materialize a record or call the sink");
    }

    {
        SemanticStateGuard guard;
        int sinkCalls = 0;
        auto logger = logger::BoundaryLogger::createForTest(
            testScope(),
            [&](logger::LogRecord) {
                ++sinkCalls;
            },
            logger::LogLevel::Debug,
            fixedTimestamp);

        int counter = 0;
        logger.debug("value={}", ExpensiveArgument{&counter});
        result.expectEqual(1, counter, "enabled fmt-style semantic debug formats an expensive argument once");
        result.expectEqual(1, sinkCalls, "enabled fmt-style semantic debug emits one record");
    }

    {
        SemanticStateGuard guard;
        auto logger = logger::BoundaryLogger::createForTest(
            testScope(),
            [](logger::LogRecord) {
            },
            logger::LogLevel::Info,
            fixedTimestamp);

        int counter = 0;
        if (logger.enabled(logger::LogLevel::Debug)) {
            logger.debug() << ExpensiveArgument{&counter};
        }
        result.expectEqual(0, counter, "guarded stream-style expensive diagnostic is not evaluated when disabled");
    }

    {
        SemanticStateGuard guard;
        int sinkCalls = 0;
        auto logger = logger::BoundaryLogger::createForTest(
            testScope(),
            [&](logger::LogRecord) {
                ++sinkCalls;
            },
            logger::LogLevel::Debug,
            fixedTimestamp);

        int counter = 0;
        if (logger.enabled(logger::LogLevel::Debug)) {
            logger.debug() << ExpensiveArgument{&counter};
        }
        result.expectEqual(1, counter, "guarded stream-style expensive diagnostic is evaluated once when enabled");
        result.expectEqual(1, sinkCalls, "enabled guarded stream-style diagnostic emits one record");
    }

    {
        SemanticStateGuard guard;
        logger::LogManager::setGlobalLevel(logger::LogLevel::Info);
        logger::LogManager::freeze();

        auto log = snode::semantic::webHttpLog();
        int counter = 0;
        log.debug("value={}", ExpensiveArgument{&counter});
        result.expectTrue(!log.enabled(logger::LogLevel::Debug), "no-threshold convenience helper rejects disabled debug locally");
        result.expectEqual(0, counter, "no-threshold convenience helper does not format disabled debug arguments");
    }

    {
        SemanticStateGuard guard;
        logger::LogManager::setGlobalLevel(logger::LogLevel::Info);
        logger::LogManager::setComponentLevel("web.http", logger::LogLevel::Debug);
        logger::LogManager::freeze();

        auto log = snode::semantic::webHttpLog();
        int counter = 0;
        log.debug("value={}", ExpensiveArgument{&counter});
        result.expectTrue(log.enabled(logger::LogLevel::Debug), "component override enables debug for selected component");
        result.expectEqual(1, counter, "component override allows selected debug logs to format once");
    }

    {
        SemanticStateGuard guard;
        logger::LogManager::setGlobalLevel(logger::LogLevel::Info);
        logger::LogManager::freeze();

        int sinkCalls = 0;
        auto log = iot::mqtt::semantic::mqttLog(
            [&](logger::LogRecord) {
                ++sinkCalls;
            },
            logger::LogLevel::Trace,
            fixedTimestamp);
        int counter = 0;
        log.debug("value={}", ExpensiveArgument{&counter});
        result.expectTrue(log.enabled(logger::LogLevel::Debug), "explicit test threshold construction remains available");
        result.expectEqual(1, counter, "explicit test threshold still formats enabled debug once");
        result.expectEqual(1, sinkCalls, "explicit test threshold still emits to custom capture sink");
    }

    return result.processResult();
}
