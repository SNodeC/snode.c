#include "core/socket/SocketAddress.h"
#include "core/socket/stream/SocketConnection.h"
#include "core/socket/stream/SocketContext.h"
#include "core/socket/stream/SocketContextFactory.h"
#include "log/Logger.h"
#include "log/SemanticLogger.h"
#include "tests/support/TestResult.h"

#include <cerrno>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>
#include <system_error>
#include <vector>

namespace {
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
                return std::string("ROUND8TICK000");
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
            : SocketConnection(8, instanceName, nullptr) {
        }
        ~TestSocketConnection() override = default;

        using core::socket::stream::SocketConnection::setSocketContext;

        int getFd() const override {
            return 8;
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
            shutdownReadCalled = true;
        }
        void shutdownWrite() override {
            shutdownWriteCalled = true;
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
            closeCalled = true;
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

        bool shutdownReadCalled = false;
        bool shutdownWriteCalled = false;
        bool closeCalled = false;

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

        void exposeReadError(int errnum) {
            onReadError(errnum);
        }
        void exposeWriteError(int errnum) {
            onWriteError(errnum);
        }

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

    const auto connectionPath = tempLogPath("snodec-round8-connection.log");
    {
        LoggerStateGuard guard(connectionPath.string());
        logger::LogManager::setGlobalLevel(logger::LogLevel::Debug);
        logger::LogManager::freeze();
        TestSocketConnection connection("round8-instance");
        auto* first = new TestSocketContext(&connection);
        auto* second = new TestSocketContext(&connection);
        connection.setSocketContext(first);
        connection.setSocketContext(second);
        delete first;
        delete second;
    }
    const auto connectionLog = readFile(connectionPath);
    result.expectTrue(connectionLog.find("SocketContext: switch") != std::string::npos,
                      "migrated SocketConnection call emits through semantic backend");
    result.expectTrue(connectionLog.find("framework/connection core.socket.stream ") != std::string::npos,
                      "migrated SocketConnection call carries framework connection scope");

    const auto contextPath = tempLogPath("snodec-round8-context.log");
    {
        LoggerStateGuard guard(contextPath.string());
        logger::LogManager::setGlobalLevel(logger::LogLevel::Debug);
        logger::LogManager::freeze();
        TestSocketConnection connection("round8-instance");
        TestSocketContext context(&connection);
        context.exposeReadError(0);
    }
    const auto contextLog = readFile(contextPath);
    result.expectTrue(contextLog.find("SocketContext: EOF received") != std::string::npos,
                      "migrated SocketContext call emits through semantic backend");
    result.expectTrue(contextLog.find("framework/context core.socket.context ") != std::string::npos,
                      "migrated SocketContext framework-internal call emits with framework origin");

    const auto filterPath = tempLogPath("snodec-round8-filter.log");
    {
        LoggerStateGuard guard(filterPath.string());
        logger::LogManager::setGlobalLevel(logger::LogLevel::Error);
        logger::LogManager::freeze();
        TestSocketConnection connection("round8-instance");
        TestSocketContext context(&connection);
        context.exposeReadError(0);
    }
    result.expectTrue(readFile(filterPath).find("SocketContext: EOF received") == std::string::npos,
                      "migrated call respects LogManager filtering");

    const auto gatePath = tempLogPath("snodec-round8-backend-gate.log");
    {
        LoggerStateGuard guard(gatePath.string());
        logger::LogManager::setGlobalLevel(logger::LogLevel::Trace);
        logger::LogManager::freeze();
        logger::Logger::setLogLevel(3);
        TestSocketConnection connection("round8-instance");
        TestSocketContext context(&connection);
        context.exposeReadError(0);
    }
    result.expectTrue(readFile(gatePath).find("SocketContext: EOF received") != std::string::npos,
                      "migrated semantic call is not double-gated by Logger::setLogLevel");

    const auto jsonPath = tempLogPath("snodec-round8-json.log");
    {
        LoggerStateGuard guard(jsonPath.string());
        logger::LogManager::setGlobalLevel(logger::LogLevel::Debug);
        logger::LogManager::setFormat(logger::LogManager::Format::Json);
        logger::LogManager::freeze();
        TestSocketConnection connection("round8-instance");
        TestSocketContext context(&connection);
        context.exposeReadError(0);
    }
    const auto jsonLog = readFile(jsonPath);
    result.expectTrue(jsonLog.find("{\"v\":1") != std::string::npos &&
                          jsonLog.find("\"message\":\"SocketContext: EOF received\"") != std::string::npos,
                      "migrated call respects Json format selection");

    const auto sysErrorPath = tempLogPath("snodec-round8-syserror.log");
    {
        LoggerStateGuard guard(sysErrorPath.string());
        logger::LogManager::setGlobalLevel(logger::LogLevel::Debug);
        logger::LogManager::setFormat(logger::LogManager::Format::Json);
        logger::LogManager::freeze();
        TestSocketConnection connection("round8-instance");
        TestSocketContext context(&connection);
        context.exposeWriteError(EACCES);
    }
    const auto sysErrorLog = readFile(sysErrorPath);
    result.expectTrue(sysErrorLog.find("SocketContext: onWriteError") != std::string::npos &&
                          sysErrorLog.find("\"code\":13") != std::string::npos &&
                          sysErrorLog.find("Permission denied") != std::string::npos,
                      "migrated PLOG-equivalent preserves errno code and text");

    TestSocketConnection sinkConnection("round8-sink");
    std::vector<logger::LogRecord> records;
    sinkConnection
        .log([&](logger::LogRecord record) {
            records.push_back(std::move(record));
        })
        .debug("sink overload still works");
    result.expectEqual(1, static_cast<int>(records.size()), "sink-taking overload remains compatible");
    result.expectTrue(records[0].message == "sink overload still works", "sink-taking overload captures message");

    return result.processResult();
}
