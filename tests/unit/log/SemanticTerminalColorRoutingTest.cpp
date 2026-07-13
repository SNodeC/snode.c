#include "SemanticLog.h"
#include "log/Logger.h"
#include "log/SemanticLogger.h"
#include "tests/support/TestResult.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <unistd.h>
#include <vector>

namespace {
    bool contains(const std::string& value, const std::string& needle) {
        return value.find(needle) != std::string::npos;
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

    std::filesystem::path tempPath(const std::string& name) {
        auto path = std::filesystem::temp_directory_path() / name;
        std::error_code error;
        std::filesystem::remove(path, error);
        std::filesystem::remove(path.string() + ".1", error);
        return path;
    }

    std::string readFile(const std::filesystem::path& path) {
        std::ifstream in(path, std::ios::binary);
        std::ostringstream out;
        out << in.rdbuf();
        return out.str();
    }

    class StdoutCapture {
    public:
        explicit StdoutCapture(std::filesystem::path path)
            : path(std::move(path))
            , saved(::dup(STDOUT_FILENO)) {
            file = ::fopen(this->path.c_str(), "w+");
            ::dup2(::fileno(file), STDOUT_FILENO);
        }
        ~StdoutCapture() {
            ::fflush(stdout);
            ::dup2(saved, STDOUT_FILENO);
            ::close(saved);
            ::fclose(file);
        }
        std::string read() const {
            ::fflush(stdout);
            return readFile(path);
        }

    private:
        std::filesystem::path path;
        int saved;
        FILE* file;
    };

    void resetLogger() {
        logger::Logger::disableLogToFile();
        logger::Logger::init();
        logger::LogManager::init();
        logger::Logger::setLogLevel(6);
        logger::Logger::setQuiet(false);
    }
} // namespace

int main() {
    tests::support::TestResult result;

    {
        const auto stdoutPath = tempPath("snodec-semantic-color-stdout.log");
        const auto filePath = tempPath("snodec-semantic-color-file.log");
        StdoutCapture capture(stdoutPath);
        resetLogger();
        logger::Logger::setDisableColor(false);
        logger::Logger::logToFile(filePath.string());
        logger::LogManager::setFormat(logger::LogManager::Format::Text);
        auto log = snode::semantic::appLog(logger::Logger::semanticSink(), logger::LogLevel::Trace, fixedClock());
        log.info("colored stdout");
        const std::string stdoutText = capture.read();
        logger::Logger::disableLogToFile();
        const std::string fileText = readFile(filePath);
        result.expectTrue(contains(stdoutText, "\033["), "explicit color enable colors redirected semantic stdout");
        result.expectTrue(!contains(fileText, "\033["), "semantic text file remains ANSI-free while stdout is colored");
    }

    {
        const auto stdoutPath = tempPath("snodec-semantic-plain-stdout.log");
        StdoutCapture capture(stdoutPath);
        resetLogger();
        logger::Logger::setDisableColor(true);
        logger::LogManager::setFormat(logger::LogManager::Format::Text);
        auto log = snode::semantic::appLog(logger::Logger::semanticSink(), logger::LogLevel::Trace, fixedClock());
        log.info("plain stdout");
        result.expectTrue(!contains(capture.read(), "\033["), "explicit color disable keeps redirected semantic stdout plain");
    }

    {
        const auto stdoutPath = tempPath("snodec-semantic-json-stdout.log");
        StdoutCapture capture(stdoutPath);
        resetLogger();
        logger::Logger::setDisableColor(false);
        logger::LogManager::setFormat(logger::LogManager::Format::Json);
        auto log = snode::semantic::appLog(logger::Logger::semanticSink(), logger::LogLevel::Trace, fixedClock());
        log.info("json stdout");
        const std::string stdoutText = capture.read();
        result.expectTrue(contains(stdoutText, "\"level\":\"info\""), "semantic JSON still uses lowercase JSON level");
        result.expectTrue(!contains(stdoutText, "\033["), "semantic JSON stdout remains ANSI-free when color is enabled");
    }

    {
        const auto stdoutPath = tempPath("snodec-semantic-quiet-stdout.log");
        const auto filePath = tempPath("snodec-semantic-quiet-file.log");
        StdoutCapture capture(stdoutPath);
        resetLogger();
        logger::Logger::setDisableColor(false);
        logger::Logger::setQuiet(true);
        logger::Logger::logToFile(filePath.string());
        logger::LogManager::setFormat(logger::LogManager::Format::Text);
        auto log = snode::semantic::appLog(logger::Logger::semanticSink(), logger::LogLevel::Trace, fixedClock());
        log.info("quiet file");
        result.expectTrue(capture.read().empty(), "quiet mode suppresses semantic stdout");
        logger::Logger::disableLogToFile();
        result.expectTrue(contains(readFile(filePath), "quiet file"), "quiet mode still writes semantic file output");
    }

    resetLogger();
    return result.processResult();
}
