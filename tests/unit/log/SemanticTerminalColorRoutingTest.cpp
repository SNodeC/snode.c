#include "SemanticLog.h"
#include "log/Logger.h"
#include "log/SemanticLogger.h"
#include "tests/support/TestResult.h"
#include "utils/hexdump.h"

#include <cctype>
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

    std::string stripAnsi(const std::string& value) {
        std::string stripped;
        for (std::size_t i = 0; i < value.size();) {
            if (value[i] == '\033' && i + 2 < value.size() && value[i + 1] == '[') {
                std::size_t j = i + 2;
                while (j < value.size() && (std::isdigit(static_cast<unsigned char>(value[j])) || value[j] == ';')) {
                    ++j;
                }
                if (j < value.size() && value[j] == 'm') {
                    i = j + 1;
                    continue;
                }
            }
            stripped.push_back(value[i]);
            ++i;
        }
        return stripped;
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

    logger::PresentedMessage dumpMessage() {
        const std::vector<char> bytes = {'A', 0, static_cast<char>(0x1B), '\n', 'Z'};
        const auto dump = utils::hexDumpPresentation(bytes, 4);
        return logger::PresentedMessage{.plain = "dump:\n" + dump.plain, .terminal = "dump:\n" + dump.terminal};
    }
} // namespace

