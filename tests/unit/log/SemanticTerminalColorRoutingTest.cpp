#include "Log.h"
#include "SemanticLog.h"
#include "log/LogScopeOwner.h"
#include "log/Logger.h"
#include "tests/support/TestResult.h"
#include "utils/hexdump.h"

#include <cctype>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <optional>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>
#include <unistd.h>
#include <utility>
#include <vector>

namespace {
    logger::BoundaryLogger makeLogger(logger::LogScope scope,
                                      logger::BoundaryLogger::Sink sink,
                                      logger::LogLevel threshold = logger::LogLevel::Trace,
                                      logger::BoundaryLogger::Clock clock = {}) {
        return logger::LogScopeOwner::fromScope(scope).logger(std::move(sink), threshold, std::move(clock));
    }

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

    class StderrCapture {
    public:
        explicit StderrCapture(std::filesystem::path path)
            : path(std::move(path))
            , saved(::dup(STDERR_FILENO)) {
            file = ::fopen(this->path.c_str(), "w+");
            ::dup2(::fileno(file), STDERR_FILENO);
        }

        ~StderrCapture() {
            ::fflush(stderr);
            ::dup2(saved, STDERR_FILENO);
            ::close(saved);
            ::fclose(file);
        }

        std::string read() const {
            ::fflush(stderr);
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
        const auto stderrPath = tempPath("snodec-semantic-default-redirected-stderr.log");
        StderrCapture capture(stderrPath);
        logger::Logger::init();
        logger::LogManager::init();
        logger::Logger::setLogLevel(6);
        logger::Logger::setQuiet(false);
        logger::LogManager::setFormat(logger::LogManager::Format::Text);
        auto log = snode::semantic::appLog(logger::Logger::semanticSink(), logger::LogLevel::Trace, fixedClock());
        log.emit(logger::LogLevel::Info, dumpMessage());
        const std::string stderrText = capture.read();
        result.expectTrue(contains(stderrText, "00000000  41 00 1b 0a 5a"), "default redirected semantic stderr emits plain dump");
        result.expectTrue(!contains(stderrText, "\033["), "non-TTY redirected semantic stderr defaults to plain dump");
        result.expectTrue(!contains(stderrText, "\\x1B["), "non-TTY redirected semantic stderr contains no literal color clutter");
    }

    {
        const auto stderrPath = tempPath("snodec-semantic-color-stderr.log");
        const auto filePath = tempPath("snodec-semantic-color-file.log");
        StderrCapture capture(stderrPath);
        resetLogger();
        logger::Logger::setDisableColor(false);
        logger::Logger::logToFile(filePath.string());
        logger::LogManager::setFormat(logger::LogManager::Format::Text);
        auto log = snode::semantic::appLog(logger::Logger::semanticSink(), logger::LogLevel::Trace, fixedClock());
        log.emit(logger::LogLevel::Info, dumpMessage());
        const std::string stderrText = capture.read();
        logger::Logger::disableLogToFile();
        const std::string fileText = readFile(filePath);
        result.expectTrue(contains(stderrText, "\033[34m") && contains(stderrText, "\033[32m") && contains(stderrText, "\033[33m"),
                          "explicit color enable routes real hexdump palette SGR to redirected semantic stderr");
        result.expectTrue(!contains(stderrText, "\\x1B[34m") && !contains(stderrText, "\\x1B[32m") && !contains(stderrText, "\\x1B[33m"),
                          "colored stderr contains no literal palette clutter");
        result.expectTrue(contains(stderrText, "│ "), "colored stderr preserves continuation markers");
        result.expectTrue(contains(stripAnsi(stderrText), "00000000  41 00 1b 0a 5a"), "colored stderr strips to readable dump");
        result.expectTrue(contains(fileText, "00000000  41 00 1b 0a 5a"), "semantic text file receives plain dump");
        result.expectTrue(!contains(fileText, "\033[") && !contains(fileText, "\\x1B["), "semantic text file remains palette-free");
    }

    {
        const auto stderrPath = tempPath("snodec-semantic-plain-stderr.log");
        StderrCapture capture(stderrPath);
        resetLogger();
        logger::Logger::setDisableColor(true);
        logger::LogManager::setFormat(logger::LogManager::Format::Text);
        auto log = snode::semantic::appLog(logger::Logger::semanticSink(), logger::LogLevel::Trace, fixedClock());
        log.emit(logger::LogLevel::Info, dumpMessage());
        const std::string stderrText = capture.read();
        result.expectTrue(contains(stderrText, "00000000  41 00 1b 0a 5a"), "explicit color disable emits plain dump");
        result.expectTrue(!contains(stderrText, "\033[") && !contains(stderrText, "\\x1B["),
                          "explicit color disable keeps redirected semantic stderr plain");
    }

    {
        const auto stderrPath = tempPath("snodec-semantic-json-stderr.log");
        StderrCapture capture(stderrPath);
        resetLogger();
        logger::Logger::setDisableColor(false);
        logger::LogManager::setFormat(logger::LogManager::Format::Json);
        auto log = snode::semantic::appLog(logger::Logger::semanticSink(), logger::LogLevel::Trace, fixedClock());
        log.emit(logger::LogLevel::Info, dumpMessage());
        const std::string stderrText = capture.read();
        result.expectTrue(contains(stderrText, "\"level\":\"info\""), "semantic JSON still uses lowercase JSON level");
        result.expectTrue(contains(stderrText, "dump:\\n00000000"),
                          "semantic JSON stderr serializes the plain message with escaped newlines");
        result.expectTrue(!contains(stderrText, "\033[") && !contains(stderrText, "terminalMessage") &&
                              !contains(stderrText, "\\u001b[34m"),
                          "semantic JSON stderr excludes terminal presentation");
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
        result.expectTrue(contains(fileText, "dump:\\n00000000"), "semantic JSON file serializes the plain message with escaped newlines");
        result.expectTrue(!contains(fileText, "\033[") && !contains(fileText, "terminalMessage") && !contains(fileText, "\\u001b[34m"),
                          "semantic JSON file excludes terminal presentation");
    }

