#include "SemanticLog.h"
#include "log/Logger.h"
#include "log/SemanticLogger.h"
#include "tests/support/TestResult.h"

#include <cerrno>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <system_error>
#include <vector>

namespace {
    class LoggerStateGuard {
    public:
        explicit LoggerStateGuard(const std::filesystem::path& logFile) {
            logger::Logger::init();
            logger::LogManager::init();
            logger::Logger::setLogLevel(6);
            logger::Logger::setVerboseLevel(1);
            logger::Logger::setQuiet(true);
            logger::Logger::setDisableColor(true);
            logger::Logger::setTickResolver([]() {
                return std::string("E2ETICK");
            });
            logger::Logger::logToFile(logFile.string());
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
        std::filesystem::remove(path.string() + ".2", error);
        std::filesystem::remove(path.string() + ".3", error);
        return path;
    }

    std::vector<std::string> readLines(const std::filesystem::path& path) {
        std::ifstream in(path);
        std::vector<std::string> lines;
        std::string line;
        while (std::getline(in, line)) {
            if (!line.empty()) {
                lines.push_back(line);
            }
        }
        return lines;
    }

    bool startsWith(const std::string& value, const std::string& prefix) {
        return value.rfind(prefix, 0) == 0;
    }

    bool contains(const std::string& value, const std::string& needle) {
        return value.find(needle) != std::string::npos;
    }

    bool containsAnsi(const std::string& value) {
        return contains(value, "\033[");
    }

    std::chrono::system_clock::time_point fixedTimestamp() {
        using namespace std::chrono;
        return system_clock::time_point{seconds{1783254896} + milliseconds{789}};
    }

