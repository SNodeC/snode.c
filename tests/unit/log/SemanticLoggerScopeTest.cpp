#include "core/eventreceiver/AcceptEventReceiver.h"
#include "core/eventreceiver/ConnectEventReceiver.h"
#include "core/socket/Socket.hpp"
#include "core/socket/SocketAddress.h"
#include "core/socket/stream/SocketAcceptor.h"
#include "core/socket/stream/SocketClient.h"
#include "core/socket/stream/SocketConnection.h"
#include "core/socket/stream/SocketConnector.h"
#include "core/socket/stream/SocketContext.h"
#include "core/socket/stream/SocketContextFactory.h"
#include "core/socket/stream/SocketServer.h"
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
            : ConfigInstance(instanceName, Role::SERVER) {
        }
        ~TestConfigInstance() override = default;
    };

    class TestSocketConnection : public core::socket::stream::SocketConnection {
    public:
        explicit TestSocketConnection(const std::string& instanceName)
            : SocketConnection(11, 23, instanceName, nullptr) {
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

    class TestSocketContextFactory : public core::socket::stream::SocketContextFactory {
    public:
        core::socket::stream::SocketContext* create(core::socket::stream::SocketConnection*) override {
            return nullptr;
        }
    };

    class TestSocketAcceptor : public core::eventreceiver::AcceptEventReceiver {
    public:
        using Config = TestConfigInstance;
        using SocketConnection = TestSocketConnection;
        using SocketAddress = core::socket::SocketAddress;
    };

    class TestSocketConnector : public core::eventreceiver::ConnectEventReceiver {
    public:
        using Config = TestConfigInstance;
        using SocketConnection = TestSocketConnection;
        using SocketAddress = core::socket::SocketAddress;
    };

    class TestSocketAddress : public core::socket::SocketAddress {
    public:
        std::string toString(bool = true) const override {
            return "scope-address";
        }
    };

    class TestPhysicalSocket {
    public:
        using SocketAddress = TestSocketAddress;
    };

    template <typename PhysicalSocket, typename Config>
    class TestTemplatedSocketConnection;

    using TestCommonSocketAcceptor =
        core::socket::stream::SocketAcceptor<TestPhysicalSocket, TestConfigInstance, TestTemplatedSocketConnection>;
    using TestCommonSocketConnector =
        core::socket::stream::SocketConnector<TestPhysicalSocket, TestConfigInstance, TestTemplatedSocketConnection>;
    using TestSocketServer = core::socket::stream::SocketServer<TestSocketAcceptor, TestSocketContextFactory>;
    using TestSocketClient = core::socket::stream::SocketClient<TestSocketConnector, TestSocketContextFactory>;

    class TestConnectorScope : public TestCommonSocketConnector {
    public:
        static logger::BoundaryLogger loggerFor(logger::BoundaryLogger::Sink sink) {
            return makeLogScope("scope-connector").logger(std::move(sink), logger::LogLevel::Trace, fixedTimestamp);
        }
    };

    class TestAcceptorScope : public TestCommonSocketAcceptor {
    public:
        static logger::BoundaryLogger loggerFor(logger::BoundaryLogger::Sink sink) {
            return makeLogScope("scope-acceptor").logger(std::move(sink), logger::LogLevel::Trace, fixedTimestamp);
        }
    };
} // namespace

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
    static_assert(std::is_same_v<decltype(std::declval<const TestSocketServer&>().log()), logger::BoundaryLogger>,
                  "SocketServer exposes log() returning BoundaryLogger");
    static_assert(std::is_same_v<decltype(std::declval<const TestSocketClient&>().log()), logger::BoundaryLogger>,
                  "SocketClient exposes log() returning BoundaryLogger");
    static_assert(std::is_same_v<decltype(std::declval<const TestCommonSocketConnector&>().log()), logger::BoundaryLogger>,
                  "SocketConnector exposes log() returning BoundaryLogger");
    static_assert(std::is_same_v<decltype(std::declval<const TestCommonSocketAcceptor&>().log()), logger::BoundaryLogger>,
                  "SocketAcceptor exposes log() returning BoundaryLogger");

    TestConfigInstance config("scope-instance");
    std::vector<logger::LogRecord> configRecords;
    config
        .log(
            [&](logger::LogRecord record) {
                configRecords.push_back(std::move(record));
            },
            logger::LogLevel::Trace,
            fixedTimestamp)
        .info("config ready");
    result.expectEqual(1, static_cast<int>(configRecords.size()), "ConfigInstance log emits one record");
    result.expectTrue(configRecords[0].origin == logger::LogOrigin::Framework, "ConfigInstance log uses framework origin");
    result.expectTrue(configRecords[0].boundary == logger::LogBoundary::Configuration, "ConfigInstance log uses configuration boundary");
    result.expectTrue(configRecords[0].component == "configuration", "ConfigInstance log uses configuration component");
    result.expectTrue(configRecords[0].instance && *configRecords[0].instance == "scope-instance",
                      "ConfigInstance log owns instance identity");
    result.expectTrue(configRecords[0].role && *configRecords[0].role == logger::LogRole::Server, "ConfigInstance log maps server role");
    result.expectTrue(!configRecords[0].connection, "ConfigInstance log leaves connection absent");

    TestSocketConnection connection("scope-instance");
    std::vector<logger::LogRecord> connectionRecords;
    connection
        .log(
            [&](logger::LogRecord record) {
                connectionRecords.push_back(std::move(record));
            },
            logger::LogLevel::Trace,
            fixedTimestamp)
        .info("connection ready");
    result.expectEqual(1, static_cast<int>(connectionRecords.size()), "SocketConnection log emits one record");
    result.expectTrue(connectionRecords[0].origin == logger::LogOrigin::Framework, "SocketConnection log uses framework origin");
    result.expectTrue(connectionRecords[0].boundary == logger::LogBoundary::Connection, "SocketConnection log uses connection boundary");
    result.expectTrue(connectionRecords[0].component == "core.socket.stream", "SocketConnection log uses stream component");
    result.expectTrue(connectionRecords[0].instance && *connectionRecords[0].instance == "scope-instance",
                      "SocketConnection log owns instance identity");
    result.expectEqual(11, connection.getFd(), "identity fixture uses fd 11");
    result.expectEqual(static_cast<std::uint64_t>(23), connection.getConnectionId(), "identity fixture uses connection ID 23");
    result.expectTrue(connection.getConnectionName() == "[11] scope-instance",
                      "display connection name remains distinct from semantic identity");
    result.expectTrue(connectionRecords[0].connection && *connectionRecords[0].connection == "23",
                      "SocketConnection uses getConnectionId rather than fd or display name");

    TestSocketContext context(&connection);
    std::vector<logger::LogRecord> contextRecords;
    context
        .log(
            [&](logger::LogRecord record) {
                contextRecords.push_back(std::move(record));
            },
            logger::LogLevel::Trace,
            fixedTimestamp)
        .info("application context ready");
    context
        .frameworkLog(
            [&](logger::LogRecord record) {
                contextRecords.push_back(std::move(record));
            },
            logger::LogLevel::Trace,
            fixedTimestamp)
        .info("framework context ready");
    result.expectEqual(2, static_cast<int>(contextRecords.size()), "SocketContext log APIs emit two records");
    result.expectTrue(contextRecords[0].origin == logger::LogOrigin::Application, "SocketContext log() uses application origin");
    result.expectTrue(contextRecords[1].origin == logger::LogOrigin::Framework, "SocketContext frameworkLog() uses framework origin");
    result.expectTrue(contextRecords[0].boundary == logger::LogBoundary::Context, "SocketContext log() uses context boundary");
    result.expectTrue(contextRecords[1].boundary == logger::LogBoundary::Context, "SocketContext frameworkLog() uses context boundary");
    result.expectTrue(contextRecords[0].component == "core.socket.context" && contextRecords[1].component == "core.socket.context",
                      "SocketContext logs use context component");
    result.expectTrue(contextRecords[0].instance && *contextRecords[0].instance == "scope-instance",
                      "SocketContext log owns instance identity");
    result.expectTrue(contextRecords[0].connection && *contextRecords[0].connection == "23",
                      "SocketContext log owns connection identity");

    TestSocketConnection anonymousConnection("<anonymous>");
    std::vector<logger::LogRecord> anonymousRecords;
    anonymousConnection
        .log(
            [&](logger::LogRecord record) {
                anonymousRecords.push_back(std::move(record));
            },
            logger::LogLevel::Trace,
            fixedTimestamp)
        .info("anonymous identity");
    result.expectTrue(anonymousRecords.size() == 1 && anonymousRecords[0].instance == "<anonymous>" &&
                          anonymousRecords[0].connection == "23",
                      "anonymous instance remains represented with numeric connection identity");

    TestSocketServer server("scope-server");
    TestSocketClient client("scope-client");
    std::vector<logger::LogRecord> endpointRecords;
    server
        .log(
            [&](logger::LogRecord record) {
                endpointRecords.push_back(std::move(record));
            },
            logger::LogLevel::Trace,
            fixedTimestamp)
        .info("server endpoint");
    client
        .log(
            [&](logger::LogRecord record) {
                endpointRecords.push_back(std::move(record));
            },
            logger::LogLevel::Trace,
            fixedTimestamp)
        .info("client endpoint");
    TestConnectorScope::loggerFor([&](logger::LogRecord record) {
        endpointRecords.push_back(std::move(record));
    }).info("connector endpoint");
    TestAcceptorScope::loggerFor([&](logger::LogRecord record) {
        endpointRecords.push_back(std::move(record));
    }).info("acceptor endpoint");

    result.expectEqual(4, static_cast<int>(endpointRecords.size()), "all four endpoint owners emit one structured record");
    const auto expectEndpoint = [&](std::size_t index,
                                    std::string_view instance,
                                    logger::LogRole role,
                                    const std::string& label) {
        const auto& record = endpointRecords[index];
        result.expectTrue(record.origin == logger::LogOrigin::Framework && record.boundary == logger::LogBoundary::Instance &&
                              record.component == "core.socket.stream" && record.instance && *record.instance == instance &&
                              record.role && *record.role == role && !record.connection,
                          label + " carries framework/instance component, role, and instance without connection identity");
    };
    expectEndpoint(0, "scope-server", logger::LogRole::Server, "SocketServer");
    expectEndpoint(1, "scope-client", logger::LogRole::Client, "SocketClient");
    expectEndpoint(2, "scope-connector", logger::LogRole::Client, "SocketConnector");
    expectEndpoint(3, "scope-acceptor", logger::LogRole::Server, "SocketAcceptor");

    return result.processResult();
}
