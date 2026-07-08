#include "core/socket/SocketAddress.h"
#include "core/socket/stream/SocketConnection.h"
#include "log/Logger.h"
#include "log/SemanticLogger.h"
#include "net/config/ConfigInstance.h"
#include "tests/support/TestResult.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <system_error>

namespace {
    std::chrono::system_clock::time_point fixedTimestamp() {
        using namespace std::chrono;
        return system_clock::time_point{seconds{1783254896} + milliseconds{789}};
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
                return std::string("ROUND7TICK000");
            });
            logger::Logger::logToFile(logFile);
        }
        ~LoggerStateGuard() {
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

    logger::LogScope scope(std::string_view instance = "round7-instance") {
        return {logger::LogOrigin::Framework,
                logger::LogBoundary::Connection,
                "core.socket.stream",
                instance,
                logger::LogRole::Server,
                "round7-connection"};
    }

    logger::LogRecord record(logger::LogLevel level, std::string message, std::string_view instance = "round7-instance") {
        logger::LogRecordOptions options;
        options.ts = fixedTimestamp();
        return logger::materialize(scope(instance), level, std::move(message), std::move(options));
    }

    class TestConfigInstance : public net::config::ConfigInstance {
    public:
        explicit TestConfigInstance(const std::string& instanceName)
            : ConfigInstance(instanceName, Role::SERVER) {
        }
        ~TestConfigInstance() override = default;
    };
} // namespace

