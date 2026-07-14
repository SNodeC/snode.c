#include "core/eventreceiver/AcceptEventReceiver.h"
#include "core/eventreceiver/ConnectEventReceiver.h"
#include "core/socket/Socket.hpp"
#include "core/socket/SocketAddress.h"
#include "core/socket/stream/SocketAcceptor.h"
#include "core/socket/stream/SocketClient.h"
#include "core/socket/stream/SocketConnection.h"
#include "core/socket/stream/SocketConnector.h"
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
            : SocketConnection(7, 7, instanceName, nullptr) {
        }
        ~TestSocketConnection() override = default;

        int getFd() const override {
            return 7;
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
            return "test-address";
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
} // namespace

int main() {
    tests::support::TestResult result;

    static_assert(std::is_same_v<decltype(std::declval<const TestSocketServer&>().log()), logger::BoundaryLogger>,
                  "SocketServer exposes log() returning BoundaryLogger");
    static_assert(std::is_same_v<decltype(std::declval<const TestSocketClient&>().log()), logger::BoundaryLogger>,
                  "SocketClient exposes log() returning BoundaryLogger");
    static_assert(std::is_same_v<decltype(std::declval<const TestCommonSocketConnector&>().log()), logger::BoundaryLogger>,
                  "SocketConnector exposes log() returning BoundaryLogger");
    static_assert(std::is_same_v<decltype(std::declval<const TestCommonSocketAcceptor&>().log()), logger::BoundaryLogger>,
                  "SocketAcceptor exposes log() returning BoundaryLogger");

    TestSocketServer server("round5-server");
    std::vector<logger::LogRecord> serverRecords;
    server
        .log(
            [&](logger::LogRecord record) {
                serverRecords.push_back(std::move(record));
            },
            logger::LogLevel::Trace,
            fixedTimestamp)
        .info("server endpoint ready");
    result.expectEqual(1, static_cast<int>(serverRecords.size()), "SocketServer log emits one record");
    result.expectTrue(serverRecords[0].origin == logger::LogOrigin::Framework, "SocketServer log uses framework origin");
    result.expectTrue(serverRecords[0].boundary == logger::LogBoundary::Instance, "SocketServer log uses instance boundary");
    result.expectTrue(serverRecords[0].component == "core.socket.stream", "SocketServer log uses stream component");
    result.expectTrue(serverRecords[0].instance && *serverRecords[0].instance == "round5-server",
                      "SocketServer log owns instance identity");
    result.expectTrue(serverRecords[0].role && *serverRecords[0].role == logger::LogRole::Server, "SocketServer log uses server role");
    result.expectTrue(!serverRecords[0].connection, "SocketServer log leaves connection absent");

    TestSocketClient client("round5-client");
    std::vector<logger::LogRecord> clientRecords;
    client
        .log(
            [&](logger::LogRecord record) {
                clientRecords.push_back(std::move(record));
            },
            logger::LogLevel::Trace,
            fixedTimestamp)
        .info("client endpoint ready");
    result.expectEqual(1, static_cast<int>(clientRecords.size()), "SocketClient log emits one record");
    result.expectTrue(clientRecords[0].origin == logger::LogOrigin::Framework, "SocketClient log uses framework origin");
    result.expectTrue(clientRecords[0].boundary == logger::LogBoundary::Instance, "SocketClient log uses instance boundary");
    result.expectTrue(clientRecords[0].component == "core.socket.stream", "SocketClient log uses stream component");
    result.expectTrue(clientRecords[0].instance && *clientRecords[0].instance == "round5-client",
                      "SocketClient log owns instance identity");
    result.expectTrue(clientRecords[0].role && *clientRecords[0].role == logger::LogRole::Client, "SocketClient log uses client role");
    result.expectTrue(!clientRecords[0].connection, "SocketClient log leaves connection absent");

    return result.processResult();
}
