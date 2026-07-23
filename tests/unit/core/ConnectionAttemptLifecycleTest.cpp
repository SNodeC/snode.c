#include "core/socket/SocketAddress.h"
#include "core/socket/stream/SocketConnector.h"
#include "core/socket/stream/SocketConnector.hpp"
#include "log/Logger.h"
#include "net/config/ConfigInstance.h"
#include "tests/support/SemanticLogCapture.h"
#include "tests/support/TestResult.h"
#include "utils/Timeval.h"

#include <functional>
#include <memory>
#include <string>

namespace {
    class TestSocketAddress : public core::socket::SocketAddress {
    public:
        std::string toString(bool = true) const override {
            return "connection-attempt-address";
        }
    };

    struct TestAddressConfig {
        TestSocketAddress getSocketAddress() const {
            return {};
        }
    };

    class TestConfigInstance
        : public net::config::ConfigInstance
        , public TestAddressConfig {
    public:
        using Local = TestAddressConfig;
        using Remote = TestAddressConfig;

        explicit TestConfigInstance(const std::string& instanceName, Role role)
            : ConfigInstance(instanceName, role) {
        }

        int getSocketOptions() const {
            return 0;
        }

        utils::Timeval getConnectTimeout() const {
            return {};
        }
    };

    class TestPhysicalSocket {
    public:
        using SocketAddress = TestSocketAddress;

        enum class Flags { NONBLOCK };

        int open(int, Flags) {
            return -1;
        }

        int bind(const SocketAddress&) {
            return -1;
        }

        int connect(const SocketAddress&) {
            return -1;
        }

        static bool connectInProgress(int) {
            return false;
        }

        int getFd() const {
            return -1;
        }

        int getSockError(int& socketError) const {
            socketError = 0;
            return 0;
        }

        SocketAddress getBindAddress() const {
            return {};
        }
    };

    template <typename PhysicalSocket, typename Config>
    class TestSocketConnection {
    public:
        using Self = TestSocketConnection<PhysicalSocket, Config>;

        TestSocketConnection(PhysicalSocket&&, const std::function<void(Self*)>&, std::uint64_t, const std::shared_ptr<Config>&) {
        }

        logger::BoundaryLogger log() const {
            return logger::LogScopeOwner(logger::LogOrigin::Framework, logger::LogBoundary::Connection, "core.socket.stream")
                .logger(logger::Logger::semanticSink());
        }

        TestSocketAddress getLocalAddress() const {
            return {};
        }

        TestSocketAddress getRemoteAddress() const {
            return {};
        }
    };

    class TestAttemptConnector
        : public core::socket::stream::SocketConnector<TestPhysicalSocket, TestConfigInstance, TestSocketConnection> {
    private:
        using Super = core::socket::stream::SocketConnector<TestPhysicalSocket, TestConfigInstance, TestSocketConnection>;

    public:
        explicit TestAttemptConnector(const std::shared_ptr<TestConfigInstance>& config)
            : Super({}, {}, {}, {}, {}, {}, config) {
        }

    private:
        void useNextSocketAddress() override {
        }
    };
} // namespace

namespace core::socket::stream {
    struct SocketConnectorLifecycleTestAccess {
        template <typename Connector>
        static void start(Connector& connector) {
            connector.startAttempt();
        }

        template <typename Connector>
        static void finish(Connector& connector, const char* outcome) {
            connector.finishAttempt(outcome);
        }
    };
} // namespace core::socket::stream

int main() {
    tests::support::TestResult result;
    tests::support::SemanticLogCapture capture("snodec-connection-attempt-lifecycle");

    logger::Logger::setLogLevel(6);
    logger::LogManager::setGlobalLevel(logger::LogLevel::Debug);
    logger::LogManager::setFormat(logger::LogManager::Format::Json);
    logger::LogManager::freeze();

    auto config = std::make_shared<TestConfigInstance>("timeout-client", TestConfigInstance::Role::CLIENT);
    TestAttemptConnector connector(config);
    core::socket::stream::SocketConnectorLifecycleTestAccess::start(connector);
    core::socket::stream::SocketConnectorLifecycleTestAccess::finish(connector, "connection attempt timed out");
    core::socket::stream::SocketConnectorLifecycleTestAccess::finish(connector, "connection attempt cancelled");

    const auto records = capture.finish();
    int attemptStarted = 0;
    int attemptTimedOut = 0;
    int attemptCancelled = 0;
    for (const auto& record : records) {
        const std::string message = record.value("message", "");
        if (message == "connection attempt started" || message == "connection attempt timed out") {
            result.expectTrue(record.value("level", "") == "debug" && record.value("origin", "") == "framework" &&
                                  record.value("boundary", "") == "instance" &&
                                  record.value("component", "") == "core.socket.stream" &&
                                  record.value("instance", "") == "timeout-client" && record.value("role", "") == "client" &&
                                  !record.contains("connection"),
                              message + " carries client endpoint identity at Debug");
        }
        attemptStarted += message == "connection attempt started" ? 1 : 0;
        attemptTimedOut += message == "connection attempt timed out" ? 1 : 0;
        attemptCancelled += message == "connection attempt cancelled" ? 1 : 0;
    }

    result.expectEqual(1, attemptStarted, "timeout scenario emits one attempt start");
    result.expectEqual(1, attemptTimedOut, "timeout scenario emits exactly one terminal timeout");
    result.expectEqual(0, attemptCancelled, "terminal timeout suppresses later cancellation");
    return result.processResult();
}
