#include "log/Logger.h"
#include "tests/support/TestResult.h"
#include "web/http/client/SemanticLog.h"

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
                return std::string("MIGRATION07ATICK");
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

    const auto enabledPath = tempLogPath("snodec-migration07a-enabled.log");
    {
        LoggerStateGuard guard(enabledPath.string());
        logger::LogManager::setGlobalLevel(logger::LogLevel::Trace);
        logger::LogManager::freeze();
        web::http::client::semantic::httpClientLog().debug("http client semantic logger emitted");
    }
    const auto enabledLog = readFile(enabledPath);
    result.expectTrue(enabledLog.find("http client semantic logger emitted") != std::string::npos,
                      "HTTP client semantic logger emits when enabled");
    result.expectTrue(enabledLog.find("framework/connection web.http.client ") != std::string::npos,
                      "records carry framework web.http.client component scope");

    const auto managerFilterPath = tempLogPath("snodec-migration07a-manager-filter.log");
    {
        LoggerStateGuard guard(managerFilterPath.string());
        logger::LogManager::setGlobalLevel(logger::LogLevel::Error);
        logger::LogManager::freeze();
        web::http::client::semantic::httpClientLog().debug("http client suppressed by manager");
    }
    result.expectTrue(readFile(managerFilterPath).find("suppressed by manager") == std::string::npos,
                      "LogManager filtering suppresses migrated HTTP client semantic calls");

    const auto backendFilterPath = tempLogPath("snodec-migration07a-backend-filter.log");
    {
        LoggerStateGuard guard(backendFilterPath.string());
        logger::LogManager::setGlobalLevel(logger::LogLevel::Trace);
        logger::LogManager::freeze();
        logger::Logger::setLogLevel(3);
        web::http::client::semantic::httpClientLog().debug("http client suppressed by backend");
    }
    result.expectTrue(readFile(backendFilterPath).find("suppressed by backend") != std::string::npos,
                      "semantic output accepted by LogManager is not suppressed by Logger::setLogLevel");

    const auto jsonPath = tempLogPath("snodec-migration07a-json.log");
    {
        LoggerStateGuard guard(jsonPath.string());
        logger::LogManager::setGlobalLevel(logger::LogLevel::Debug);
        logger::LogManager::setFormat(logger::LogManager::Format::Json);
        logger::LogManager::freeze();
        web::http::client::semantic::httpClientUpgradeLog().debug("http client upgrade json format respected");
    }
    const auto jsonLog = readFile(jsonPath);
    result.expectTrue(jsonLog.find("{\"v\":1") != std::string::npos &&
                          jsonLog.find("\"message\":\"http client upgrade json format respected\"") != std::string::npos &&
                          jsonLog.find("\"origin\":\"framework\"") != std::string::npos &&
                          jsonLog.find("\"component\":\"web.http.client.upgrade\"") != std::string::npos,
                      "JSON format selection is respected");

    std::vector<logger::LogRecord> sysErrorRecords;
    web::http::client::semantic::httpClientLog([&](logger::LogRecord record) {
        sysErrorRecords.push_back(std::move(record));
    }).sysError(logger::LogLevel::Error, ENOENT, "{} HTTP: file open failed", "conn-07a");
    result.expectTrue(sysErrorRecords.size() == 1 && sysErrorRecords[0].error && sysErrorRecords[0].error->code == ENOENT &&
                          sysErrorRecords[0].error->text.find("No such file") != std::string::npos,
                      "sysError errno code/text is preserved by HTTP client helper");

    std::vector<logger::LogRecord> sinkRecords;
    web::http::client::semantic::httpClientEventSourceLog([&](logger::LogRecord record) {
        sinkRecords.push_back(std::move(record));
    }).error("EventSource url not accepted: {}", "http+bad://example.invalid/sse");
    result.expectTrue(sinkRecords.size() == 1 && sinkRecords[0].component == "web.http.client.eventsource" &&
                          sinkRecords[0].message.find("EventSource url not accepted") != std::string::npos,
                      "sink-taking overload remains compatible and EventSource payload is preserved");

    const auto suppressedPath = tempLogPath("snodec-migration07a-suppressed.log");
    {
        LoggerStateGuard guard(suppressedPath.string());
        logger::LogManager::setGlobalLevel(logger::LogLevel::Error);
        logger::LogManager::freeze();
        int evaluations = 0;
        web::http::client::semantic::httpClientLog().debug("{}", ExpensiveValue{&evaluations});
        result.expectEqual(0, evaluations, "suppressed production formatting does not evaluate ExpensiveValue after PR #151");
    }

    std::vector<logger::LogRecord> requestRecords;
    web::http::client::semantic::httpClientLog([&](logger::LogRecord record) {
        requestRecords.push_back(std::move(record));
    }).info("{} HTTP: Request ({}) accepted: {} {} HTTP/{}.{}", "conn-07a", 7, "GET", "/resource", 1, 1);
    result.expectTrue(requestRecords.size() == 1 && requestRecords[0].message.find(
                                                        "conn-07a HTTP: Request (7) accepted: GET /resource HTTP/1.1") != std::string::npos,
                      "representative HTTP request payload preservation is covered");

    std::vector<logger::LogRecord> responseRecords;
    web::http::client::semantic::httpClientLog([&](logger::LogRecord record) {
        responseRecords.push_back(std::move(record));
    }).info("{}   HTTP/{}.{} {} {}", "conn-07a", 1, 1, "200", "OK");
    result.expectTrue(responseRecords.size() == 1 && responseRecords[0].message.find("conn-07a   HTTP/1.1 200 OK") != std::string::npos,
                      "representative HTTP response payload preservation is covered");

    std::vector<logger::LogRecord> upgradeRecords;
    web::http::client::semantic::httpClientUpgradeLog([&](logger::LogRecord record) {
        upgradeRecords.push_back(std::move(record));
    })
        .debug("{} HTTP upgrade: bootstrap {}; Protocol selected: {}; requested: {}; Subprotocol selected: {}; requested: {}",
               "conn-07a",
               "success",
               "websocket",
               "websocket",
               "chat",
               "chat");
    result.expectTrue(upgradeRecords.size() == 1 && upgradeRecords[0].component == "web.http.client.upgrade" &&
                          upgradeRecords[0].message.find("Protocol selected: websocket") != std::string::npos,
                      "representative HTTP upgrade payload preservation is covered");

    return result.processResult();
}
