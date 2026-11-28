#include "core/SNodeC.h"
#include "log/Logger.h"
#include "tests/support/TestResult.h"
#include "utils/CLI11.hpp"
#include "utils/Config.h"

#include <cerrno>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <fcntl.h>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <optional>
#include <pty.h>
#include <stdexcept>
#include <string>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <utility>
#include <vector>

namespace {
    class Capture {
    public:
        explicit Capture(bool terminal) {
            if (terminal) {
                if (openpty(&reader, &writer, nullptr, nullptr, nullptr) != 0) {
                    throw std::runtime_error("openpty");
                }
            } else {
                std::string path = (std::filesystem::temp_directory_path() / "console-stream-XXXXXX").string();
                reader = mkstemp(path.data());
                if (reader < 0) {
                    throw std::runtime_error("mkstemp");
                }
                unlink(path.c_str());
                writer = reader;
            }
            fcntl(reader, F_SETFL, O_NONBLOCK);
        }

        ~Capture() {
            if (writer != reader) {
                close(writer);
            }
            close(reader);
        }

        std::string read() const {
            lseek(reader, 0, SEEK_SET);
            std::string value;
            char bytes[4096];
            ssize_t count = 1;
            while (count > 0) {
                count = ::read(reader, bytes, sizeof(bytes));
                if (count > 0) {
                    value.append(bytes, static_cast<std::size_t>(count));
                }
            }
            return value;
        }

        int reader = -1;
        int writer = -1;
    };

    struct Case {
        std::string name;
        std::vector<std::string> arguments;
        bool stderrSelected = false;
        bool stdoutTerminal = false;
        bool stderrTerminal = false;
        bool color = false;
        bool quiet = false;
        bool valid = true;
        bool file = false;
        bool reset = false;
    };

    int child(const Case& current, const std::string& file) {
        std::vector<std::string> arguments{"console-stream-test"};
        arguments.insert(arguments.end(), current.arguments.begin(), current.arguments.end());
        if (current.file) {
            arguments.insert(arguments.end(), {"--enforce-log-file", "--log-file", file});
        }
        std::vector<char*> argv;
        for (std::string& argument : arguments) {
            argv.push_back(argument.data());
        }
        argv.push_back(nullptr);
        core::SNodeC::init(static_cast<int>(arguments.size()), argv.data());
        int result = 1;
        if (utils::Config::bootstrap()) {
            const CLI::Option* option = utils::Config::configRoot.getOption("--log-console");
            result = option != nullptr && option->get_configurable() ? 0 : 2;
            logger::LogRecordOptions options;
            options.ts = std::chrono::system_clock::time_point{};
            const logger::LogScope scope{
                logger::LogOrigin::Application, logger::LogBoundary::Application, "console-stream-test", {}, logger::LogRole::Unknown, {}};
            auto record = logger::materialize(scope, logger::LogLevel::Info, "console-marker", std::move(options));
            record.terminalMessage = "\033[32mconsole-marker\033[0m";
            logger::Logger::emitSemantic(record);
            logger::Logger::startAsync();
            logger::Logger::emitSemantic(record);
            logger::Logger::shutdown();
            logger::Logger::disableLogToFile();
            if (current.reset) {
                logger::Logger::init();
                logger::LogManager::init();
                record.message = "reset-marker";
                record.terminalMessage.reset();
                logger::Logger::emitSemantic(record);
                logger::Logger::shutdown();
            }
        }
        return result;
    }

    int occurrences(const std::string& text, const std::string& value) {
        int count = 0;
        std::size_t position = 0;
        while ((position = text.find(value, position)) != std::string::npos) {
            ++count;
            position += value.size();
        }
        return count;
    }
} // namespace

