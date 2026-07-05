#include "core/socket/SocketAddress.h"
#include "core/socket/stream/SocketConnection.h"
#include "core/socket/stream/SocketContext.h"
#include "log/SemanticLogger.h"
#include "net/config/ConfigInstance.h"
#include "tests/support/TestResult.h"

#include <chrono>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace {
    std::chrono::system_clock::time_point fixedTimestamp() {
        using namespace std::chrono;
        return system_clock::time_point{seconds{1783254896} + milliseconds{789}};
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

    static_assert(std::is_same_v<decltype(std::declval<const TestConfigInstance&>().log()), logger::BoundaryLogger>,
                  "ConfigInstance exposes log() returning BoundaryLogger");
    static_assert(std::is_same_v<decltype(std::declval<const TestSocketConnection&>().log()), logger::BoundaryLogger>,
                  "SocketConnection exposes log() returning BoundaryLogger");
    static_assert(std::is_same_v<decltype(std::declval<const TestSocketContext&>().log()), logger::BoundaryLogger>,
                  "SocketContext exposes log() returning BoundaryLogger");
    static_assert(std::is_same_v<decltype(std::declval<const TestSocketContext&>().frameworkLog()), logger::BoundaryLogger>,
                  "SocketContext exposes frameworkLog() returning BoundaryLogger");

    TestConfigInstance config("round4-instance");
    std::vector<logger::LogRecord> configRecords;
    config.log([&](logger::LogRecord record) { configRecords.push_back(std::move(record)); }, logger::LogLevel::Trace, fixedTimestamp).info("config ready");
    result.expectEqual(1, static_cast<int>(configRecords.size()), "ConfigInstance log emits one record");
    result.expectTrue(configRecords[0].origin == logger::LogOrigin::Framework, "ConfigInstance log uses framework origin");
    result.expectTrue(configRecords[0].boundary == logger::LogBoundary::Configuration, "ConfigInstance log uses configuration boundary");
    result.expectTrue(configRecords[0].component == "configuration", "ConfigInstance log uses configuration component");
    result.expectTrue(configRecords[0].instance && *configRecords[0].instance == "round4-instance", "ConfigInstance log owns instance identity");
    result.expectTrue(configRecords[0].role && *configRecords[0].role == logger::LogRole::Server, "ConfigInstance log maps server role");
    result.expectTrue(!configRecords[0].connection, "ConfigInstance log leaves connection absent");

    TestSocketConnection connection("round4-instance");
    std::vector<logger::LogRecord> connectionRecords;
    connection.log([&](logger::LogRecord record) { connectionRecords.push_back(std::move(record)); }, logger::LogLevel::Trace, fixedTimestamp).info("connection ready");
    result.expectEqual(1, static_cast<int>(connectionRecords.size()), "SocketConnection log emits one record");
    result.expectTrue(connectionRecords[0].origin == logger::LogOrigin::Framework, "SocketConnection log uses framework origin");
    result.expectTrue(connectionRecords[0].boundary == logger::LogBoundary::Connection, "SocketConnection log uses connection boundary");
    result.expectTrue(connectionRecords[0].component == "core.socket.stream", "SocketConnection log uses stream component");
    result.expectTrue(connectionRecords[0].instance && *connectionRecords[0].instance == "round4-instance", "SocketConnection log owns instance identity");
    result.expectTrue(connectionRecords[0].connection && *connectionRecords[0].connection == "[7] round4-instance", "SocketConnection log owns connection identity");

    TestSocketContext context(&connection);
    std::vector<logger::LogRecord> contextRecords;
    context.log([&](logger::LogRecord record) { contextRecords.push_back(std::move(record)); }, logger::LogLevel::Trace, fixedTimestamp).info("application context ready");
    context.frameworkLog([&](logger::LogRecord record) { contextRecords.push_back(std::move(record)); }, logger::LogLevel::Trace, fixedTimestamp).info("framework context ready");
    result.expectEqual(2, static_cast<int>(contextRecords.size()), "SocketContext log APIs emit two records");
    result.expectTrue(contextRecords[0].origin == logger::LogOrigin::Application, "SocketContext log() uses application origin");
    result.expectTrue(contextRecords[1].origin == logger::LogOrigin::Framework, "SocketContext frameworkLog() uses framework origin");
    result.expectTrue(contextRecords[0].boundary == logger::LogBoundary::Context, "SocketContext log() uses context boundary");
    result.expectTrue(contextRecords[1].boundary == logger::LogBoundary::Context, "SocketContext frameworkLog() uses context boundary");
    result.expectTrue(contextRecords[0].component == "core.socket.context" && contextRecords[1].component == "core.socket.context",
                      "SocketContext logs use context component");
    result.expectTrue(contextRecords[0].instance && *contextRecords[0].instance == "round4-instance", "SocketContext log owns instance identity");
    result.expectTrue(contextRecords[0].connection && *contextRecords[0].connection == "[7] round4-instance", "SocketContext log owns connection identity");

    return result.processResult();
}
