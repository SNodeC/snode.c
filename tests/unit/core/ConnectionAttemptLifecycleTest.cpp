#include "core/EventReceiver.h"
#include "core/SNodeC.h"
#include "core/eventreceiver/ConnectEventReceiver.h"
#include "core/socket/Socket.hpp"
#include "core/socket/SocketAddress.h"
#include "core/socket/stream/SocketClient.h"
#include "core/socket/stream/SocketConnection.h"
#include "core/socket/stream/SocketConnector.h"
#include "core/socket/stream/SocketConnector.hpp"
#include "core/socket/stream/SocketContextFactory.h"
#include "log/Logger.h"
#include "net/config/ConfigInstance.h"
#include "tests/support/SemanticLogCapture.h"
#include "tests/support/TestResult.h"
#include "utils/Timeval.h"

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace {
    class SNodeCGuard {
    public:
        SNodeCGuard() {
            argv[0] = arg0;
            argv[1] = quiet;
            argv[2] = nullptr;
            core::SNodeC::init(2, argv);
        }

        ~SNodeCGuard() {
            core::SNodeC::free();
        }

    private:
        char arg0[31] = "ConnectionAttemptLifecycleTest";
        char quiet[8] = "--quiet";
        char* argv[3]{};
    };

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

        explicit TestConfigInstance(const std::string& instanceName, Role role = Role::CLIENT)
            : ConfigInstance(instanceName, role) {
        }

        int getSocketOptions() const {
            return 0;
        }

        utils::Timeval getConnectTimeout() const {
            return {};
        }

        TestConfigInstance* setRetry(bool enabled = true) {
            retry = enabled;
            return this;
        }

        bool getRetry() const {
            return retry;
        }

        TestConfigInstance* setRetryOnFatal(bool enabled = true) {
            retryOnFatal = enabled;
            return this;
        }

        bool getRetryOnFatal() const {
            return retryOnFatal;
        }

        TestConfigInstance* setRetryTimeout(double value) {
            retryTimeout = value;
            return this;
        }

        double getRetryTimeout() const {
            return retryTimeout;
        }

        TestConfigInstance* setRetryTries(unsigned int value = 0) {
            retryTries = value;
            return this;
        }

        unsigned int getRetryTries() const {
            return retryTries;
        }

        TestConfigInstance* setRetryBase(double value) {
            retryBase = value;
            return this;
        }

        double getRetryBase() const {
            return retryBase;
        }

        TestConfigInstance* setRetryLimit(double value) {
            retryLimit = value;
            return this;
        }

        double getRetryLimit() const {
            return retryLimit;
        }

        TestConfigInstance* setRetryJitter(double value) {
            retryJitter = value;
            return this;
        }

        double getRetryJitter() const {
            return retryJitter;
        }

        TestConfigInstance* setReconnect(bool enabled = true) {
            reconnect = enabled;
            return this;
        }

        bool getReconnect() const {
            return reconnect;
        }

        TestConfigInstance* setReconnectTime(double value) {
            reconnectTime = value;
            return this;
        }

        double getReconnectTime() const {
            return reconnectTime;
        }

    private:
        bool retry{false};
        bool retryOnFatal{false};
        double retryTimeout{1};
        unsigned int retryTries{0};
        double retryBase{1.8};
        double retryLimit{0};
        double retryJitter{0};
        bool reconnect{false};
        double reconnectTime{1};
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
    class TestConnectorSocketConnection {
    public:
        using Self = TestConnectorSocketConnection<PhysicalSocket, Config>;

        TestConnectorSocketConnection(PhysicalSocket&&,
                                      const std::function<void(Self*)>&,
                                      std::uint64_t,
                                      const std::shared_ptr<Config>&) {
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
        : public core::socket::stream::SocketConnector<TestPhysicalSocket, TestConfigInstance, TestConnectorSocketConnection> {
    private:
        using Super = core::socket::stream::SocketConnector<TestPhysicalSocket, TestConfigInstance, TestConnectorSocketConnection>;

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

namespace {
    class TestEndpointSocketConnection {
    public:
        TestEndpointSocketConnection(int fd, std::uint64_t connectionId, std::string instanceName)
            : fd(fd)
            , connectionId(connectionId)
            , connectionName("[" + std::to_string(fd) + "] " + std::move(instanceName)) {
        }

        int getFd() const noexcept {
            return fd;
        }

        std::uint64_t getConnectionId() const noexcept {
            return connectionId;
        }

        const std::string& getConnectionName() const noexcept {
            return connectionName;
        }

        const TestSocketAddress& getLocalAddress() const {
            return address;
        }

        const TestSocketAddress& getRemoteAddress() const {
            return address;
        }

        std::string getOnlineSince() const {
            return "connection-attempt-online-since";
        }

        double getOnlineDuration() const {
            return 1.0;
        }

        std::size_t getTotalQueued() const {
            return 0;
        }

        std::size_t getTotalSent() const {
            return 0;
        }

        std::size_t getTotalRead() const {
            return 0;
        }

        std::size_t getTotalProcessed() const {
            return 0;
        }

    private:
        int fd;
        std::uint64_t connectionId;
        std::string connectionName;
        TestSocketAddress address;
    };

    class TestSocketContextFactory : public core::socket::stream::SocketContextFactory {
    public:
        core::socket::stream::SocketContext* create(core::socket::stream::SocketConnection*) override {
            return nullptr;
        }
    };

    enum class ClientAction { TerminalError, ConnectedThenDisconnect, ConnectedStop };

    class TestLifecycleConnector : public core::eventreceiver::ConnectEventReceiver {
    public:
        using Config = TestConfigInstance;
        using SocketAddress = TestSocketAddress;
        using SocketConnection = TestEndpointSocketConnection;

        TestLifecycleConnector(const std::shared_ptr<core::socket::stream::SocketContextFactory>&,
                               const std::function<void(SocketConnection*)>& onConnect,
                               const std::function<void(SocketConnection*)>& onConnected,
                               const std::function<void(SocketConnection*)>& onDisconnect,
                               const std::function<void(core::eventreceiver::ConnectEventReceiver*)>&,
                               const std::function<void(const SocketAddress&, core::socket::State)>& onStatus,
                               const std::function<std::uint64_t()>& allocateConnectionId,
                               const std::shared_ptr<Config>& config)
            : core::eventreceiver::ConnectEventReceiver(config->getInstanceName() + " TestLifecycleConnector", 0) {
            liveConnectors.push_back(this);
            ++attemptCount;

            const ClientAction action = nextAction();
            if (action == ClientAction::TerminalError) {
                onStatus(address, core::socket::State(core::socket::State::ERROR, __FILE__, __LINE__));
                return;
            }

            onStatus(address, core::socket::State(core::socket::State::OK, __FILE__, __LINE__));
            connections.push_back(
                std::make_unique<SocketConnection>(11, allocateConnectionId(), config->getInstanceName()));
            SocketConnection* connection = connections.back().get();
            if (onConnect) {
                onConnect(connection);
            }
            if (onConnected) {
                onConnected(connection);
            }

            if (action == ClientAction::ConnectedThenDisconnect) {
                if (onDisconnect) {
                    onDisconnect(connection);
                }
            } else {
                core::SNodeC::stop();
            }
        }

        static void reset(std::vector<ClientAction> actions) {
            queuedActions = std::move(actions);
            attemptCount = 0;
        }

        static std::size_t attempts() noexcept {
            return attemptCount;
        }

        static void cleanup() {
            connections.clear();
            for (TestLifecycleConnector* connector : liveConnectors) {
                delete connector;
            }
            liveConnectors.clear();
            queuedActions.clear();
            attemptCount = 0;
        }

    private:
        void connectEvent() override {
        }

        void unobservedEvent() override {
        }

        static ClientAction nextAction() {
            if (queuedActions.empty()) {
                return ClientAction::ConnectedStop;
            }
            const ClientAction action = queuedActions.front();
            queuedActions.erase(queuedActions.begin());
            return action;
        }

        TestSocketAddress address;

        static std::vector<ClientAction> queuedActions;
        static std::vector<std::unique_ptr<SocketConnection>> connections;
        static std::vector<TestLifecycleConnector*> liveConnectors;
        static std::size_t attemptCount;
    };

    std::vector<ClientAction> TestLifecycleConnector::queuedActions;
    std::vector<std::unique_ptr<TestLifecycleConnector::SocketConnection>> TestLifecycleConnector::connections;
    std::vector<TestLifecycleConnector*> TestLifecycleConnector::liveConnectors;
    std::size_t TestLifecycleConnector::attemptCount = 0;

    using TestSocketClient = core::socket::stream::SocketClient<TestLifecycleConnector, TestSocketContextFactory>;

    void runLoopOnce() {
        core::SNodeC::start(utils::Timeval(0));
    }

    int runConnectorTerminalTest() {
        tests::support::TestResult result;
        tests::support::SemanticLogCapture capture("snodec-connection-attempt-lifecycle");

        logger::Logger::setLogLevel(6);
        logger::LogManager::setGlobalLevel(logger::LogLevel::Debug);
        logger::LogManager::setFormat(logger::LogManager::Format::Json);
        logger::LogManager::freeze();

        auto config = std::make_shared<TestConfigInstance>("timeout-client");
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

    int runExplicitConnectAfterFailureTest() {
        tests::support::TestResult result;
        SNodeCGuard snodeGuard;
        TestLifecycleConnector::reset({ClientAction::TerminalError, ClientAction::ConnectedStop});

        TestSocketClient client("explicit-after-failure");
        client.getConfig()->setRetry(false)->setReconnect(false);

        std::vector<std::uint64_t> connectionIds;
        client.setOnConnected([&](TestEndpointSocketConnection* connection) {
            connectionIds.push_back(connection->getConnectionId());
        });

        int failedStatuses = 0;
        std::function<void(const TestSocketAddress&, core::socket::State)> onStatus;
        onStatus = [&](const TestSocketAddress&, core::socket::State state) {
            if (state == core::socket::State::ERROR && failedStatuses++ == 0) {
                core::EventReceiver::atNextTick([&client, &onStatus]() {
                    client.connect(onStatus);
                });
            }
        };

        client.connect(onStatus);
        runLoopOnce();

        result.expectEqual(2, static_cast<int>(TestLifecycleConnector::attempts()),
                           "the same SocketClient starts a second explicit attempt after terminal failure");
        result.expectEqual(1, failedStatuses, "the first explicit attempt reports one terminal error");
        result.expectTrue(connectionIds.size() == 1 && connectionIds.front() == 1,
                          "the second explicit attempt establishes the first connection on the shared context");
        result.expectEqual(0, static_cast<int>(client.getFlowController()->getRetryCount()),
                           "an explicit later connect is not reported as an automatic retry");
        result.expectEqual(0, static_cast<int>(client.getFlowController()->getReconnectCount()),
                           "an explicit later connect is not reported as an automatic reconnect");

        TestLifecycleConnector::cleanup();
        return result.processResult();
    }

    int runExplicitConnectAfterDisconnectTest() {
        tests::support::TestResult result;
        SNodeCGuard snodeGuard;
        TestLifecycleConnector::reset({ClientAction::ConnectedThenDisconnect, ClientAction::ConnectedStop});

        TestSocketClient client("explicit-after-disconnect");
        client.getConfig()->setRetry(false)->setReconnect(false);

        std::vector<std::uint64_t> connectionIds;
        client.setOnConnected([&](TestEndpointSocketConnection* connection) {
            connectionIds.push_back(connection->getConnectionId());
        });

        const std::function<void(const TestSocketAddress&, core::socket::State)> onStatus =
            [](const TestSocketAddress&, core::socket::State) {};
        int disconnects = 0;
        client.setOnDisconnect([&](TestEndpointSocketConnection*) {
            if (disconnects++ == 0) {
                core::EventReceiver::atNextTick([&client, &onStatus]() {
                    client.connect(onStatus);
                });
            }
        });

        client.connect(onStatus);
        runLoopOnce();

        result.expectEqual(2, static_cast<int>(TestLifecycleConnector::attempts()),
                           "the same SocketClient starts a second explicit attempt after disconnect");
        result.expectEqual(1, disconnects, "the first established connection disconnects exactly once");
        result.expectTrue(connectionIds.size() == 2 && connectionIds[0] == 1 && connectionIds[1] == 2,
                          "explicit reconnect preserves the shared monotonically increasing connection sequence");
        result.expectEqual(0, static_cast<int>(client.getFlowController()->getReconnectCount()),
                           "manual reuse of connect is distinct from configured automatic reconnect");

        TestLifecycleConnector::cleanup();
        return result.processResult();
    }

    int runStaleConnectCallbackTest() {
        tests::support::TestResult result;
        SNodeCGuard snodeGuard;
        TestLifecycleConnector::reset({ClientAction::TerminalError, ClientAction::ConnectedStop});

        TestSocketClient client("stale-connect-callback");
        client.getConfig()->setRetry(false)->setReconnect(false);

        int staleSuccesses = 0;
        int restartedSuccesses = 0;
        int terminations = 0;
        const std::function<void(const TestSocketAddress&, core::socket::State)> staleStatus =
            [&](const TestSocketAddress&, core::socket::State state) {
                staleSuccesses += state == core::socket::State::OK ? 1 : 0;
            };
        const std::function<void(const TestSocketAddress&, core::socket::State)> restartedStatus =
            [&](const TestSocketAddress&, core::socket::State state) {
                restartedSuccesses += state == core::socket::State::OK ? 1 : 0;
            };

        client.getFlowController()->setOnFlowTerminated([&](core::socket::stream::ClientFlowController*) {
            if (terminations++ == 0) {
                client.connect(restartedStatus);
            }
        });

        client.connect([](const TestSocketAddress&, core::socket::State) {});
        client.connect(staleStatus);
        runLoopOnce();

        result.expectEqual(2, static_cast<int>(TestLifecycleConnector::attempts()),
                           "an explicit restart invalidates an older queued connect callback");
        result.expectEqual(0, staleSuccesses, "the stale callback cannot claim the restarted flow");
        result.expectEqual(1, restartedSuccesses, "the explicitly restarted callback establishes the connection");

        TestLifecycleConnector::cleanup();
        return result.processResult();
    }

    int runOutOfScopeReconnectTest() {
        tests::support::TestResult result;
        SNodeCGuard snodeGuard;
        TestLifecycleConnector::reset({ClientAction::ConnectedThenDisconnect, ClientAction::ConnectedStop});

        std::vector<std::uint64_t> connectionIds;
        {
            TestSocketClient client("out-of-scope-reconnect");
            client.getConfig()->setRetry(false)->setReconnect(true)->setReconnectTime(0);
            client.setOnConnected([&](TestEndpointSocketConnection* connection) {
                connectionIds.push_back(connection->getConnectionId());
            });
            client.connect([](const TestSocketAddress&, core::socket::State) {});
        }

        runLoopOnce();

        result.expectEqual(2, static_cast<int>(TestLifecycleConnector::attempts()),
                           "the shared client flow remains alive after the SocketClient handle leaves scope");
        result.expectTrue(connectionIds.size() == 2 && connectionIds[0] == 1 && connectionIds[1] == 2,
                          "configured reconnect completes after the original SocketClient handle is destroyed");

        TestLifecycleConnector::cleanup();
        return result.processResult();
    }
} // namespace

int main(int argc, char** argv) {
    if (argc == 1) {
        const std::vector<std::string> scenarios = {
            "connector-terminal",
            "explicit-after-failure",
            "explicit-after-disconnect",
            "stale-connect-callback",
            "out-of-scope-reconnect",
        };

        tests::support::TestResult result;
        for (const std::string& scenario : scenarios) {
            const std::string command = std::string(argv[0]) + " " + scenario;
            result.expectEqual(0, std::system(command.c_str()), "connection lifecycle scenario " + scenario + " passes");
        }
        return result.processResult();
    }

    const std::string scenario = argv[1];
    if (scenario == "connector-terminal") {
        return runConnectorTerminalTest();
    }
    if (scenario == "explicit-after-failure") {
        return runExplicitConnectAfterFailureTest();
    }
    if (scenario == "explicit-after-disconnect") {
        return runExplicitConnectAfterDisconnectTest();
    }
    if (scenario == "stale-connect-callback") {
        return runStaleConnectCallbackTest();
    }
    if (scenario == "out-of-scope-reconnect") {
        return runOutOfScopeReconnectTest();
    }

    return EXIT_FAILURE;
}
