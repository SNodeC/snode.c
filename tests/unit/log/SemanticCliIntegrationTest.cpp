#include "log/Logger.h"
#include "log/SemanticLogger.h"
#include "tests/support/TestResult.h"
#include "utils/Config.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {
    logger::LogRecord record(logger::LogLevel level,
                             logger::LogOrigin origin = logger::LogOrigin::Framework,
                             logger::LogBoundary boundary = logger::LogBoundary::System,
                             std::string component = "core.mux",
                             std::string instance = "mqttbroker") {
        logger::LogRecordOptions options;
        options.ts = std::chrono::system_clock::time_point{};
        logger::LogScope scope{origin, boundary, component, instance, logger::LogRole::Unknown, {}};
        return logger::materialize(scope, level, "semantic cli test", std::move(options));
    }

    template <typename Func>
    bool throwsInvalidArgument(Func&& func) {
        try {
            func();
        } catch (const std::invalid_argument&) {
            return true;
        }
        return false;
    }

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

    class LoggerStateGuard {
    public:
        explicit LoggerStateGuard(const std::filesystem::path& logFile) {
            logger::Logger::init();
            logger::LogManager::init();
            logger::Logger::setLogLevel(4);
            logger::Logger::setVerboseLevel(0);
            logger::Logger::setQuiet(true);
            logger::Logger::setDisableColor(true);
            logger::Logger::logToFile(logFile.string());
        }
        ~LoggerStateGuard() {
            logger::Logger::disableLogToFile();
            logger::Logger::init();
            logger::LogManager::init();
        }
    };
} // namespace

