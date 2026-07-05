#include "core/socket/SocketAddress.h"
#include "core/socket/stream/SocketConnection.h"
#include "core/socket/stream/SocketContext.h"
#include "log/Logger.h"
#include "log/SemanticLogger.h"
#include "net/config/ConfigInstance.h"
#include "tests/support/TestResult.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

namespace {
    std::chrono::system_clock::time_point fixedTimestamp() {
        using namespace std::chrono;
        return system_clock::time_point{seconds{1783254896} + milliseconds{789}};
    }

    class LoggerStateGuard {
    public:
        explicit LoggerStateGuard(const std::string& logFile) {
            logger::Logger::init();
            logger::Logger::setLogLevel(6);
            logger::Logger::setVerboseLevel(0);
            logger::Logger::setQuiet(true);
            logger::Logger::setDisableColor(true);
            logger::Logger::setTickResolver([]() { return std::string("ROUND6TICK000"); });
            logger::Logger::logToFile(logFile);
        }

        ~LoggerStateGuard() {
            logger::Logger::disableLogToFile();
            logger::Logger::setTickResolver({});
            logger::Logger::init();
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
        return {logger::LogOrigin::Framework, logger::LogBoundary::System, "round6.component", "round6-instance", logger::LogRole::Server, "round6-connection"};
    }

    logger::LogRecord record(logger::LogLevel level, std::string message) {
        logger::LogRecordOptions options;
        options.ts = fixedTimestamp();
        return logger::materialize(testScope(), level, std::move(message), std::move(options));
    }

    class TestConfigInstance : public net::config::ConfigInstance {
    public:
        explicit TestConfigInstance(const std::string& instanceName)
            : ConfigInstance(instanceName, Role::SERVER) {}
        ~TestConfigInstance() override = default;
    };

    class TestSocketConnection : public core::socket::stream::SocketConnection {
    public:
        explicit TestSocketConnection(const std::string& instanceName)
            : SocketConnection(7, instanceName, nullptr) {}
        ~TestSocketConnection() override = default;

        int getFd() const override { return 7; }
        void sendToPeer(const char*, std::size_t) override {}
        bool streamToPeer(core::pipe::Source*) override { return false; }
        void streamEof() override {}
        std::size_t readFromPeer(char*, std::size_t) override { return 0; }
        void shutdownRead() override {}
        void shutdownWrite() override {}
        const core::socket::SocketAddress& getBindAddress() const override { return unusableAddress(); }
        const core::socket::SocketAddress& getLocalAddress() const override { return unusableAddress(); }
        const core::socket::SocketAddress& getRemoteAddress() const override { return unusableAddress(); }
        void close() override {}
        void setTimeout(const utils::Timeval&) override {}
        void setReadTimeout(const utils::Timeval&) override {}
        void setWriteTimeout(const utils::Timeval&) override {}
        std::size_t getTotalSent() const override { return 0; }
        std::size_t getTotalQueued() const override { return 0; }
        std::size_t getTotalRead() const override { return 0; }
        std::size_t getTotalProcessed() const override { return 0; }

    private:
        static const core::socket::SocketAddress& unusableAddress() { return *static_cast<const core::socket::SocketAddress*>(nullptr); }
    };

    class TestSocketContext : public core::socket::stream::SocketContext {
    public:
        explicit TestSocketContext(core::socket::stream::SocketConnection* socketConnection)
            : SocketContext(socketConnection) {}
        ~TestSocketContext() override = default;

    private:
        std::size_t onReceivedFromPeer() override { return 0; }
        bool onSignal(int) override { return false; }
        void onConnected() override {}
        void onDisconnected() override {}
    };
}

int main() {
    tests::support::TestResult result;

    const auto semanticPath = tempLogPath("snodec-round6-semantic.log");
    {
        LoggerStateGuard guard(semanticPath.string());
        logger::Logger::emitSemantic(record(logger::LogLevel::Info, "semantic info emitted"));
        logger::Logger::emitSemantic(record(logger::LogLevel::Off, "semantic off hidden"));
        LOG(INFO) << "legacy macro emitted";
    }
    const auto semanticLog = readFile(semanticPath);
    result.expectTrue(semanticLog.find("INFO") != std::string::npos, "semantic info maps to legacy INFO label");
    result.expectTrue(semanticLog.find("semantic info emitted") != std::string::npos, "semantic info emits through file backend");
    result.expectTrue(semanticLog.find("semantic off hidden") == std::string::npos, "semantic off emits nothing");
    result.expectTrue(semanticLog.find("ROUND6TICK000") != std::string::npos, "semantic emission uses legacy tick resolver pattern");
    result.expectTrue(semanticLog.find("legacy macro emitted") != std::string::npos, "legacy LOG macro still emits through backend");

    const auto filteredPath = tempLogPath("snodec-round6-filtered.log");
    {
        LoggerStateGuard guard(filteredPath.string());
        logger::Logger::setLogLevel(3);
        logger::Logger::emitSemantic(record(logger::LogLevel::Info, "filtered info hidden"));
        logger::Logger::emitSemantic(record(logger::LogLevel::Warn, "warning visible"));
    }
    const auto filteredLog = readFile(filteredPath);
    result.expectTrue(filteredLog.find("filtered info hidden") == std::string::npos, "semantic backend respects Logger::setLogLevel for info");
    result.expectTrue(filteredLog.find("warning visible") != std::string::npos, "semantic backend respects Logger::setLogLevel for warning");

    const auto objectPath = tempLogPath("snodec-round6-objects.log");
    {
        LoggerStateGuard guard(objectPath.string());
        TestConfigInstance config("round6-config");
        config.log().info("config default backend");
        TestSocketConnection connection("round6-connection");
        connection.log().info("connection default backend");
        TestSocketContext context(&connection);
        context.log().info("context default backend");
        context.frameworkLog().info("framework context default backend");
    }
    const auto objectLog = readFile(objectPath);
    result.expectTrue(objectLog.find("config default backend") != std::string::npos, "ConfigInstance no-argument log emits through backend");
    result.expectTrue(objectLog.find("connection default backend") != std::string::npos, "SocketConnection no-argument log emits through backend");
    result.expectTrue(objectLog.find("context default backend") != std::string::npos, "SocketContext no-argument log emits through backend");
    result.expectTrue(objectLog.find("framework context default backend") != std::string::npos, "SocketContext frameworkLog no-argument emits through backend");

    std::vector<logger::LogRecord> captured;
    logger::BoundaryLogger::createForTest(testScope(), [&](logger::LogRecord capturedRecord) { captured.push_back(std::move(capturedRecord)); }, logger::LogLevel::Trace, fixedTimestamp)
        .info("custom sink still captures");
    result.expectEqual(1, static_cast<int>(captured.size()), "sink-taking semantic overloads still capture records");
    result.expectTrue(captured[0].message == "custom sink still captures", "custom sink receives semantic message");

    return result.processResult();
}