int main() {
    tests::support::TestResult result;

    logger::LogManager::init();
    result.expectTrue(!logger::LogManager::isFrozen(), "LogManager init leaves policy mutable");
    result.expectTrue(logger::LogManager::effectiveLevel(scope()) == logger::LogLevel::Trace, "LogManager default global level is Trace");
    result.expectTrue(logger::LogManager::format() == logger::LogManager::Format::Text, "LogManager default format is Text");

    logger::LogManager::setGlobalLevel(logger::LogLevel::Warn);
    result.expectTrue(!logger::LogManager::shouldEmit(record(logger::LogLevel::Info, "hidden by global")), "global Warn filters Info");
    result.expectTrue(logger::LogManager::shouldEmit(record(logger::LogLevel::Warn, "allowed by global")), "global Warn allows Warn");

    logger::LogManager::init();
    logger::LogManager::setGlobalLevel(logger::LogLevel::Error);
    logger::LogManager::setOriginLevel(logger::LogOrigin::Framework, logger::LogLevel::Warn);
    result.expectTrue(logger::LogManager::effectiveLevel(scope()) == logger::LogLevel::Warn, "origin overrides global");
    logger::LogManager::setBoundaryLevel(logger::LogBoundary::Connection, logger::LogLevel::Debug);
    result.expectTrue(logger::LogManager::effectiveLevel(scope()) == logger::LogLevel::Debug, "boundary overrides origin");
    logger::LogManager::setComponentLevel("core.socket.stream", logger::LogLevel::Info);
    result.expectTrue(logger::LogManager::effectiveLevel(scope()) == logger::LogLevel::Info, "component overrides boundary");
    logger::LogManager::setInstanceLevel("round7-instance", logger::LogLevel::Trace);
    result.expectTrue(logger::LogManager::effectiveLevel(scope()) == logger::LogLevel::Trace, "instance overrides component");
    result.expectTrue(logger::LogManager::effectiveLevel(scope("")) == logger::LogLevel::Info,
                      "missing instance does not match instance override");

    logger::LogManager::init();
    logger::LogManager::setGlobalLevel(logger::LogLevel::Off);
    result.expectTrue(!logger::LogManager::shouldEmit(record(logger::LogLevel::Critical, "off hides critical")),
                      "LogLevel::Off suppresses records");

    logger::LogManager::init();
    logger::LogManager::setGlobalLevel(logger::LogLevel::Info);
    logger::LogManager::setFormat(logger::LogManager::Format::Json);
    logger::LogManager::freeze();
    bool threw = false;
    try {
        logger::LogManager::setGlobalLevel(logger::LogLevel::Trace);
    } catch (const std::logic_error&) {
        threw = true;
    }
    result.expectTrue(threw, "post-freeze setters throw std::logic_error");
    result.expectTrue(logger::LogManager::effectiveLevel(scope()) == logger::LogLevel::Info, "post-freeze setter does not mutate policy");
    result.expectTrue(logger::LogManager::format() == logger::LogManager::Format::Json, "format is frozen with policy");

    const auto sample = record(logger::LogLevel::Info, "format sample");
    logger::LogManager::init();
    logger::LogManager::setFormat(logger::LogManager::Format::Text);
    result.expectTrue(logger::LogManager::formatRecord(sample) == logger::formatText(sample), "Text format uses formatText");
    logger::LogManager::setFormat(logger::LogManager::Format::Json);
    const std::string json = logger::LogManager::formatRecord(sample);
    result.expectTrue(json == logger::formatJsonV1(sample), "Json format uses formatJsonV1");
    result.expectTrue(json.find("\"v\"") != std::string::npos && json.find("\"ts\"") != std::string::npos &&
                          json.find("\"level\"") != std::string::npos && json.find("\"origin\"") != std::string::npos &&
                          json.find("\"boundary\"") != std::string::npos && json.find("\"component\"") != std::string::npos &&
                          json.find("\"message\"") != std::string::npos,
                      "Json output includes mandatory JSON v1 fields");
    const auto minimal = logger::materialize(
        {logger::LogOrigin::Application, logger::LogBoundary::Application, "round7.minimal", "", logger::LogRole::Unknown, ""},
        logger::LogLevel::Info,
        "minimal",
        logger::LogRecordOptions{.ts = fixedTimestamp()});
    const std::string minimalJson = logger::formatJsonV1(minimal);
    result.expectTrue(minimalJson.find(":null") == std::string::npos && minimalJson.find("\"instance\"") == std::string::npos &&
                          minimalJson.find("\"role\"") == std::string::npos && minimalJson.find("\"connection\"") == std::string::npos,
                      "Json output omits absent optional fields rather than null");

    const auto semanticPath = tempLogPath("snodec-round7-semantic-filter.log");
    {
        LoggerStateGuard guard(semanticPath.string());
        logger::LogManager::setGlobalLevel(logger::LogLevel::Warn);
        logger::LogManager::freeze();
        logger::Logger::emitSemantic(record(logger::LogLevel::Info, "semantic info hidden"));
        logger::Logger::emitSemantic(record(logger::LogLevel::Warn, "semantic warning visible"));
    }
    const auto semanticLog = readFile(semanticPath);
    result.expectTrue(semanticLog.find("semantic info hidden") == std::string::npos, "Logger::emitSemantic honors semantic filter");
    result.expectTrue(semanticLog.find("semantic warning visible") != std::string::npos, "Logger::emitSemantic emits allowed record");

    const auto objectPath = tempLogPath("snodec-round7-object-filter.log");
    {
        LoggerStateGuard guard(objectPath.string());
        logger::LogManager::setGlobalLevel(logger::LogLevel::Error);
        logger::LogManager::freeze();
        TestConfigInstance config("round7-config");
        config.log().info("object info hidden");
        config.log().error("object error visible");
    }
    const auto objectLog = readFile(objectPath);
    result.expectTrue(objectLog.find("object info hidden") == std::string::npos,
                      "default no-argument object logger respects semantic filtering");
    result.expectTrue(objectLog.find("object error visible") != std::string::npos,
                      "default no-argument object logger emits allowed semantic record");

    const auto gatePath = tempLogPath("snodec-round7-backend-gate.log");
    {
        LoggerStateGuard guard(gatePath.string());
        logger::LogManager::setGlobalLevel(logger::LogLevel::Trace);
        logger::LogManager::freeze();
        logger::Logger::setLogLevel(3);
        logger::Logger::emitSemantic(record(logger::LogLevel::Info, "backend info hidden"));
        logger::Logger::emitSemantic(record(logger::LogLevel::Warn, "backend warn visible"));
        LOG(INFO) << "legacy info hidden";
        LOG(WARNING) << "legacy warning visible";
    }
    const auto gateLog = readFile(gatePath);
    result.expectTrue(gateLog.find("backend info hidden") != std::string::npos,
                      "semantic records accepted by LogManager are not double-gated by Logger::setLogLevel");
    result.expectTrue(gateLog.find("backend warn visible") != std::string::npos, "semantic warning still emits after semantic filtering");
    result.expectTrue(gateLog.find("legacy info hidden") == std::string::npos &&
                          gateLog.find("legacy warning visible") != std::string::npos,
                      "legacy LOG macro behavior remains unchanged");

    const auto jsonPath = tempLogPath("snodec-round7-json-format.log");
    {
        LoggerStateGuard guard(jsonPath.string());
        logger::LogManager::setFormat(logger::LogManager::Format::Json);
        logger::LogManager::freeze();
        logger::Logger::emitSemantic(record(logger::LogLevel::Info, "json backend visible"));
    }
    const auto jsonLog = readFile(jsonPath);
    result.expectTrue(jsonLog.find("{\"v\":1") != std::string::npos &&
                          jsonLog.find("\"message\":\"json backend visible\"") != std::string::npos,
                      "Json format reaches semantic backend output");

    return result.processResult();
}