int main() {
    tests::support::TestResult testResult;
    const std::vector<Case> cases{
        {.name = "default"},
        {.name = "stdout", .arguments = {"--log-console", "stdout"}},
        {.name = "stderr", .arguments = {"--log-console", "stderr"}, .stderrSelected = true},
        {.name = "quiet", .arguments = {"--log-console", "stderr", "--quiet"}, .stderrSelected = true, .quiet = true},
        {.name = "invalid", .arguments = {"--log-console", "other"}, .valid = false},
        {.name = "stderr-tty", .arguments = {"--log-console", "stderr"}, .stderrSelected = true, .stderrTerminal = true, .color = true},
        {.name = "stdout-tty-stderr-pipe", .arguments = {"--log-console", "stderr"}, .stderrSelected = true, .stdoutTerminal = true},
        {.name = "stdout-pipe-stderr-tty", .stderrTerminal = true},
        {.name = "stdout-tty", .stdoutTerminal = true, .color = true},
        {.name = "mono-before",
         .arguments = {"--monochrom=true", "--log-console", "stderr"},
         .stderrSelected = true,
         .stderrTerminal = true},
        {.name = "mono-after",
         .arguments = {"--log-console", "stderr", "--monochrom=true"},
         .stderrSelected = true,
         .stderrTerminal = true},
        {.name = "color-before", .arguments = {"--monochrom=false", "--log-console", "stderr"}, .stderrSelected = true, .color = true},
        {.name = "color-after", .arguments = {"--log-console", "stderr", "--monochrom=false"}, .stderrSelected = true, .color = true},
        {.name = "json", .arguments = {"--log-console", "stderr", "--log-format", "json"}, .stderrSelected = true, .stderrTerminal = true},
        {.name = "file",
         .arguments = {"--log-console", "stderr", "--monochrom=false"},
         .stderrSelected = true,
         .color = true,
         .file = true},
        {.name = "quiet-file", .arguments = {"--log-console", "stderr", "--quiet"}, .stderrSelected = true, .quiet = true, .file = true},
        {.name = "reset", .arguments = {"--log-console", "stderr"}, .stderrSelected = true, .reset = true}};
    for (const Case& current : cases) {
        Capture output(current.stdoutTerminal);
        Capture error(current.stderrTerminal);
        const std::string file =
            (std::filesystem::temp_directory_path() / ("console-stream-" + std::to_string(getpid()) + "-" + current.name + ".log"))
                .string();
        const pid_t pid = fork();
        int status = -1;
        if (pid == 0) {
            dup2(output.writer, STDOUT_FILENO);
            dup2(error.writer, STDERR_FILENO);
            const int result = child(current, file);
            fflush(stdout);
            fflush(stderr);
            _exit(result);
        } else if (pid > 0) {
            pid_t waited = -1;
            do {
                waited = waitpid(pid, &status, 0);
            } while (waited < 0 && errno == EINTR);
        }
        const std::string stdoutText = output.read();
        const std::string stderrText = error.read();
        testResult.expectTrue(WIFEXITED(status) && WEXITSTATUS(status) == (current.valid ? 0 : 1),
                              current.name + ": CLI validation and option lifetime");
        if (current.valid) {
            const std::string& selected = current.stderrSelected ? stderrText : stdoutText;
            const std::string& other = current.stderrSelected ? stdoutText : stderrText;
            testResult.expectEqual(current.quiet ? 0 : 2,
                                   occurrences(selected, "console-marker"),
                                   current.name + ": sync drain and async records use selected stream");
            testResult.expectEqual(0, occurrences(other, "console-marker"), current.name + ": no record reaches other stream");
            testResult.expectTrue((selected.find('\033') != std::string::npos) == (current.color && !current.quiet),
                                  current.name + ": color follows destination and explicit override");
            if (current.name == "json") {
                testResult.expectTrue(selected.find("\"message\":\"console-marker\"") != std::string::npos,
                                      "JSON output keeps the plain message");
            }
            if (current.file) {
                std::ifstream stream(file);
                const std::string text{std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>()};
                testResult.expectEqual(2, occurrences(text, "console-marker"), current.name + ": file receives both records");
                testResult.expectTrue(text.find('\033') == std::string::npos, current.name + ": file remains plain");
            }
            if (current.reset) {
                testResult.expectEqual(1, occurrences(stdoutText, "reset-marker"), "init restores stdout");
                testResult.expectEqual(0, occurrences(stderrText, "reset-marker"), "init stops using stderr");
            }
        }
        if (current.file) {
            std::filesystem::remove(file);
        }
    }
    return testResult.processResult();
}
