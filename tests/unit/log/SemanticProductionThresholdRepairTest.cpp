#include "core/socket/SocketAddress.h"
#include "core/socket/stream/SocketConnection.h"
#include "core/socket/stream/SocketContext.h"
#include "log/Logger.h"
#include "log/SemanticLogger.h"
#include "tests/support/TestResult.h"

#include <cerrno>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <ostream>
#include <string>
#include <system_error>
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
                return std::string("REPAIRTICK");
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

    logger::LogScope testScope() {
        return {logger::LogOrigin::Framework,
                logger::LogBoundary::System,
                "repair.test",
                "repair-instance",
                logger::LogRole::Server,
                "repair-connection"};
    }

    std::chrono::system_clock::time_point fixedTimestamp() {
        using namespace std::chrono;
        return system_clock::time_point{seconds{1783254896} + milliseconds{789}};
    }

    class TestSocketConnection : public core::socket::stream::SocketConnection {
    public:
        explicit TestSocketConnection(const std::string& instanceName)
            : SocketConnection(9, instanceName, nullptr) {
        }
        ~TestSocketConnection() override = default;

        int getFd() const override {
            return 9;
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

    class TestSocketContext : public core::socket::stream::SocketContext {
    public:
        explicit TestSocketContext(core::socket::stream::SocketConnection* socketConnection)
            : SocketContext(socketConnection) {
        }
        ~TestSocketContext() override = default;

    private:
        std::size_t onReceivedFromPeer() override {
            return 0;
        }
        bool onSignal(int) override {
            return false;
        }
        void onConnected() override {
        }
        void onDisconnected() override {
        }
    };
} // namespace

int main() {
    tests::support::TestResult result;

    const auto suppressedConnectionPath = tempLogPath("snodec-semantic-repair-connection-suppressed.log");
    {
        LoggerStateGuard guard(suppressedConnectionPath.string());
        logger::LogManager::setGlobalLevel(logger::LogLevel::Error);
        logger::LogManager::freeze();
        TestSocketConnection connection("repair-instance");
        int evaluations = 0;
        connection.log().debug("{}", ExpensiveValue{&evaluations});
        result.expectEqual(0, evaluations, "production SocketConnection::log debug formatting is skipped when globally suppressed");
    }
    result.expectTrue(readFile(suppressedConnectionPath).empty(),
                      "production SocketConnection::log suppressed debug emits no backend output");

    const auto enabledConnectionPath = tempLogPath("snodec-semantic-repair-connection-enabled.log");
    {
        LoggerStateGuard guard(enabledConnectionPath.string());
        logger::LogManager::setGlobalLevel(logger::LogLevel::Debug);
        logger::LogManager::freeze();
        TestSocketConnection connection("repair-instance");
        int evaluations = 0;
        connection.log().debug("{}", ExpensiveValue{&evaluations});
        result.expectEqual(1, evaluations, "production SocketConnection::log debug formatting runs when enabled");
    }
    const auto enabledConnectionLog = readFile(enabledConnectionPath);
    result.expectTrue(enabledConnectionLog.find("expensive") != std::string::npos,
                      "enabled SocketConnection::log writes formatted message");
    result.expectTrue(enabledConnectionLog.find(" framework/connection core.socket.stream ") != std::string::npos,
                      "enabled SocketConnection::log preserves framework/connection scope");

    const auto suppressedContextPath = tempLogPath("snodec-semantic-repair-context-suppressed.log");
    {
        LoggerStateGuard guard(suppressedContextPath.string());
        logger::LogManager::setGlobalLevel(logger::LogLevel::Error);
        logger::LogManager::freeze();
        TestSocketConnection connection("repair-instance");
        TestSocketContext context(&connection);
        int evaluations = 0;
        context.frameworkLog().debug("{}", ExpensiveValue{&evaluations});
        result.expectEqual(0, evaluations, "production SocketContext::frameworkLog debug formatting is skipped when globally suppressed");
    }
    result.expectTrue(readFile(suppressedContextPath).empty(),
                      "production SocketContext::frameworkLog suppressed debug emits no backend output");

    int disabledSysErrorEvaluations = 0;
    int disabledSysErrorSinkCalls = 0;
    auto disabledSysErrorLogger = logger::BoundaryLogger::createForTest(
        testScope(),
        [&](logger::LogRecord) {
            ++disabledSysErrorSinkCalls;
        },
        logger::LogLevel::Error,
        fixedTimestamp);
    disabledSysErrorLogger.sysError(logger::LogLevel::Debug, EACCES, "{}", ExpensiveValue{&disabledSysErrorEvaluations});
    result.expectEqual(0, disabledSysErrorEvaluations, "disabled int sysError does not format message");
    result.expectEqual(0, disabledSysErrorSinkCalls, "disabled int sysError does not call sink");

    std::vector<logger::LogRecord> sysErrorRecords;
    auto enabledSysErrorLogger = logger::BoundaryLogger::createForTest(
        testScope(),
        [&](logger::LogRecord record) {
            sysErrorRecords.push_back(std::move(record));
        },
        logger::LogLevel::Debug,
        fixedTimestamp);
    int enabledSysErrorEvaluations = 0;
    enabledSysErrorLogger.sysError(logger::LogLevel::Debug, EACCES, "{}", ExpensiveValue{&enabledSysErrorEvaluations});
    result.expectEqual(1, enabledSysErrorEvaluations, "enabled int sysError formats message");
    result.expectTrue(sysErrorRecords.size() == 1 && sysErrorRecords[0].message == "expensive" && sysErrorRecords[0].error &&
                          sysErrorRecords[0].error->code == EACCES &&
                          sysErrorRecords[0].error->text.find("Permission") != std::string::npos,
                      "enabled int sysError preserves message, errno code, and errno text");

    std::vector<logger::LogRecord> explicitThresholdRecords;
    logger::LogManager::init();
    logger::LogManager::setGlobalLevel(logger::LogLevel::Error);
    logger::LogManager::freeze();
    TestSocketConnection explicitConnection("repair-instance");
    explicitConnection
        .log(
            [&](logger::LogRecord record) {
                explicitThresholdRecords.push_back(std::move(record));
            },
            logger::LogLevel::Trace)
        .debug("explicit threshold still emits");
    result.expectTrue(explicitThresholdRecords.size() == 1 && explicitThresholdRecords[0].message == "explicit threshold still emits",
                      "explicit custom-sink threshold overload remains independent of LogManager global level");
    logger::LogManager::init();

    return result.processResult();
}