    {
        const auto stderrPath = tempPath("snodec-semantic-untrusted-esc-stderr.log");
        StderrCapture capture(stderrPath);
        resetLogger();
        logger::Logger::setDisableColor(false);
        logger::LogManager::setFormat(logger::LogManager::Format::Text);
        auto log = snode::semantic::appLog(logger::Logger::semanticSink(), logger::LogLevel::Trace, fixedClock());
        log.info(std::string("ordinary ") + static_cast<char>(0x1B) + "[2J");
        const std::string stderrText = capture.read();
        result.expectTrue(contains(stderrText, "\\x1B[2J"), "ordinary untrusted ESC is printable escaped with color enabled");
        result.expectTrue(!contains(stderrText, std::string("\033[2J")), "ordinary untrusted ESC[2J is not emitted raw");
    }

    {
        const auto stderrPath = tempPath("snodec-semantic-invalid-sidecar-stderr.log");
        StderrCapture capture(stderrPath);
        resetLogger();
        logger::Logger::setDisableColor(false);
        logger::LogManager::setFormat(logger::LogManager::Format::Text);
        auto log = snode::semantic::appLog(logger::Logger::semanticSink(), logger::LogLevel::Trace, fixedClock());
        log.emit(logger::LogLevel::Info, logger::PresentedMessage{.plain = "text", .terminal = "\033[32mtext"});
        const std::string stderrText = capture.read();
        result.expectTrue(contains(stderrText, "text"), "invalid presented message routes plain fallback");
        result.expectTrue(!contains(stderrText, "\033[32mtext"), "invalid presented message does not route invalid sidecar SGR");
    }

    {
        const auto stderrPath = tempPath("snodec-semantic-quiet-stderr.log");
        const auto filePath = tempPath("snodec-semantic-quiet-file.log");
        StderrCapture capture(stderrPath);
        resetLogger();
        logger::Logger::setDisableColor(false);
        logger::Logger::setQuiet(true);
        logger::Logger::logToFile(filePath.string());
        logger::LogManager::setFormat(logger::LogManager::Format::Text);
        auto log = snode::semantic::appLog(logger::Logger::semanticSink(), logger::LogLevel::Trace, fixedClock());
        log.info("quiet file");
        result.expectTrue(capture.read().empty(), "quiet mode suppresses semantic stderr");
        logger::Logger::disableLogToFile();
        result.expectTrue(contains(readFile(filePath), "quiet file"), "quiet mode still writes semantic file output");
    }

    resetLogger();
    for (const bool color : {false, true}) {
        for (const auto format : {logger::LogManager::Format::Text, logger::LogManager::Format::Json}) {
            const auto stderrPath = tempPath("snodec-hexdump-api-stderr.log");
            const auto filePath = tempPath("snodec-hexdump-api-file.log");
            StderrCapture capture(stderrPath);
            resetLogger();
            logger::Logger::setDisableColor(!color);
            logger::Logger::logToFile(filePath.string());
            logger::LogManager::setFormat(format);
            auto log = snode::log::application("hex-api", {.instance = "peer", .role = snode::log::Role::Client, .connection = "7"});
            const std::string payload("A\0\x1b\nZ", 5);
            log.hexDump(snode::log::Level::Info, "Binary", std::string_view(payload));
            log.hexDump(snode::log::Level::Info, "Bytes", std::as_bytes(std::span(payload)));
            log.hexDump(snode::log::Level::Info, "Empty", std::string_view{});
            const std::string output = capture.read();
            logger::Logger::disableLogToFile();
            const std::string file = readFile(filePath);
            result.expectTrue(stripAnsi(output) == file, "new API sends equivalent plain and terminal content");
            result.expectTrue(contains(file, "Binary (5 bytes)") && contains(file, "Bytes (5 bytes)") &&
                                  contains(file, "Empty (0 bytes)") && contains(file, "41 00 1b 0a 5a"),
                              "both public overloads preserve binary bytes and explicit empty input");
            result.expectTrue(contains(file, "hex-api") && contains(file, "peer") && contains(file, "client"), "scope is preserved");
            result.expectTrue(!contains(file, "\033[") && !contains(file, "\\u001b"), "files and JSON exclude palette sequences");
            result.expectTrue(contains(output, "\033[32m") == (color && format == logger::LogManager::Format::Text),
                              "hex colors follow existing output policy");
        }
    }
    {
        resetLogger();
        logger::Logger::setDisableColor(true);
        std::vector<logger::LogRecord> records;
        auto log = makeLogger(
            {logger::LogOrigin::Application, logger::LogBoundary::Application, "hex-api", {}, logger::LogRole::Unknown, {}},
            [&](logger::LogRecord record) {
                records.push_back(std::move(record));
            },
            logger::LogLevel::Info);
        log.hexDump(logger::LogLevel::Trace, "Suppressed", "payload");
        result.expectTrue(records.empty(), "disabled hex dump does not emit");
        log.hexDump(logger::LogLevel::Info, "Plain", "payload");
        result.expectTrue(records.size() == 1 && (!records[0].terminalMessage || records[0].terminalMessage->empty()),
                          "plain-only output does not construct a colored dump");
    }
    return result.processResult();
}