    logger::BoundaryLogger::Clock fixedClock() {
        return []() {
            return fixedTimestamp();
        };
    }
} // namespace

int main() {
    tests::support::TestResult result;

    const auto textPath = tempLogPath("snodec-semantic-e2e-text.log");
    {
        LoggerStateGuard guard(textPath);
        logger::LogManager::setFormat(logger::LogManager::Format::Text);
        auto log = snode::semantic::coreSocketLog(logger::Logger::semanticSink(), logger::LogLevel::Trace, fixedClock());
        log.info("semantic text output");
    }
    const auto textLines = readLines(textPath);
    result.expectEqual(1, static_cast<int>(textLines.size()), "semantic text emits one logical output line");
    if (!textLines.empty()) {
        const auto& line = textLines.front();
        result.expectTrue(line == "2026-07-05T12:34:56.789Z INF framework/connection core.socket — semantic text output",
                          "semantic text file output exactly matches the final plain formatter shape");
        result.expectTrue(!containsAnsi(line), "semantic text file output contains no ANSI");
        result.expectTrue(!startsWith(line, "INFO") && !startsWith(line, "DEBUG") && !startsWith(line, "TRACE") &&
                              !startsWith(line, "WARNING") && !startsWith(line, "ERROR") && !startsWith(line, "FATAL"),
                          "semantic text is not wrapped with a legacy severity prefix");
        result.expectTrue(!contains(line, " INFO   ") && !contains(line, " E2ETICK "),
                          "semantic text is not wrapped with legacy logger date/tick output");
    }

    const auto jsonPath = tempLogPath("snodec-semantic-e2e-json.log");
    {
        LoggerStateGuard guard(jsonPath);
        logger::LogManager::setFormat(logger::LogManager::Format::Json);
        auto log = snode::semantic::webHttpLog(logger::Logger::semanticSink(), logger::LogLevel::Trace, fixedClock());
        log.info("semantic json output");
        log.warn("semantic json warning");
    }
    const auto jsonLines = readLines(jsonPath);
    result.expectEqual(2, static_cast<int>(jsonLines.size()), "semantic JSON emits one object per record line");
    const std::vector<std::string> expectedJsonLines{
        R"({"v":1,"ts":"2026-07-05T12:34:56.789Z","level":"info","origin":"framework","boundary":"connection","component":"web.http","message":"semantic json output"})",
        R"({"v":1,"ts":"2026-07-05T12:34:56.789Z","level":"warn","origin":"framework","boundary":"connection","component":"web.http","message":"semantic json warning"})",
    };
    if (jsonLines.size() == expectedJsonLines.size()) {
        for (std::size_t i = 0; i < jsonLines.size(); ++i) {
            result.expectTrue(jsonLines[i] == expectedJsonLines[i], "semantic JSON file line exactly matches JSON baseline");
            result.expectTrue(startsWith(jsonLines[i], "{") && jsonLines[i].back() == '}', "semantic JSON line is one complete object");
            result.expectTrue(!containsAnsi(jsonLines[i]), "semantic JSON file line contains no ANSI");
            result.expectTrue(!startsWith(jsonLines[i], "INFO ") && !startsWith(jsonLines[i], "WARNING") &&
                                  !startsWith(jsonLines[i], "ERROR") && !contains(jsonLines[i], " E2ETICK "),
                              "semantic JSON line is not legacy-prefixed");
        }
    }

    const auto legacyPath = tempLogPath("snodec-semantic-e2e-legacy.log");
    {
        LoggerStateGuard guard(legacyPath);
        LOG(INFO) << "legacy info still works";
        errno = EACCES;
        PLOG(ERROR) << "legacy plog still works";
    }
    const auto legacyLines = readLines(legacyPath);
    std::ostringstream legacyLog;
    for (const auto& line : legacyLines) {
        legacyLog << line << '\n';
    }
    result.expectTrue(contains(legacyLog.str(), "INFO") && contains(legacyLog.str(), "legacy info still works"),
                      "legacy LOG still emits through the legacy path");
    result.expectTrue(contains(legacyLog.str(), "ERROR") && contains(legacyLog.str(), "legacy plog still works") &&
                          contains(legacyLog.str(), "Permission denied"),
                      "legacy PLOG still appends errno text through the legacy path");

    std::vector<logger::LogRecord> identityRecords;
    auto identityLog = snode::semantic::coreSocketLog(
        snode::semantic::LogIdentity{"socket-instance", logger::LogRole::Server, "tcp://127.0.0.1:8080"},
        [&](logger::LogRecord record) {
            identityRecords.push_back(std::move(record));
        },
        logger::LogLevel::Trace,
        fixedClock());
    identityLog.info("identity helper output");
    result.expectEqual(1, static_cast<int>(identityRecords.size()), "identity-capable helper emits one captured record");
    if (!identityRecords.empty()) {
        result.expectTrue(identityRecords.front().component == "core.socket", "identity helper preserves component");
        result.expectTrue(identityRecords.front().instance && *identityRecords.front().instance == "socket-instance",
                          "identity helper preserves instance");
        result.expectTrue(identityRecords.front().role && *identityRecords.front().role == logger::LogRole::Server,
                          "identity helper preserves role");
        result.expectTrue(identityRecords.front().connection && *identityRecords.front().connection == "tcp://127.0.0.1:8080",
                          "identity helper preserves connection");
    }

    const auto quietFilePath = tempLogPath("snodec-semantic-e2e-quiet-file.log");
    {
        LoggerStateGuard guard(quietFilePath);
        logger::Logger::setQuiet(true);
        auto log = snode::semantic::appLog(logger::Logger::semanticSink(), logger::LogLevel::Trace, fixedClock());
        log.info("quiet still writes file");
    }
    const auto quietFileLines = readLines(quietFilePath);
    result.expectEqual(1, static_cast<int>(quietFileLines.size()), "quiet mode does not suppress configured semantic file logging");
    result.expectTrue(!quietFileLines.empty() && contains(quietFileLines.front(), "quiet still writes file"),
                      "semanticSink still writes to file logger when quiet mode is enabled");

    return result.processResult();
}
