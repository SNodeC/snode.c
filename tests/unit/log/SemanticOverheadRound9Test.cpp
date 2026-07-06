#include "log/Logger.h"
#include "log/SemanticLogger.h"
#include "tests/support/TestResult.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>
#include <system_error>
#include <optional>
#include <ostream>
#include <vector>

namespace {
    struct ExpensiveValue {
        int* evaluations;
    };

    std::ostream& operator<<(std::ostream& out, const ExpensiveValue& value) {
        ++*value.evaluations;
        return out << "expensive";
    }

    class StateGuard {
    public:
        explicit StateGuard(const std::string& logFile) {
            logger::Logger::init();
            logger::LogManager::init();
            logger::Logger::setLogLevel(6);
            logger::Logger::setVerboseLevel(0);
            logger::Logger::setQuiet(true);
            logger::Logger::setDisableColor(true);
            logger::Logger::setTickResolver([]() { return std::string("ROUND9OVERHD"); });
            logger::Logger::logToFile(logFile);
        }
        ~StateGuard() {
            logger::Logger::disableLogToFile();
            logger::Logger::setTickResolver({});
            logger::Logger::init();
            logger::LogManager::init();
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

    logger::LogScope scope(std::string_view component = "round9.overhead", std::string_view instance = "round9-overhead") {
        return {logger::LogOrigin::Framework, logger::LogBoundary::System, component, instance, logger::LogRole::Server, "round9-overhead-connection"};
    }

    std::chrono::system_clock::time_point fixedTimestamp() {
        using namespace std::chrono;
        return system_clock::time_point{seconds{1783254896} + milliseconds{789}};
    }
}

int main() {
    tests::support::TestResult result;

    int sinkCalls = 0;
    auto suppressed = logger::BoundaryLogger::createForTest(scope(), [&](logger::LogRecord) { ++sinkCalls; }, logger::LogLevel::Error, fixedTimestamp);
    suppressed.info("suppressed by threshold");
    result.expectEqual(0, sinkCalls, "BoundaryLogger threshold suppression does not invoke sink");
    suppressed.emit(logger::LogLevel::Off, "suppressed off");
    result.expectEqual(0, sinkCalls, "LogLevel::Off does not invoke sink");

    const auto semanticSuppressedPath = tempLogPath("snodec-round9-overhead-semantic.log");
    {
        StateGuard guard(semanticSuppressedPath.string());
        logger::LogManager::setGlobalLevel(logger::LogLevel::Off);
        logger::LogManager::freeze();
        logger::BoundaryLogger::createForTest(scope(), logger::Logger::semanticSink(), logger::LogLevel::Trace, fixedTimestamp).error("suppressed by LogManager");
    }
    result.expectTrue(readFile(semanticSuppressedPath).find("suppressed by LogManager") == std::string::npos, "LogManager semantic suppression prevents backend output");

    const auto backendSuppressedPath = tempLogPath("snodec-round9-overhead-backend.log");
    {
        StateGuard guard(backendSuppressedPath.string());
        logger::Logger::setLogLevel(2);
        logger::BoundaryLogger::createForTest(scope(), logger::Logger::semanticSink(), logger::LogLevel::Trace, fixedTimestamp).info("suppressed by backend threshold");
    }
    result.expectTrue(readFile(backendSuppressedPath).find("suppressed by backend threshold") == std::string::npos, "Logger::setLogLevel backend suppression prevents backend output");

    int expensiveEvaluations = 0;
    suppressed.info() << ExpensiveValue{&expensiveEvaluations};
    result.expectEqual(0, expensiveEvaluations, "suppressed stream path does not evaluate expensive insertion");

    const auto formatGatePath = tempLogPath("snodec-round9-overhead-format-gate.log");
    {
        StateGuard guard(formatGatePath.string());
        logger::LogManager::setGlobalLevel(logger::LogLevel::Off);
        logger::LogManager::setFormat(logger::LogManager::Format::Json);
        logger::LogManager::freeze();
        logger::BoundaryLogger::createForTest(scope(), logger::Logger::semanticSink(), logger::LogLevel::Trace, fixedTimestamp).error("format gate message");
    }
    const auto gatedLog = readFile(formatGatePath);
    result.expectTrue(gatedLog.find("format gate message") == std::string::npos && gatedLog.find("{\"v\":1") == std::string::npos,
                      "suppressed default backend path emits no formatted text");

    logger::LogManager::init();
    logger::LogManager::setComponentLevel("round9.overhead", logger::LogLevel::Warn);
    logger::LogManager::freeze();
    const auto first = logger::LogManager::effectiveLevel(scope());
    bool stable = true;
    for (int i = 0; i < 16; ++i) {
        stable = stable && logger::LogManager::effectiveLevel(scope()) == first;
    }
    result.expectTrue(stable && first == logger::LogLevel::Warn, "repeated effectiveLevel lookups after freeze are stable");
    logger::LogManager::init();

    std::vector<logger::LogRecord> records;
    std::optional<logger::BoundaryLogger> lifetimeLogger;
    {
        std::string component = "lifetime.component";
        std::string instance = "lifetime-instance";
        lifetimeLogger.emplace(logger::BoundaryLogger::createForTest(scope(component, instance), [&](logger::LogRecord record) { records.push_back(std::move(record)); }, logger::LogLevel::Trace, fixedTimestamp));
        component.assign("changed");
        instance.assign("changed");
    }
    lifetimeLogger->info("lifetime safe");
    result.expectTrue(records.size() == 1 && records[0].component == "lifetime.component" && records[0].instance && *records[0].instance == "lifetime-instance",
                      "original scope strings may be mutated/destroyed before logger emission");

    return result.processResult();
}
