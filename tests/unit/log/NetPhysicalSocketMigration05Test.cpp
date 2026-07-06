#include "log/Logger.h"
#include "net/un/phy/stream/PhysicalSocketClient.h"
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

    using TestUnixPhysicalSocket = net::un::phy::stream::PhysicalSocketClient;

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
                return std::string("MIGRATION05TICK");
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

    const auto enabledPath = tempLogPath("snodec-migration05-enabled.log");
    {
        LoggerStateGuard guard(enabledPath.string());
        logger::LogManager::setGlobalLevel(logger::LogLevel::Trace);
        logger::LogManager::freeze();
        TestUnixPhysicalSocket owner;
        owner.log().debug("net physical socket semantic owner emitted");
    }
    const auto enabledLog = readFile(enabledPath);
    result.expectTrue(enabledLog.find("net physical socket semantic owner emitted") != std::string::npos,
                      "net physical-socket semantic owner emits when enabled");
    result.expectTrue(enabledLog.find(" framework system net.un ") != std::string::npos,
                      "emitted records carry framework net physical-socket component scope");

    const auto managerFilterPath = tempLogPath("snodec-migration05-manager-filter.log");
    {
        LoggerStateGuard guard(managerFilterPath.string());
        logger::LogManager::setGlobalLevel(logger::LogLevel::Error);
        logger::LogManager::freeze();
        TestUnixPhysicalSocket owner;
        owner.log().debug("net physical socket suppressed by manager");
    }
    result.expectTrue(readFile(managerFilterPath).find("suppressed by manager") == std::string::npos,
                      "LogManager filtering suppresses migrated net semantic calls");

    const auto backendFilterPath = tempLogPath("snodec-migration05-backend-filter.log");
    {
        LoggerStateGuard guard(backendFilterPath.string());
        logger::LogManager::setGlobalLevel(logger::LogLevel::Trace);
        logger::LogManager::freeze();
        logger::Logger::setLogLevel(3);
        TestUnixPhysicalSocket owner;
        owner.log().debug("net physical socket suppressed by backend");
    }
    result.expectTrue(readFile(backendFilterPath).find("suppressed by backend") == std::string::npos,
                      "Logger::setLogLevel backend filtering suppresses output");

    const auto jsonPath = tempLogPath("snodec-migration05-json.log");
    {
        LoggerStateGuard guard(jsonPath.string());
        logger::LogManager::setGlobalLevel(logger::LogLevel::Debug);
        logger::LogManager::setFormat(logger::LogManager::Format::Json);
        logger::LogManager::freeze();
        TestUnixPhysicalSocket owner;
        owner.log().debug("net physical socket json format respected");
    }
    const auto jsonLog = readFile(jsonPath);
    result.expectTrue(jsonLog.find("{\"v\":1") != std::string::npos &&
                          jsonLog.find("\"message\":\"net physical socket json format respected\"") != std::string::npos &&
                          jsonLog.find("\"origin\":\"framework\"") != std::string::npos &&
                          jsonLog.find("\"component\":\"net.un\"") != std::string::npos,
                      "JSON format selection is respected");

    std::vector<logger::LogRecord> sysErrorRecords;
    TestUnixPhysicalSocket sysErrorOwner;
    sysErrorOwner
        .log([&](logger::LogRecord record) {
            sysErrorRecords.push_back(std::move(record));
        })
        .sysError(logger::LogLevel::Error, EACCES, "net physical socket preserved errno");
    result.expectTrue(sysErrorRecords.size() == 1 && sysErrorRecords[0].message == "net physical socket preserved errno" &&
                          sysErrorRecords[0].error && sysErrorRecords[0].error->code == EACCES &&
                          sysErrorRecords[0].error->text.find("Permission") != std::string::npos,
                      "sysError errno code/text is preserved");

    std::vector<logger::LogRecord> sinkRecords;
    TestUnixPhysicalSocket sinkOwner;
    sinkOwner
        .log([&](logger::LogRecord record) {
            sinkRecords.push_back(std::move(record));
        })
        .info("sink overload still works");
    result.expectTrue(sinkRecords.size() == 1 && sinkRecords[0].message == "sink overload still works" &&
                          sinkRecords[0].component == "net.un",
                      "sink-taking overload remains compatible");

    const auto suppressedPath = tempLogPath("snodec-migration05-suppressed.log");
    {
        LoggerStateGuard guard(suppressedPath.string());
        logger::LogManager::setGlobalLevel(logger::LogLevel::Error);
        logger::LogManager::freeze();

        int evaluations = 0;
        TestUnixPhysicalSocket owner;
        owner.log().debug("{}", ExpensiveValue{&evaluations});
        result.expectEqual(0, evaluations, "suppressed production formatting does not evaluate ExpensiveValue after PR #151");
    }
    result.expectTrue(readFile(suppressedPath).empty(), "suppressed production formatting emits no backend output");

    return result.processResult();
}
