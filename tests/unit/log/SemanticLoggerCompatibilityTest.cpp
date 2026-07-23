#include "log/Logger.h"
#include "log/SemanticLogger.h"
#include "tests/support/TestResult.h"

#include <cerrno>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>
#include <system_error>

namespace {
    class StateGuard {
    public:
        explicit StateGuard(const std::string& logFile)
            : savedErrno(errno) {
            logger::Logger::init();
            logger::LogManager::init();
            logger::Logger::setLogLevel(6);
            logger::Logger::setVerboseLevel(0);
            logger::Logger::setQuiet(true);
            logger::Logger::setDisableColor(true);
            logger::Logger::logToFile(logFile);
        }

        ~StateGuard() {
            logger::Logger::disableLogToFile();
            logger::Logger::init();
            logger::LogManager::init();
            errno = savedErrno;
        }

    private:
        int savedErrno;
    };

    std::filesystem::path tempLogPath(const std::string& name) {
        auto path = std::filesystem::temp_directory_path() / name;
        std::error_code error;
        std::filesystem::remove(path, error);
        std::filesystem::remove(path.string() + ".1", error);
        return path;
    }

    std::string readFile(const std::filesystem::path& path) {
        std::ifstream input(path);
        return std::string(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
    }

    logger::LogScope scope() {
        return {logger::LogOrigin::Framework,
                logger::LogBoundary::System,
                "compatibility.component",
                "compatibility-instance",
                logger::LogRole::Server,
                "compatibility-connection"};
    }

    std::chrono::system_clock::time_point fixedTimestamp() {
        return std::chrono::system_clock::time_point{std::chrono::seconds{1783254896}};
    }
} // namespace

int main() {
    tests::support::TestResult result;

    const auto legacyPath = tempLogPath("snodec-semantic-compatibility-legacy.log");
    {
        StateGuard guard(legacyPath.string());
        LOG(INFO) << "legacy info";
        errno = EACCES;
        PLOG(ERROR) << "legacy plog";
        logger::Logger::setVerboseLevel(1);
        logger::Logger::setLogLevel(3);
        LOG(INFO) << "numeric info hidden";
        LOG(WARNING) << "numeric warning visible";
    }
    const std::string legacyLog = readFile(legacyPath);
    result.expectTrue(legacyLog.find("legacy info") != std::string::npos, "legacy LOG remains functional");
    result.expectTrue(legacyLog.find("legacy plog") != std::string::npos &&
                          legacyLog.find("Permission denied") != std::string::npos,
                      "legacy PLOG remains functional and preserves errno text");
    result.expectTrue(legacyLog.find("numeric info hidden") == std::string::npos &&
                          legacyLog.find("numeric warning visible") != std::string::npos,
                      "legacy numeric thresholds retain their behavior");

    const auto semanticOffPath = tempLogPath("snodec-semantic-compatibility-off.log");
    {
        StateGuard guard(semanticOffPath.string());
        logger::LogManager::setGlobalLevel(logger::LogLevel::Off);
        logger::LogManager::freeze();
        LOG(ERROR) << "legacy unaffected by semantic off";
        logger::BoundaryLogger::createForTest(scope(), logger::Logger::semanticSink(), logger::LogLevel::Off, fixedTimestamp)
            .error("semantic filtered by off");
    }
    const std::string semanticOffLog = readFile(semanticOffPath);
    result.expectTrue(semanticOffLog.find("legacy unaffected by semantic off") != std::string::npos &&
                          semanticOffLog.find("semantic filtered by off") == std::string::npos,
                      "semantic Off does not disable legacy output");

    const auto coexistencePath = tempLogPath("snodec-semantic-compatibility-coexistence.log");
    {
        StateGuard guard(coexistencePath.string());
        logger::LogManager::setGlobalLevel(logger::LogLevel::Trace);
        logger::LogManager::freeze();
        logger::Logger::setLogLevel(2);
        LOG(INFO) << "legacy info remains filtered";
        logger::BoundaryLogger::createForTest(scope(), logger::Logger::semanticSink(), logger::LogLevel::Trace, fixedTimestamp)
            .info("accepted semantic info");
    }
    const std::string coexistenceLog = readFile(coexistencePath);
    result.expectTrue(coexistenceLog.find("legacy info remains filtered") == std::string::npos &&
                          coexistenceLog.find("accepted semantic info") != std::string::npos,
                      "legacy thresholds do not re-filter accepted semantic records");

    return result.processResult();
}
