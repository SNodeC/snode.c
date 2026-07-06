#include "log/Logger.h"
#include "tests/support/TestResult.h"
#include "web/http/server/SemanticLog.h"

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
                return std::string("MIGRATION07BTICK");
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

    const auto enabledPath = tempLogPath("snodec-migration07b-enabled.log");
    {
        LoggerStateGuard guard(enabledPath.string());
        logger::LogManager::setGlobalLevel(logger::LogLevel::Trace);
        logger::LogManager::freeze();
        web::http::server::semantic::httpServerLog().debug("http server semantic logger emitted");
    }
    const auto enabledLog = readFile(enabledPath);
    result.expectTrue(enabledLog.find("http server semantic logger emitted") != std::string::npos,
                      "HTTP server semantic logger emits when enabled");
    result.expectTrue(enabledLog.find(" framework connection web.http.server ") != std::string::npos,
                      "records carry framework web.http.server component scope");

    const auto managerFilterPath = tempLogPath("snodec-migration07b-manager-filter.log");
    {
        LoggerStateGuard guard(managerFilterPath.string());
        logger::LogManager::setGlobalLevel(logger::LogLevel::Error);
        logger::LogManager::freeze();
        web::http::server::semantic::httpServerLog().debug("http server suppressed by manager");
    }
    result.expectTrue(readFile(managerFilterPath).find("suppressed by manager") == std::string::npos,
                      "LogManager filtering suppresses migrated HTTP server semantic calls");

    const auto backendFilterPath = tempLogPath("snodec-migration07b-backend-filter.log");
    {
        LoggerStateGuard guard(backendFilterPath.string());
        logger::LogManager::setGlobalLevel(logger::LogLevel::Trace);
        logger::LogManager::freeze();
        logger::Logger::setLogLevel(3);
        web::http::server::semantic::httpServerLog().debug("http server suppressed by backend");
    }
    result.expectTrue(readFile(backendFilterPath).find("suppressed by backend") == std::string::npos,
                      "Logger::setLogLevel backend filtering suppresses output");

    const auto jsonPath = tempLogPath("snodec-migration07b-json.log");
    {
        LoggerStateGuard guard(jsonPath.string());
        logger::LogManager::setGlobalLevel(logger::LogLevel::Debug);
        logger::LogManager::setFormat(logger::LogManager::Format::Json);
        logger::LogManager::freeze();
        web::http::server::semantic::httpServerUpgradeLog().debug("http server upgrade json format respected");
    }
    const auto jsonLog = readFile(jsonPath);
    result.expectTrue(jsonLog.find("{\"v\":1") != std::string::npos &&
                          jsonLog.find("\"message\":\"http server upgrade json format respected\"") != std::string::npos &&
                          jsonLog.find("\"origin\":\"framework\"") != std::string::npos &&
                          jsonLog.find("\"component\":\"web.http.server.upgrade\"") != std::string::npos,
                      "JSON format selection is respected");

    std::vector<logger::LogRecord> sysErrorRecords;
    web::http::server::semantic::httpServerLog([&](logger::LogRecord record) {
        sysErrorRecords.push_back(std::move(record));
    }).sysError(logger::LogLevel::Error, EPIPE, "{} HTTP: response write failed", "server-conn-07b");
    result.expectTrue(sysErrorRecords.size() == 1 && sysErrorRecords[0].error && sysErrorRecords[0].error->code == EPIPE &&
                          sysErrorRecords[0].error->text.find("Broken pipe") != std::string::npos,
                      "sysError errno code/text is preserved by HTTP server helper");

    std::vector<logger::LogRecord> sinkRecords;
    web::http::server::semantic::httpServerUpgradeLog([&](logger::LogRecord record) {
        sinkRecords.push_back(std::move(record));
    }).warn("HTTP upgrade: Overriding http upgrade library dir");
    result.expectTrue(sinkRecords.size() == 1 && sinkRecords[0].component == "web.http.server.upgrade" &&
                          sinkRecords[0].message == "HTTP upgrade: Overriding http upgrade library dir",
                      "sink-taking overload remains compatible");

    const auto suppressedPath = tempLogPath("snodec-migration07b-suppressed.log");
    {
        LoggerStateGuard guard(suppressedPath.string());
        logger::LogManager::setGlobalLevel(logger::LogLevel::Error);
        logger::LogManager::freeze();
        int evaluations = 0;
        web::http::server::semantic::httpServerLog().debug("{}", ExpensiveValue{&evaluations});
        result.expectEqual(0, evaluations, "suppressed production formatting does not evaluate ExpensiveValue after PR #151");
    }

    std::vector<logger::LogRecord> requestRecords;
    web::http::server::semantic::httpServerLog([&](logger::LogRecord record) {
        requestRecords.push_back(std::move(record));
    }).info("{} HTTP: Request parse success: {} {} HTTP/{}.{}", "server-conn-07b", "GET", "/resource", 1, 1);
    result.expectTrue(requestRecords.size() == 1 &&
                          requestRecords[0].message.find("server-conn-07b HTTP: Request parse success: GET /resource HTTP/1.1") !=
                              std::string::npos,
                      "representative HTTP request parse-success payload preservation is covered");

    std::vector<logger::LogRecord> responseRecords;
    web::http::server::semantic::httpServerLog([&](logger::LogRecord record) {
        responseRecords.push_back(std::move(record));
    })
        .info("{} HTTP: Response completed for request: {} {} HTTP/{}.{}; HTTP/{}.{} {} {}",
              "server-conn-07b",
              "GET",
              "/resource",
              1,
              1,
              1,
              1,
              200,
              "OK");
    result.expectTrue(responseRecords.size() == 1 && responseRecords[0].message.find("HTTP/1.1 200 OK") != std::string::npos,
                      "representative HTTP response payload preservation is covered");

    std::vector<logger::LogRecord> upgradeRecords;
    web::http::server::semantic::httpServerUpgradeLog([&](logger::LogRecord record) {
        upgradeRecords.push_back(std::move(record));
    })
        .debug("{} HTTP: Upgrade bootstrap {}; Protocol selected: {}; requested: {}; Subprotocol selected: {}; requested: {}",
               "server-conn-07b",
               "success",
               "websocket",
               "websocket",
               "chat",
               "chat");
    result.expectTrue(upgradeRecords.size() == 1 && upgradeRecords[0].component == "web.http.server.upgrade" &&
                          upgradeRecords[0].message.find("Protocol selected: websocket") != std::string::npos,
                      "representative HTTP server-upgrade payload preservation is covered");

    std::vector<logger::LogRecord> errorRecords;
    web::http::server::semantic::httpServerLog([&](logger::LogRecord record) {
        errorRecords.push_back(std::move(record));
    })
        .error("{} HTTP: Request parse error: {} ({}) / HTTP upgrade: Request has gone away / HTTP upgrade: Unexpected disconnect",
               "server-conn-07b",
               "Bad Request",
               400);
    result.expectTrue(errorRecords.size() == 1 &&
                          errorRecords[0].message.find("Request parse error: Bad Request (400)") != std::string::npos &&
                          errorRecords[0].message.find("Request has gone away") != std::string::npos &&
                          errorRecords[0].message.find("Unexpected disconnect") != std::string::npos,
                      "representative parse-error/gone-away/unexpected-disconnect payload preservation is covered");

    return result.processResult();
}
