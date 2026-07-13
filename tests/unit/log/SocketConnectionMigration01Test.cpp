#include "core/socket/SocketAddress.h"
#include "core/socket/stream/SocketConnection.h"
#include "log/Logger.h"
#include "log/SemanticLogger.h"
#include "tests/support/TestResult.h"

#include <cerrno>
#include <filesystem>
#include <fstream>
#include <ostream>
#include <string>
#include <system_error>
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
                return std::string("MIGRATION01TICK");
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

    class TestSocketConnection : public core::socket::stream::SocketConnection {
    public:
        explicit TestSocketConnection(const std::string& instanceName)
            : SocketConnection(11, instanceName, nullptr) {
        }
        ~TestSocketConnection() override = default;

        int getFd() const override {
            return 11;
        }
        void sendToPeer(const char*, std::size_t) override {
        }
        bool streamToPeer(core::pipe::Source*) override {
            return false;
        }
        void streamEof() override {
        }
        std::size_t readFromPeer(char*, std::size_t) override {
            return 0;
        }
        void shutdownRead() override {
        }
        void shutdownWrite() override {
        }
        const core::socket::SocketAddress& getBindAddress() const override {
            return unusableAddress();
        }
        const core::socket::SocketAddress& getLocalAddress() const override {
            return unusableAddress();
        }
        const core::socket::SocketAddress& getRemoteAddress() const override {
            return unusableAddress();
        }
        void close() override {
        }
        void setTimeout(const utils::Timeval&) override {
        }
        void setReadTimeout(const utils::Timeval&) override {
        }
        void setWriteTimeout(const utils::Timeval&) override {
        }
        std::size_t getTotalSent() const override {
            return 0;
        }
        std::size_t getTotalQueued() const override {
            return 0;
        }
        std::size_t getTotalRead() const override {
            return 0;
        }
        std::size_t getTotalProcessed() const override {
            return 0;
        }

    private:
        static const core::socket::SocketAddress& unusableAddress() {
            return *static_cast<const core::socket::SocketAddress*>(nullptr);
        }
    };
} // namespace

int main() {
    tests::support::TestResult result;

    const auto enabledPath = tempLogPath("snodec-migration01-enabled.log");
    {
        LoggerStateGuard guard(enabledPath.string());
        logger::LogManager::setGlobalLevel(logger::LogLevel::Debug);
        logger::LogManager::freeze();
        TestSocketConnection connection("migration01-instance");
        connection.log().debug("SocketConnection: switch completed");
    }
    const auto enabledLog = readFile(enabledPath);
    result.expectTrue(enabledLog.find("SocketConnection: switch completed") != std::string::npos,
                      "migrated non-error SocketConnection.hpp call emits semantically when enabled");
    result.expectTrue(enabledLog.find(" framework/connection core.socket.stream ") != std::string::npos,
                      "migrated call carries framework/connection core.socket.stream scope");

    const auto managerFilterPath = tempLogPath("snodec-migration01-manager-filter.log");
    {
        LoggerStateGuard guard(managerFilterPath.string());
        logger::LogManager::setGlobalLevel(logger::LogLevel::Error);
        logger::LogManager::freeze();
        TestSocketConnection connection("migration01-instance");
        connection.log().debug("SocketConnection: switch completed");
    }
    result.expectTrue(readFile(managerFilterPath).find("SocketConnection: switch completed") == std::string::npos,
                      "migrated call respects LogManager filtering");

    const auto backendFilterPath = tempLogPath("snodec-migration01-backend-filter.log");
    {
        LoggerStateGuard guard(backendFilterPath.string());
        logger::LogManager::setGlobalLevel(logger::LogLevel::Trace);
        logger::LogManager::freeze();
        logger::Logger::setLogLevel(3);
        TestSocketConnection connection("migration01-instance");
        connection.log().debug("SocketConnection: switch completed");
    }
    result.expectTrue(readFile(backendFilterPath).find("SocketConnection: switch completed") != std::string::npos,
                      "migrated semantic call is not double-gated by Logger::setLogLevel");

    const auto jsonPath = tempLogPath("snodec-migration01-json.log");
    {
        LoggerStateGuard guard(jsonPath.string());
        logger::LogManager::setGlobalLevel(logger::LogLevel::Debug);
        logger::LogManager::setFormat(logger::LogManager::Format::Json);
        logger::LogManager::freeze();
        TestSocketConnection connection("migration01-instance");
        connection.log().debug("SocketConnection: switch completed");
    }
    const auto jsonLog = readFile(jsonPath);
    result.expectTrue(jsonLog.find("{\"v\":1") != std::string::npos &&
                          jsonLog.find("\"message\":\"SocketConnection: switch completed\"") != std::string::npos,
                      "migrated call respects JSON format selection");

    std::vector<logger::LogRecord> sysErrorRecords;
    TestSocketConnection sysErrorConnection("migration01-instance");
    sysErrorConnection
        .log(
            [&](logger::LogRecord record) {
                sysErrorRecords.push_back(std::move(record));
            },
            logger::LogLevel::Trace)
        .sysError(logger::LogLevel::Error, EACCES, "Shutdown (RD)");
    result.expectTrue(sysErrorRecords.size() == 1 && sysErrorRecords[0].message == "Shutdown (RD)" && sysErrorRecords[0].error &&
                          sysErrorRecords[0].error->code == EACCES &&
                          sysErrorRecords[0].error->text.find("Permission") != std::string::npos,
                      "migrated PLOG/sysError conversion preserves errno code/text");

    std::vector<logger::LogRecord> sinkRecords;
    TestSocketConnection sinkConnection("migration01-sink");
    sinkConnection
        .log([&](logger::LogRecord record) {
            sinkRecords.push_back(std::move(record));
        })
        .debug("sink overload still works");
    result.expectTrue(sinkRecords.size() == 1 && sinkRecords[0].message == "sink overload still works",
                      "sink-taking overload remains compatible");

    const auto suppressedPath = tempLogPath("snodec-migration01-suppressed.log");
    {
        LoggerStateGuard guard(suppressedPath.string());
        logger::LogManager::setGlobalLevel(logger::LogLevel::Error);
        logger::LogManager::freeze();

        TestSocketConnection connection("migration01-instance");
        int evaluations = 0;
        connection.log().debug("{}", ExpensiveValue{&evaluations});

        result.expectEqual(0, evaluations, "suppressed production SocketConnection formatting is not evaluated after PR #151");
    }
    result.expectTrue(readFile(suppressedPath).empty(), "suppressed production SocketConnection formatting emits no backend output");

    return result.processResult();
}