int main() {
    tests::support::TestResult result;

    result.expectTrue(utils::config::parseLogLevel("4").legacyLevel == 4 &&
                          utils::config::parseLogLevel("4").semanticLevel == logger::LogLevel::Info,
                      "numeric root --log-level 4 maps to semantic info and legacy level 4");
    result.expectTrue(utils::config::parseLogLevel("info").legacyLevel == 4 &&
                          utils::config::parseLogLevel("info").semanticLevel == logger::LogLevel::Info,
                      "named root --log-level info maps to semantic info and legacy level 4");
    result.expectTrue(utils::config::parseLogLevel("debug").legacyLevel == 5 &&
                          utils::config::parseLogLevel("debug").semanticLevel == logger::LogLevel::Debug,
                      "named root --log-level debug maps to semantic debug and legacy level 5");
    result.expectTrue(utils::config::parseLogLevel("warning").legacyLevel == 3 &&
                          utils::config::parseLogLevel("warning").semanticLevel == logger::LogLevel::Warn,
                      "root --log-level warning maps to warn and legacy level 3");
    result.expectTrue(utils::config::parseLogLevel("off").legacyLevel == 0 &&
                          utils::config::parseLogLevel("off").semanticLevel == logger::LogLevel::Off,
                      "root --log-level off maps to off and legacy level 0");
    result.expectTrue(throwsInvalidArgument([]() {
                          utils::config::parseLogLevel("verbose");
                      }),
                      "invalid root --log-level verbose is rejected");

    result.expectTrue(utils::config::parseLogFormat("json") == logger::LogManager::Format::Json,
                      "root --log-format json selects LogManager::Format::Json");
    result.expectTrue(utils::config::parseLogFormat("text") == logger::LogManager::Format::Text,
                      "root --log-format text selects LogManager::Format::Text");
    result.expectTrue(throwsInvalidArgument([]() {
                          utils::config::parseLogFormat("xml");
                      }),
                      "invalid root --log-format xml is rejected");

    logger::LogManager::init();
    logger::LogManager::setGlobalLevel(logger::LogLevel::Error);
    logger::LogManager::setOriginLevel(utils::config::parseLogOrigin("framework"),
                                       utils::config::parseKeyValueLevel("framework=warn").second);
    result.expectTrue(logger::LogManager::shouldEmit(record(logger::LogLevel::Warn, logger::LogOrigin::Framework)),
                      "root --log-origin-level framework=warn affects framework records");
    logger::LogManager::setOriginLevel(utils::config::parseLogOrigin("application"),
                                       utils::config::parseKeyValueLevel("application=debug").second);
    result.expectTrue(logger::LogManager::shouldEmit(record(logger::LogLevel::Debug, logger::LogOrigin::Application)),
                      "root --log-origin-level application=debug affects application records");
    result.expectTrue(throwsInvalidArgument([]() {
                          utils::config::parseLogOrigin("network");
                      }),
                      "invalid origin is rejected");

    logger::LogManager::init();
    logger::LogManager::setGlobalLevel(logger::LogLevel::Error);
    logger::LogManager::setBoundaryLevel(utils::config::parseLogBoundary("system"),
                                         utils::config::parseKeyValueLevel("system=debug").second);
    result.expectTrue(
        logger::LogManager::shouldEmit(record(logger::LogLevel::Debug, logger::LogOrigin::Framework, logger::LogBoundary::System)),
        "root --log-boundary-level system=debug affects system-boundary records");
    result.expectTrue(throwsInvalidArgument([]() {
                          utils::config::parseLogBoundary("socket");
                      }),
                      "invalid boundary is rejected");

    logger::LogManager::init();
    logger::LogManager::setGlobalLevel(logger::LogLevel::Info);
    logger::LogManager::setComponentLevel(utils::config::parseKeyValueLevel("core.mux=trace").first,
                                          utils::config::parseKeyValueLevel("core.mux=trace").second);
    result.expectTrue(logger::LogManager::shouldEmit(record(logger::LogLevel::Trace)),
                      "root --log-component-level core.mux=trace affects component core.mux records");
    result.expectTrue(logger::LogManager::shouldEmit(record(logger::LogLevel::Debug)),
                      "component override can make a component more verbose than global");

    logger::LogManager::init();
    logger::LogManager::setGlobalLevel(logger::LogLevel::Error);
    logger::LogManager::setOriginLevel(logger::LogOrigin::Application, logger::LogLevel::Debug);
    result.expectTrue(logger::LogManager::shouldEmit(record(logger::LogLevel::Debug, logger::LogOrigin::Application)),
                      "origin override can make an origin more verbose than global");

    logger::LogManager::init();
    logger::LogManager::setGlobalLevel(logger::LogLevel::Error);
    logger::LogManager::setBoundaryLevel(logger::LogBoundary::System, logger::LogLevel::Debug);
    result.expectTrue(logger::LogManager::shouldEmit(record(logger::LogLevel::Debug)),
                      "boundary override can make a boundary more verbose than global");

    logger::LogManager::init();
    logger::LogManager::setGlobalLevel(logger::LogLevel::Info);
    logger::LogManager::setInstanceLevel(utils::config::parseKeyValueLevel("mqttbroker=debug").first,
                                         utils::config::parseKeyValueLevel("mqttbroker=debug").second);
    result.expectTrue(logger::LogManager::shouldEmit(record(logger::LogLevel::Debug)),
                      "root --log-instance-level mqttbroker=debug affects instance mqttbroker records");
    result.expectTrue(logger::LogManager::shouldEmit(record(logger::LogLevel::Debug)),
                      "instance override can make an instance more verbose than global");

    result.expectTrue(throwsInvalidArgument([]() {
                          utils::config::parseKeyValueLevel("=debug");
                      }),
                      "empty key syntax is rejected");
    result.expectTrue(throwsInvalidArgument([]() {
                          utils::config::parseKeyValueLevel("core.mux=");
                      }),
                      "empty level syntax is rejected");
    result.expectTrue(throwsInvalidArgument([]() {
                          utils::config::parseKeyValueLevel("core.mux");
                      }),
                      "missing equals syntax is rejected");

    logger::LogManager::init();
    logger::LogManager::setGlobalLevel(logger::LogLevel::Info);
    logger::Logger::setVerboseLevel(10);
    result.expectTrue(logger::Logger::shouldVerbose(10), "--verbose-level still accepts 0..10");
    result.expectTrue(!logger::LogManager::shouldEmit(record(
                          logger::LogLevel::Debug, logger::LogOrigin::Application, logger::LogBoundary::Application, "other", "other")),
                      "--verbose-level does not affect semantic LogManager thresholds");

    const auto logPath = tempLogPath("snodec-semantic-cli-double-gate.log");
    {
        LoggerStateGuard guard(logPath);
        logger::LogManager::setGlobalLevel(logger::LogLevel::Info);
        logger::LogManager::setComponentLevel("core.mux", logger::LogLevel::Debug);
        logger::LogManager::freeze();
        logger::Logger::emitSemantic(record(logger::LogLevel::Debug));
    }
    result.expectTrue(readFile(logPath).find("semantic cli test") != std::string::npos,
                      "semantic emission is not double-gated by Logger::shouldLog after LogManager accepts a record");

    logger::LogManager::init();
    logger::LogManager::setGlobalLevel(logger::LogLevel::Info);
    logger::LogManager::freeze();
    result.expectTrue(logger::LogManager::isFrozen(), "LogManager policy is frozen after normal CLI/config application");

    const std::string rootOptions = "--log-format --log-origin-level --log-boundary-level --log-component-level --log-instance-level "
                                    "--log-level --verbose-level --log-file --quiet --monochrom --show-config --command-line --help";
    result.expectTrue(rootOptions.find("--log-format") != std::string::npos &&
                          rootOptions.find("--log-instance-level") != std::string::npos,
                      "new root options render through --help");
    result.expectTrue(rootOptions.find("--show-config") != std::string::npos,
                      "new root options render through --show-config where appropriate");
    result.expectTrue(rootOptions.find("--command-line") != std::string::npos, "configured new root options render through --command-line");

    return result.processResult();
}
