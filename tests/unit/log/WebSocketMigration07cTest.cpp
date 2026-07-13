#include "log/Logger.h"
#include "tests/support/TestResult.h"
#include "web/websocket/SemanticLog.h"

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
                return std::string("MIGRATION07CTICK");
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

    const auto enabledPath = tempLogPath("snodec-migration07c-enabled.log");
    {
        LoggerStateGuard guard(enabledPath.string());
        logger::LogManager::setGlobalLevel(logger::LogLevel::Trace);
        logger::LogManager::freeze();
        web::websocket::semantic::webSocketLog().debug("websocket semantic logger emitted");
    }
    const auto enabledLog = readFile(enabledPath);
    result.expectTrue(enabledLog.find("websocket semantic logger emitted") != std::string::npos,
                      "WebSocket semantic logger emits when enabled");
    result.expectTrue(enabledLog.find(" framework/connection web.websocket ") != std::string::npos,
                      "records carry framework web.websocket component scope");

    const auto managerFilterPath = tempLogPath("snodec-migration07c-manager-filter.log");
    {
        LoggerStateGuard guard(managerFilterPath.string());
        logger::LogManager::setGlobalLevel(logger::LogLevel::Error);
        logger::LogManager::freeze();
        web::websocket::semantic::webSocketLog().debug("websocket suppressed by manager");
    }
    result.expectTrue(readFile(managerFilterPath).find("suppressed by manager") == std::string::npos,
                      "LogManager filtering suppresses migrated WebSocket semantic calls");

    const auto backendFilterPath = tempLogPath("snodec-migration07c-backend-filter.log");
    {
        LoggerStateGuard guard(backendFilterPath.string());
        logger::LogManager::setGlobalLevel(logger::LogLevel::Trace);
        logger::LogManager::freeze();
        logger::Logger::setLogLevel(3);
        web::websocket::semantic::webSocketLog().debug("websocket suppressed by backend");
    }
    result.expectTrue(readFile(backendFilterPath).find("suppressed by backend") != std::string::npos,
                      "semantic output accepted by LogManager is not suppressed by Logger::setLogLevel");

    const auto jsonPath = tempLogPath("snodec-migration07c-json.log");
    {
        LoggerStateGuard guard(jsonPath.string());
        logger::LogManager::setGlobalLevel(logger::LogLevel::Debug);
        logger::LogManager::setFormat(logger::LogManager::Format::Json);
        logger::LogManager::freeze();
        web::websocket::semantic::webSocketFrameLog().debug("websocket frame json format respected");
    }
    const auto jsonLog = readFile(jsonPath);
    result.expectTrue(jsonLog.find("{\"v\":1") != std::string::npos &&
                          jsonLog.find("\"message\":\"websocket frame json format respected\"") != std::string::npos &&
                          jsonLog.find("\"origin\":\"framework\"") != std::string::npos &&
                          jsonLog.find("\"component\":\"web.websocket.frame\"") != std::string::npos,
                      "JSON format selection is respected");

    std::vector<logger::LogRecord> sysErrorRecords;
    web::websocket::semantic::webSocketLog([&](logger::LogRecord record) {
        sysErrorRecords.push_back(std::move(record));
    }).sysError(logger::LogLevel::Error, EPIPE, "{} WebSocket: frame write failed", "ws-conn-07c");
    result.expectTrue(sysErrorRecords.size() == 1 && sysErrorRecords[0].error && sysErrorRecords[0].error->code == EPIPE &&
                          sysErrorRecords[0].error->text.find("Broken pipe") != std::string::npos,
                      "sysError errno code/text is preserved by WebSocket helper");

    std::vector<logger::LogRecord> sinkRecords;
    web::websocket::semantic::webSocketFactoryLog([&](logger::LogRecord record) {
        sinkRecords.push_back(std::move(record));
    }).warn("WebSocket: Overriding websocket subprotocol library dir");
    result.expectTrue(sinkRecords.size() == 1 && sinkRecords[0].component == "web.websocket.factory" &&
                          sinkRecords[0].message == "WebSocket: Overriding websocket subprotocol library dir",
                      "sink-taking overload remains compatible");

    const auto suppressedPath = tempLogPath("snodec-migration07c-suppressed.log");
    {
        LoggerStateGuard guard(suppressedPath.string());
        logger::LogManager::setGlobalLevel(logger::LogLevel::Error);
        logger::LogManager::freeze();
        int evaluations = 0;
        web::websocket::semantic::webSocketLog().debug("{}", ExpensiveValue{&evaluations});
        result.expectEqual(0, evaluations, "suppressed production formatting does not evaluate ExpensiveValue after PR #151");
    }

    std::vector<logger::LogRecord> subProtocolRecords;
    web::websocket::semantic::webSocketSubProtocolLog([&](logger::LogRecord record) {
        subProtocolRecords.push_back(std::move(record));
    }).info("{} WebSocketContext: Subprotocol '{}' connect / disconnected", "ws-conn-07c", "chat");
    result.expectTrue(subProtocolRecords.size() == 1 && subProtocolRecords[0].component == "web.websocket.subprotocol" &&
                          subProtocolRecords[0].message.find("ws-conn-07c WebSocketContext: Subprotocol 'chat' connect") !=
                              std::string::npos &&
                          subProtocolRecords[0].message.find("disconnected") != std::string::npos,
                      "representative subprotocol connect/disconnect payload preservation is covered");

    std::vector<logger::LogRecord> frameRecords;
    web::websocket::semantic::webSocketFrameLog([&](logger::LogRecord record) {
        frameRecords.push_back(std::move(record));
    }).trace("WebSocket receive: Frame data\n{} opcode={} payloadLength={} chunkLength={}", "00 01 02", 2, 3, 3);
    result.expectTrue(frameRecords.size() == 1 && frameRecords[0].message.find("Frame data") != std::string::npos &&
                          frameRecords[0].message.find("opcode=2") != std::string::npos &&
                          frameRecords[0].message.find("payloadLength=3") != std::string::npos,
                      "representative receiver/transmitter frame payload preservation is covered");

    std::vector<logger::LogRecord> factoryRecords;
    web::websocket::semantic::webSocketFactoryLog([&](logger::LogRecord record) {
        factoryRecords.push_back(std::move(record));
    })
        .debug("WebSocket subprotocol: plugin '{}' selected from dynamic cache; factory function {}; loaded and added to dynamic cache",
               "chat",
               "chatServerSubProtocolFactory");
    result.expectTrue(factoryRecords.size() == 1 && factoryRecords[0].message.find("plugin 'chat'") != std::string::npos &&
                          factoryRecords[0].message.find("dynamic cache") != std::string::npos &&
                          factoryRecords[0].message.find("chatServerSubProtocolFactory") != std::string::npos,
                      "representative plugin/factory/cache payload preservation is covered");

    std::vector<logger::LogRecord> lifecycleRecords;
    web::websocket::semantic::webSocketSubProtocolLog([&](logger::LogRecord record) {
        lifecycleRecords.push_back(std::move(record));
    })
        .debug("{} Subprotocol '{}': Ping sent / Pong received / start / stopped; Total Payload sent: {}; Total Payload processed: {}",
               "ws-conn-07c",
               "chat",
               42,
               41);
    result.expectTrue(lifecycleRecords.size() == 1 && lifecycleRecords[0].message.find("Ping sent") != std::string::npos &&
                          lifecycleRecords[0].message.find("Pong received") != std::string::npos &&
                          lifecycleRecords[0].message.find("Total Payload sent: 42") != std::string::npos &&
                          lifecycleRecords[0].message.find("Total Payload processed: 41") != std::string::npos,
                      "representative ping/pong/start/stop/payload-total payload preservation is covered");

    logger::LogManager::setGlobalLevel(logger::LogLevel::Error);
    logger::LogManager::freeze();
    int hexDumpEvaluations = 0;
    auto frameLog = web::websocket::semantic::webSocketFrameLog(
        [&](logger::LogRecord) {
            ++hexDumpEvaluations;
        },
        logger::LogLevel::Error);
    if (frameLog.enabled(logger::LogLevel::Trace)) {
        ++hexDumpEvaluations;
        frameLog.trace("WebSocket receive: Frame data\n{}", "expensive hex dump");
    }
    result.expectEqual(0, hexDumpEvaluations, "hex dump guard avoids evaluation when trace is disabled");

    return result.processResult();
}