int main() {
    tests::support::TestResult result;

    {
        const auto stdoutPath = tempPath("snodec-semantic-default-redirected-stdout.log");
        StdoutCapture capture(stdoutPath);
        logger::Logger::init();
        logger::LogManager::init();
        logger::Logger::setLogLevel(6);
        logger::Logger::setQuiet(false);
        logger::LogManager::setFormat(logger::LogManager::Format::Text);
        auto log = snode::semantic::appLog(logger::Logger::semanticSink(), logger::LogLevel::Trace, fixedClock());
        log.emit(logger::LogLevel::Info, dumpMessage());
        const std::string stdoutText = capture.read();
        result.expectTrue(contains(stdoutText, ": 00000000  41 00 1b 0a 5a"), "default redirected semantic stdout emits plain dump");
        result.expectTrue(!contains(stdoutText, "\033["), "non-TTY redirected semantic stdout defaults to plain dump");
        result.expectTrue(!contains(stdoutText, "\\x1B["), "non-TTY redirected semantic stdout contains no literal color clutter");
    }

    {
        const auto stdoutPath = tempPath("snodec-semantic-color-stdout.log");
        const auto filePath = tempPath("snodec-semantic-color-file.log");
        StdoutCapture capture(stdoutPath);
        resetLogger();
        logger::Logger::setDisableColor(false);
        logger::Logger::logToFile(filePath.string());
        logger::LogManager::setFormat(logger::LogManager::Format::Text);
        auto log = snode::semantic::appLog(logger::Logger::semanticSink(), logger::LogLevel::Trace, fixedClock());
        log.emit(logger::LogLevel::Info, dumpMessage());
        const std::string stdoutText = capture.read();
        logger::Logger::disableLogToFile();
        const std::string fileText = readFile(filePath);
        result.expectTrue(contains(stdoutText, "\033[34m") && contains(stdoutText, "\033[32m") && contains(stdoutText, "\033[33m"),
                          "explicit color enable routes real hexdump palette SGR to redirected semantic stdout");
        result.expectTrue(!contains(stdoutText, "\\x1B[34m") && !contains(stdoutText, "\\x1B[32m") && !contains(stdoutText, "\\x1B[33m"),
                          "colored stdout contains no literal palette clutter");
        result.expectTrue(contains(stdoutText, "│ "), "colored stdout preserves continuation markers");
        result.expectTrue(contains(stripAnsi(stdoutText), ": 00000000  41 00 1b 0a 5a"), "colored stdout strips to readable dump");
        result.expectTrue(contains(fileText, ": 00000000  41 00 1b 0a 5a"), "semantic text file receives plain dump");
        result.expectTrue(!contains(fileText, "\033[") && !contains(fileText, "\\x1B["), "semantic text file remains palette-free");
    }

    {
        const auto stdoutPath = tempPath("snodec-semantic-plain-stdout.log");
        StdoutCapture capture(stdoutPath);
        resetLogger();
        logger::Logger::setDisableColor(true);
        logger::LogManager::setFormat(logger::LogManager::Format::Text);
        auto log = snode::semantic::appLog(logger::Logger::semanticSink(), logger::LogLevel::Trace, fixedClock());
        log.emit(logger::LogLevel::Info, dumpMessage());
        const std::string stdoutText = capture.read();
        result.expectTrue(contains(stdoutText, ": 00000000  41 00 1b 0a 5a"), "explicit color disable emits plain dump");
        result.expectTrue(!contains(stdoutText, "\033[") && !contains(stdoutText, "\\x1B["),
                          "explicit color disable keeps redirected semantic stdout plain");
    }

    {
        const auto stdoutPath = tempPath("snodec-semantic-json-stdout.log");
        StdoutCapture capture(stdoutPath);
        resetLogger();
        logger::Logger::setDisableColor(false);
        logger::LogManager::setFormat(logger::LogManager::Format::Json);
        auto log = snode::semantic::appLog(logger::Logger::semanticSink(), logger::LogLevel::Trace, fixedClock());
        log.emit(logger::LogLevel::Info, dumpMessage());
        const std::string stdoutText = capture.read();
        result.expectTrue(contains(stdoutText, "\"level\":\"info\""), "semantic JSON still uses lowercase JSON level");
        result.expectTrue(contains(stdoutText, "dump:\\n: 00000000"),
                          "semantic JSON stdout serializes the plain message with escaped newlines");
        result.expectTrue(!contains(stdoutText, "\033[") && !contains(stdoutText, "terminalMessage") &&
                              !contains(stdoutText, "\\u001b[34m"),
                          "semantic JSON stdout excludes terminal presentation");
    }

    {
        const auto filePath = tempPath("snodec-semantic-json-file.log");
        resetLogger();
        logger::Logger::setDisableColor(false);
        logger::Logger::logToFile(filePath.string());
        logger::Logger::setQuiet(true);
        logger::LogManager::setFormat(logger::LogManager::Format::Json);
        auto log = snode::semantic::appLog(logger::Logger::semanticSink(), logger::LogLevel::Trace, fixedClock());
        log.emit(logger::LogLevel::Info, dumpMessage());
        logger::Logger::disableLogToFile();
        const std::string fileText = readFile(filePath);
        result.expectTrue(contains(fileText, "\"level\":\"info\""), "semantic JSON file contains lowercase JSON level");
        result.expectTrue(contains(fileText, "dump:\\n: 00000000"),
                          "semantic JSON file serializes the plain message with escaped newlines");
        result.expectTrue(!contains(fileText, "\033[") && !contains(fileText, "terminalMessage") && !contains(fileText, "\\u001b[34m"),
                          "semantic JSON file excludes terminal presentation");
    }

    {
        const auto stdoutPath = tempPath("snodec-semantic-untrusted-esc-stdout.log");
        StdoutCapture capture(stdoutPath);
        resetLogger();
        logger::Logger::setDisableColor(false);
        logger::LogManager::setFormat(logger::LogManager::Format::Text);
        auto log = snode::semantic::appLog(logger::Logger::semanticSink(), logger::LogLevel::Trace, fixedClock());
        log.info(std::string("ordinary ") + static_cast<char>(0x1B) + "[2J");
        const std::string stdoutText = capture.read();
        result.expectTrue(contains(stdoutText, "\\x1B[2J"), "ordinary untrusted ESC is printable escaped with color enabled");
        result.expectTrue(!contains(stdoutText, std::string("\033[2J")), "ordinary untrusted ESC[2J is not emitted raw");
    }

    {
        const auto stdoutPath = tempPath("snodec-semantic-invalid-sidecar-stdout.log");
        StdoutCapture capture(stdoutPath);
        resetLogger();
        logger::Logger::setDisableColor(false);
        logger::LogManager::setFormat(logger::LogManager::Format::Text);
        auto log = snode::semantic::appLog(logger::Logger::semanticSink(), logger::LogLevel::Trace, fixedClock());
        log.emit(logger::LogLevel::Info, logger::PresentedMessage{.plain = "text", .terminal = "\033[32mtext"});
        const std::string stdoutText = capture.read();
        result.expectTrue(contains(stdoutText, "text"), "invalid presented message routes plain fallback");
        result.expectTrue(!contains(stdoutText, "\033[32mtext"), "invalid presented message does not route invalid sidecar SGR");
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
