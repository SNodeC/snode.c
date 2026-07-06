#include "src/SemanticLog.h"
#include "tests/support/TestResult.h"

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
                return std::string("MIGRATION09TICK");
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

    const auto enabledPath = tempLogPath("snodec-migration09-enabled.log");
    {
        LoggerStateGuard guard(enabledPath.string());
        logger::LogManager::setGlobalLevel(logger::LogLevel::Trace);
        logger::LogManager::freeze();
        snode::semantic::mariaDbLog().debug() << "MariaDB connect: success";
        snode::semantic::tlsConfigLog().trace() << "tls-instance SSL/TLS SNI: found for example.test";
        snode::semantic::appLog().info() << "application request completed";
    }
    const auto enabledLog = readFile(enabledPath);
    result.expectTrue(enabledLog.find("MariaDB connect: success") != std::string::npos, "DB/MariaDB semantic logger emits when enabled");
    result.expectTrue(enabledLog.find("tls-instance SSL/TLS SNI: found for example.test") != std::string::npos,
                      "TLS config semantic logger emits when enabled");
    result.expectTrue(enabledLog.find("application request completed") != std::string::npos,
                      "app/example semantic logger emits when enabled");
    result.expectTrue(enabledLog.find(" framework connection db.mariadb ") != std::string::npos,
                      "DB records carry correct framework/connection/component scope");
    result.expectTrue(enabledLog.find(" framework configuration net.config.tls ") != std::string::npos,
                      "TLS records carry correct framework/configuration/component scope");
    result.expectTrue(enabledLog.find(" application application app ") != std::string::npos,
                      "application records carry correct application/application/component scope");

    const auto managerFilterPath = tempLogPath("snodec-migration09-manager-filter.log");
    {
        LoggerStateGuard guard(managerFilterPath.string());
        logger::LogManager::setGlobalLevel(logger::LogLevel::Error);
        logger::LogManager::freeze();
        snode::semantic::tlsConfigLog().trace() << "suppressed by manager";
    }
    result.expectTrue(readFile(managerFilterPath).find("suppressed by manager") == std::string::npos,
                      "LogManager filtering suppresses migrated Migration 9 semantic calls");

    const auto backendFilterPath = tempLogPath("snodec-migration09-backend-filter.log");
    {
        LoggerStateGuard guard(backendFilterPath.string());
        logger::LogManager::setGlobalLevel(logger::LogLevel::Trace);
        logger::LogManager::freeze();
        logger::Logger::setLogLevel(3);
        snode::semantic::mariaDbLog().debug() << "suppressed by backend";
    }
    result.expectTrue(readFile(backendFilterPath).find("suppressed by backend") == std::string::npos,
                      "Logger::setLogLevel backend filtering suppresses output");

    const auto jsonPath = tempLogPath("snodec-migration09-json.log");
    {
        LoggerStateGuard guard(jsonPath.string());
        logger::LogManager::setGlobalLevel(logger::LogLevel::Debug);
        logger::LogManager::setFormat(logger::LogManager::Format::Json);
        logger::LogManager::freeze();
        snode::semantic::appLog().debug() << "json format respected";
    }
    const auto jsonLog = readFile(jsonPath);
    result.expectTrue(
        jsonLog.find("{\"v\":1") != std::string::npos && jsonLog.find("\"message\":\"json format respected\"") != std::string::npos &&
            jsonLog.find("\"origin\":\"application\"") != std::string::npos && jsonLog.find("\"component\":\"app\"") != std::string::npos,
        "JSON format selection is respected");

    std::vector<logger::LogRecord> sysErrorRecords;
    errno = ENOENT;
    snode::semantic::sysError(snode::semantic::appLog([&](logger::LogRecord record) {
                                  sysErrorRecords.push_back(std::move(record));
                              }),
                              logger::LogLevel::Error,
                              ENOENT)
        << "Pipe not created";
    result.expectTrue(sysErrorRecords.size() == 1 && sysErrorRecords[0].error && sysErrorRecords[0].error->code == ENOENT &&
                          sysErrorRecords[0].error->text.find("No such file") != std::string::npos,
                      "sysError errno code/text is preserved");

    std::vector<logger::LogRecord> sinkRecords;
    snode::semantic::tlsConfigLog([&](logger::LogRecord record) {
        sinkRecords.push_back(std::move(record));
    }).warn()
        << "tls-instance SSL/TLS: Can not create SNI_SSL_CTX for domain 'example.test'";
    result.expectTrue(sinkRecords.size() == 1 && sinkRecords[0].component == "net.config.tls" &&
                          sinkRecords[0].message.find("example.test") != std::string::npos,
                      "sink-taking overload remains compatible and TLS/SNI payload is preserved");

    {
        LoggerStateGuard guard(tempLogPath("snodec-migration09-suppressed.log").string());
        logger::LogManager::setGlobalLevel(logger::LogLevel::Error);
        logger::LogManager::freeze();
        int evaluations = 0;
        snode::semantic::mariaDbLog(logger::Logger::semanticSink(), logger::LogLevel::Error).debug("{}", ExpensiveValue{&evaluations});
        result.expectEqual(0, evaluations, "suppressed production formatting does not evaluate ExpensiveValue after PR #151");
    }

    std::vector<logger::LogRecord> payloadRecords;
    const auto collect = [&](logger::LogRecord record) {
        payloadRecords.push_back(std::move(record));
    };
    snode::semantic::mariaDbLog(collect).warn() << "conn09 MariaDB connect: error: Access denied : 1045";
    snode::semantic::tlsConfigLog(collect).trace() << "tls09 SSL/TLS SNI: Lookup for sni='www.example.test' in sni certificates";
    snode::semantic::appLog(collect).error() << "app09 127.0.0.1:8080: startup failed";
    result.expectTrue(payloadRecords.size() == 3, "representative DB/TLS/app payloads emitted through sink path");
    result.expectTrue(payloadRecords[0].message.find("Access denied : 1045") != std::string::npos,
                      "representative DB/MariaDB payload preservation is covered");
    result.expectTrue(payloadRecords[1].message.find("www.example.test") != std::string::npos,
                      "representative TLS config/SNI payload preservation is covered");
    result.expectTrue(payloadRecords[2].message.find("127.0.0.1:8080") != std::string::npos,
                      "representative app/example payload preservation is covered");

    bool expensiveDumpBuilt = false;
    const auto tlsLog = snode::semantic::tlsConfigLog(
        [](logger::LogRecord) {
        },
        logger::LogLevel::Error);
    if (tlsLog.enabled(logger::LogLevel::Trace)) {
        expensiveDumpBuilt = true;
        tlsLog.trace("expensive SNI dump {}", "payload");
    }
    result.expectTrue(!expensiveDumpBuilt, "expensive payload/dump guard suppresses construction when disabled");

    return result.processResult();
}
