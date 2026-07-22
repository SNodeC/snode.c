#include "core/EventLoop.h"
#include "core/EventReceiver.h"
#include "core/SNodeC.h"
#include "core/eventreceiver/AcceptEventReceiver.h"
#include "core/eventreceiver/ConnectEventReceiver.h"
#include "core/socket/Socket.hpp"
#include "core/socket/SocketAddress.h"
#include "core/socket/stream/SocketClient.h"
#include "core/socket/stream/SocketConnection.h"
#include "core/socket/stream/SocketContextFactory.h"
#include "core/socket/stream/SocketServer.h"
#include "log/Logger.h"
#include "log/SemanticLogger.h"
#include "net/config/ConfigInstance.h"
#include "tests/support/TestResult.h"
#include "utils/Timeval.h"

#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iterator>
#include <memory>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

namespace {
    class LoggerStateGuard {
    public:
        explicit LoggerStateGuard(const std::string& logFile) {
            logger::Logger::init();
            logger::LogManager::init();
            logger::Logger::setLogLevel(6);
            logger::Logger::setVerboseLevel(6);
            logger::Logger::setQuiet(true);
            logger::Logger::setDisableColor(true);
            logger::Logger::setTickResolver([]() {
                return std::string("ENDPOINTLIFETIME");
            });
            logger::LogManager::setGlobalLevel(logger::LogLevel::Trace);
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

    class SNodeCGuard {
    public:
        SNodeCGuard() {
            argv[0] = arg0;
            argv[1] = logLevel;
            argv[2] = quiet;
            argv[3] = nullptr;
            core::SNodeC::init(3, argv);
        }

        ~SNodeCGuard() {
            core::SNodeC::free();
        }

    private:
        char arg0[29] = "EndpointLifetimeCountersTest";
        char logLevel[14] = "--log-level=6";
        char quiet[8] = "--quiet";
        char* argv[4]{};
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

    std::size_t countOccurrences(const std::string& haystack, const std::string& needle) {
        std::size_t count = 0;
        std::size_t pos = 0;
        while ((pos = haystack.find(needle, pos)) != std::string::npos) {
            ++count;
            pos += needle.size();
        }
        return count;
    }

    class TestSocketAddress : public core::socket::SocketAddress {
    public:
        std::string toString(bool = true) const override {
            return "endpoint-lifetime-address";
        }
    };

    class TestSocketConnection {
    public:
        TestSocketConnection(int fd, std::uint64_t connectionId, std::string instanceName)
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
            return "endpoint-lifetime-online-since";
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

    class TestServerConfig : public net::config::ConfigInstance {
    public:
        explicit TestServerConfig(const std::string& instanceName)
            : ConfigInstance(instanceName, Role::SERVER) {
        }
        ~TestServerConfig() override = default;

        TestServerConfig* setRetry(bool enabled = true) {
            retry = enabled;
            return this;
        }
        bool getRetry() const {
            return retry;
        }
        TestServerConfig* setRetryOnFatal(bool enabled = true) {
            retryOnFatal = enabled;
            return this;
        }
        bool getRetryOnFatal() const {
            return retryOnFatal;
        }
        TestServerConfig* setRetryTimeout(double value) {
            retryTimeout = value;
            return this;
        }
        double getRetryTimeout() const {
            return retryTimeout;
        }
        TestServerConfig* setRetryTries(unsigned int value = 0) {
            retryTries = value;
            return this;
        }
        unsigned int getRetryTries() const {
            return retryTries;
        }
        TestServerConfig* setRetryBase(double value) {
            retryBase = value;
            return this;
        }
        double getRetryBase() const {
            return retryBase;
        }
        TestServerConfig* setRetryLimit(unsigned int value) {
            retryLimit = value;
            return this;
        }
        double getRetryLimit() const {
            return retryLimit;
        }
        TestServerConfig* setRetryJitter(double value) {
            retryJitter = value;
            return this;
        }
        double getRetryJitter() const {
            return retryJitter;
        }

    private:
        bool retry{false};
        bool retryOnFatal{false};
        double retryTimeout{1};
        unsigned int retryTries{0};
        double retryBase{1.8};
        double retryLimit{0};
        double retryJitter{0};
    };

    class TestClientConfig : public TestServerConfig {
    public:
        explicit TestClientConfig(const std::string& instanceName)
            : TestServerConfig(instanceName) {
            setInstanceName(instanceName);
        }
        ~TestClientConfig() override = default;

        TestClientConfig* setReconnect(bool enabled = true) {
            reconnect = enabled;
            return this;
        }
        bool getReconnect() const {
            return reconnect;
        }
        TestClientConfig* setReconnectTime(double value) {
            reconnectTime = value;
            return this;
        }
        double getReconnectTime() const {
            return reconnectTime;
        }

    private:
        bool reconnect{false};
        double reconnectTime{1};
    };

    enum class ServerAction {
        TerminalError,
        ErrorThenStopBeforePolicy,
        NoRetryError,
        RetryableError,
        OkStop,
        TwoAcceptedConnectionsStop
    };

    enum class ClientAction {
        TerminalError,
        ErrorThenStopBeforePolicy,
        NoRetryError,
        RetryableError,
        OkStop,
        ConnectedThenDisconnect,
        ConnectedThenDisconnectStopping,
        ConnectedStop
    };

    class TestAcceptEventReceiver : public core::eventreceiver::AcceptEventReceiver {
    public:
        using Config = TestServerConfig;
        using SocketAddress = TestSocketAddress;
        using SocketConnection = TestSocketConnection;

        TestAcceptEventReceiver(const std::shared_ptr<core::socket::stream::SocketContextFactory>&,
                                const std::function<void(SocketConnection*)>& onConnect,
                                const std::function<void(SocketConnection*)>& onConnected,
                                const std::function<void(SocketConnection*)>&,
                                const std::function<void(core::eventreceiver::AcceptEventReceiver*)>&,
                                const std::function<void(const SocketAddress&, core::socket::State)>& onStatus,
                                const std::function<std::uint64_t()>& allocateConnectionId,
                                const std::shared_ptr<Config>& config)
            : core::eventreceiver::AcceptEventReceiver(config->getInstanceName() + " TestAcceptEventReceiver", 0) {
            liveAcceptors.push_back(this);
            const ServerAction action = nextAction();
            if (action == ServerAction::TwoAcceptedConnectionsStop) {
                acceptConnection(11, config->getInstanceName(), allocateConnectionId, onConnect, onConnected);
                acceptConnection(11, config->getInstanceName(), allocateConnectionId, onConnect, onConnected);
                core::SNodeC::stop();
                return;
            }

            if (action == ServerAction::ErrorThenStopBeforePolicy) {
                core::SNodeC::stop();
            }

            core::socket::State status = action == ServerAction::OkStop ? core::socket::State(core::socket::State::OK, __FILE__, __LINE__) : core::socket::State(core::socket::State::ERROR, __FILE__, __LINE__);
            if (action == ServerAction::NoRetryError) {
                status |= core::socket::State::NO_RETRY;
            }
            onStatus(address, status);

            if (action == ServerAction::TerminalError || action == ServerAction::NoRetryError || action == ServerAction::OkStop) {
                core::SNodeC::stop();
            }
        }

        static void reset(std::vector<ServerAction> actions) {
            queuedActions = std::move(actions);
        }

        static void cleanup() {
            acceptedConnections.clear();
            for (TestAcceptEventReceiver* acceptor : liveAcceptors) {
                delete acceptor;
            }
            liveAcceptors.clear();
        }

    private:
        void acceptEvent() override {
        }

        void unobservedEvent() override {
        }

        static ServerAction nextAction() {
            if (queuedActions.empty()) {
                return ServerAction::OkStop;
            }
            const ServerAction action = queuedActions.front();
            queuedActions.erase(queuedActions.begin());
            return action;
        }

        static void acceptConnection(int fd,
                                     const std::string& instanceName,
                                     const std::function<std::uint64_t()>& allocateConnectionId,
                                     const std::function<void(SocketConnection*)>& onConnect,
                                     const std::function<void(SocketConnection*)>& onConnected) {
            acceptedConnections.push_back(std::make_unique<SocketConnection>(fd, allocateConnectionId(), instanceName));
            SocketConnection* connection = acceptedConnections.back().get();
            if (onConnect) {
                onConnect(connection);
            }
            if (onConnected) {
                onConnected(connection);
            }
        }

        TestSocketAddress address;
        static std::vector<ServerAction> queuedActions;
        static std::vector<std::unique_ptr<SocketConnection>> acceptedConnections;
        static std::vector<TestAcceptEventReceiver*> liveAcceptors;
    };

    std::vector<ServerAction> TestAcceptEventReceiver::queuedActions;
    std::vector<std::unique_ptr<TestAcceptEventReceiver::SocketConnection>> TestAcceptEventReceiver::acceptedConnections;
    std::vector<TestAcceptEventReceiver*> TestAcceptEventReceiver::liveAcceptors;

    class TestConnectEventReceiver : public core::eventreceiver::ConnectEventReceiver {
    public:
        using Config = TestClientConfig;
        using SocketAddress = TestSocketAddress;
        using SocketConnection = TestSocketConnection;

        TestConnectEventReceiver(const std::shared_ptr<core::socket::stream::SocketContextFactory>&,
                                 const std::function<void(SocketConnection*)>& onConnect,
                                 const std::function<void(SocketConnection*)>& onConnected,
                                 const std::function<void(SocketConnection*)>& onDisconnect,
                                 const std::function<void(core::eventreceiver::ConnectEventReceiver*)>&,
                                 const std::function<void(const SocketAddress&, core::socket::State)>& onStatus,
                                 const std::function<std::uint64_t()>& allocateConnectionId,
                                 const std::shared_ptr<Config>& config)
            : core::eventreceiver::ConnectEventReceiver(config->getInstanceName() + " TestConnectEventReceiver", 0) {
            liveConnectors.push_back(this);
            const ClientAction action = nextAction();
            if (action == ClientAction::ConnectedThenDisconnect || action == ClientAction::ConnectedThenDisconnectStopping ||
                action == ClientAction::ConnectedStop) {
                connectConnection(11, config->getInstanceName(), allocateConnectionId, onConnect, onConnected, onDisconnect, action);
                if (action == ClientAction::ConnectedStop) {
                    core::SNodeC::stop();
                }
                return;
            }

            if (action == ClientAction::ErrorThenStopBeforePolicy) {
                core::SNodeC::stop();
            }

            core::socket::State status = action == ClientAction::OkStop ? core::socket::State(core::socket::State::OK, __FILE__, __LINE__) : core::socket::State(core::socket::State::ERROR, __FILE__, __LINE__);
            if (action == ClientAction::NoRetryError) {
                status |= core::socket::State::NO_RETRY;
            }
            onStatus(address, status);

            if (action == ClientAction::TerminalError || action == ClientAction::NoRetryError || action == ClientAction::OkStop) {
                core::SNodeC::stop();
            }
        }

        static void reset(std::vector<ClientAction> actions) {
            queuedActions = std::move(actions);
        }

        static void cleanup() {
            connectedConnections.clear();
            for (TestConnectEventReceiver* connector : liveConnectors) {
                delete connector;
            }
            liveConnectors.clear();
        }

    private:
        void connectEvent() override {
        }

        void unobservedEvent() override {
        }

        static ClientAction nextAction() {
            if (queuedActions.empty()) {
                return ClientAction::OkStop;
            }
            const ClientAction action = queuedActions.front();
            queuedActions.erase(queuedActions.begin());
            return action;
        }

        static void connectConnection(int fd,
                                      const std::string& instanceName,
                                      const std::function<std::uint64_t()>& allocateConnectionId,
                                      const std::function<void(SocketConnection*)>& onConnect,
                                      const std::function<void(SocketConnection*)>& onConnected,
                                      const std::function<void(SocketConnection*)>& onDisconnect,
                                      ClientAction action) {
            connectedConnections.push_back(std::make_unique<SocketConnection>(fd, allocateConnectionId(), instanceName));
            SocketConnection* connection = connectedConnections.back().get();
            if (onConnect) {
                onConnect(connection);
            }
            if (onConnected) {
                onConnected(connection);
            }
            if (action == ClientAction::ConnectedThenDisconnectStopping) {
                core::SNodeC::stop();
            }
            if (action != ClientAction::ConnectedStop && onDisconnect) {
                onDisconnect(connection);
            }
        }

        TestSocketAddress address;
        static std::vector<ClientAction> queuedActions;
        static std::vector<std::unique_ptr<SocketConnection>> connectedConnections;
        static std::vector<TestConnectEventReceiver*> liveConnectors;
    };

    std::vector<ClientAction> TestConnectEventReceiver::queuedActions;
    std::vector<std::unique_ptr<TestConnectEventReceiver::SocketConnection>> TestConnectEventReceiver::connectedConnections;
    std::vector<TestConnectEventReceiver*> TestConnectEventReceiver::liveConnectors;

    using TestSocketServer = core::socket::stream::SocketServer<TestAcceptEventReceiver, TestSocketContextFactory>;
    using TestSocketClient = core::socket::stream::SocketClient<TestConnectEventReceiver, TestSocketContextFactory>;

    void runLoopOnce() {
        core::SNodeC::start(utils::Timeval(0));
    }

    bool hasSummaryScope(const std::string& log, const std::string& role, const std::string& instance) {
        return log.find(" INF framework/instance core.socket.stream role=" + role + " inst=" + instance + " ") != std::string::npos;
    }

    bool summaryAppearsBeforeShutdown(const std::string& log) {
        const auto summary = log.find("Instance terminated:");
        const auto config = log.find("Core: Shutdown config system");
        const auto bye = log.find("SNode.C: Ended ... BYE");
        return summary != std::string::npos && (config == std::string::npos || summary < config) && (bye == std::string::npos || summary < bye);
    }
} // namespace

int main(int argc, char* argv[]) {
    const std::vector<std::string> scenarios = {
        "server-terminal",
        "server-retry",
        "server-retry-cancelled",
        "client-terminal",
        "client-retry",
        "client-retry-cancelled",
        "client-disconnect-terminal",
        "client-reconnect-accepted",
        "client-reconnect-cancelled",
        "server-sequence",
        "client-sequence",
        "expired-weak-context",
    };

    if (argc == 1) {
        tests::support::TestResult parentResult;
        for (const std::string& scenario : scenarios) {
            const std::string command = std::string(argv[0]) + " " + scenario;
            parentResult.expectEqual(0, std::system(command.c_str()), "endpoint lifetime scenario " + scenario + " passes");
        }
        return parentResult.processResult();
    }

    const std::string scenario = argv[1];
    tests::support::TestResult result;

    if (scenario == "server-terminal") {
        const auto logPath = tempLogPath("snodec-endpoint-server-terminal.log");
        LoggerStateGuard loggerGuard(logPath.string());
        SNodeCGuard snodeGuard;
        TestAcceptEventReceiver::reset({ServerAction::TerminalError, ServerAction::TerminalError});
        TestSocketServer server("endpoint-server-terminal");
        server.getConfig()->setRetry(false);
        server.listen([](const TestSocketAddress&, core::socket::State) {});
        server.listen([](const TestSocketAddress&, core::socket::State) {});
        runLoopOnce();
        logger::Logger::disableLogToFile();
        const auto log = readFile(logPath);
        result.expectEqual(1, static_cast<int>(countOccurrences(log, "Instance terminated: connections=0 retries=0")),
                           "real server terminal listen ERROR emits exactly one summary");
        result.expectTrue(hasSummaryScope(log, "server", "endpoint-server-terminal"),
                          "server summary is Info framework/instance core.socket.stream with server role and instance");
        result.expectTrue(log.find("conn=") == std::string::npos, "server instance summary has no connection field");
        result.expectTrue(summaryAppearsBeforeShutdown(log), "server terminal summary remains before shutdown logs and is not repeated by free");
    }

    if (scenario == "server-retry") {
        const auto logPath = tempLogPath("snodec-endpoint-server-retry.log");
        LoggerStateGuard loggerGuard(logPath.string());
        SNodeCGuard snodeGuard;
        TestAcceptEventReceiver::reset({ServerAction::RetryableError, ServerAction::OkStop});
        TestSocketServer server("endpoint-server-retry");
        server.getConfig()->setRetry(true)->setRetryTimeout(0)->setRetryTries(1);
        std::uint64_t retrySeenByObserver = 0;
        server.getFlowController()->setOnFlowRetry([&](core::socket::stream::ServerFlowController* flow) {
            retrySeenByObserver = flow->getRetryCount();
        });
        server.listen([](const TestSocketAddress&, core::socket::State) {});
        runLoopOnce();
        logger::Logger::disableLogToFile();
        const auto log = readFile(logPath);
        result.expectEqual(1, static_cast<int>(server.getFlowController()->getRetryCount()),
                           "real server retry dispatch increments retryCount once");
        result.expectEqual(1, static_cast<int>(retrySeenByObserver), "server retry observer sees incremented retryCount");
        result.expectEqual(1, static_cast<int>(countOccurrences(log, "retry scheduled")), "server retry is scheduled once");
        result.expectEqual(1, static_cast<int>(countOccurrences(log, "retry dispatched")), "server retry is dispatched once");
        result.expectEqual(0, static_cast<int>(countOccurrences(log, "retry cancelled")), "dispatched server retry is not cancelled");
    }

    if (scenario == "server-no-retry") {
        const auto logPath = tempLogPath("snodec-endpoint-server-no-retry.log");
        LoggerStateGuard loggerGuard(logPath.string());
        SNodeCGuard snodeGuard;
        TestAcceptEventReceiver::reset({ServerAction::NoRetryError});
        TestSocketServer server("endpoint-server-no-retry");
        server.getConfig()->setRetry(false);
        server.listen([](const TestSocketAddress&, core::socket::State) {});
        runLoopOnce();
        logger::Logger::disableLogToFile();
        const auto log = readFile(logPath);
        result.expectEqual(1, static_cast<int>(countOccurrences(log, "Instance terminated: connections=0 retries=0")),
                           "server NO_RETRY address fallback emits graceful shutdown summary");
    }

    if (scenario == "server-retry-cancelled") {
        const auto logPath = tempLogPath("snodec-endpoint-server-retry-cancelled.log");
        LoggerStateGuard loggerGuard(logPath.string());
        SNodeCGuard snodeGuard;
        TestAcceptEventReceiver::reset({ServerAction::ErrorThenStopBeforePolicy});
        TestSocketServer server("endpoint-server-retry-cancelled");
        server.getConfig()->setRetry(true)->setRetryTimeout(30)->setRetryTries(1);
        server.listen([](const TestSocketAddress&, core::socket::State) {
        });
        runLoopOnce();
        server.getFlowController()->terminateFlow();
        logger::Logger::disableLogToFile();
        const auto log = readFile(logPath);
        result.expectEqual(1, static_cast<int>(countOccurrences(log, "retry scheduled")), "server retry cancellation starts scheduled");
        result.expectEqual(0, static_cast<int>(countOccurrences(log, "retry dispatched")), "cancelled server retry is never dispatched");
        result.expectEqual(1, static_cast<int>(countOccurrences(log, "retry cancelled")), "scheduled server retry is cancelled once");
        result.expectEqual(
            0, static_cast<int>(server.getFlowController()->getRetryCount()), "cancelled server retry does not increment counter");
    }

    if (scenario == "server-shutdown") {
        const auto logPath = tempLogPath("snodec-endpoint-server-shutdown.log");
        LoggerStateGuard loggerGuard(logPath.string());
        SNodeCGuard snodeGuard;
        TestAcceptEventReceiver::reset({ServerAction::ErrorThenStopBeforePolicy});
        TestSocketServer server("endpoint-server-shutdown");
        server.getConfig()->setRetry(false);
        server.listen([](const TestSocketAddress&, core::socket::State) {});
        runLoopOnce();
        logger::Logger::disableLogToFile();
        const auto log = readFile(logPath);
        result.expectEqual(1, static_cast<int>(countOccurrences(log, "Instance terminated: connections=0 retries=0")),
                           "server graceful shutdown emits exactly one summary");
        result.expectTrue(summaryAppearsBeforeShutdown(log), "server graceful summary appears before config shutdown and BYE");
    }

    if (scenario == "client-terminal") {
        const auto logPath = tempLogPath("snodec-endpoint-client-terminal.log");
        LoggerStateGuard loggerGuard(logPath.string());
        SNodeCGuard snodeGuard;
        TestConnectEventReceiver::reset({ClientAction::TerminalError, ClientAction::TerminalError});
        TestSocketClient client("endpoint-client-terminal");
        client.getConfig()->setRetry(false);
        client.connect([](const TestSocketAddress&, core::socket::State) {});
        client.connect([](const TestSocketAddress&, core::socket::State) {});
        runLoopOnce();
        logger::Logger::disableLogToFile();
        const auto log = readFile(logPath);
        result.expectEqual(1, static_cast<int>(countOccurrences(log, "Instance terminated: connections=0 retries=0 reconnects=0")),
                           "real client terminal connect ERROR emits exactly one summary");
        result.expectTrue(hasSummaryScope(log, "client", "endpoint-client-terminal"),
                          "client connect summary is Info framework/instance core.socket.stream with client role and instance");
        result.expectTrue(log.find("conn=") == std::string::npos, "client connect instance summary has no connection field");
        result.expectTrue(summaryAppearsBeforeShutdown(log), "client terminal summary remains before shutdown logs and is not repeated by free");
    }

    if (scenario == "client-retry") {
        const auto logPath = tempLogPath("snodec-endpoint-client-retry.log");
        LoggerStateGuard loggerGuard(logPath.string());
        SNodeCGuard snodeGuard;
        TestConnectEventReceiver::reset({ClientAction::RetryableError, ClientAction::OkStop});
        TestSocketClient client("endpoint-client-retry");
        client.getConfig()->setRetry(true)->setRetryTimeout(0)->setRetryTries(1);
        std::uint64_t retrySeenByObserver = 0;
        client.getFlowController()->setOnFlowRetry([&](core::socket::stream::ClientFlowController* flow) {
            retrySeenByObserver = flow->getRetryCount();
        });
        client.connect([](const TestSocketAddress&, core::socket::State) {});
        runLoopOnce();
        logger::Logger::disableLogToFile();
        const auto log = readFile(logPath);
        result.expectEqual(1, static_cast<int>(client.getFlowController()->getRetryCount()),
                           "real client retry dispatch increments retryCount once");
        result.expectEqual(1, static_cast<int>(retrySeenByObserver), "client retry observer sees incremented retryCount");
        result.expectEqual(1, static_cast<int>(countOccurrences(log, "retry scheduled")), "client retry is scheduled once");
        result.expectEqual(1, static_cast<int>(countOccurrences(log, "retry dispatched")), "client retry is dispatched once");
        result.expectEqual(0, static_cast<int>(countOccurrences(log, "retry cancelled")), "dispatched client retry is not cancelled");
    }

    if (scenario == "client-no-retry") {
        const auto logPath = tempLogPath("snodec-endpoint-client-no-retry.log");
        LoggerStateGuard loggerGuard(logPath.string());
        SNodeCGuard snodeGuard;
        TestConnectEventReceiver::reset({ClientAction::NoRetryError});
        TestSocketClient client("endpoint-client-no-retry");
        client.getConfig()->setRetry(false);
        client.connect([](const TestSocketAddress&, core::socket::State) {});
        runLoopOnce();
        logger::Logger::disableLogToFile();
        const auto log = readFile(logPath);
        result.expectEqual(1, static_cast<int>(countOccurrences(log, "Instance terminated: connections=0 retries=0 reconnects=0")),
                           "client NO_RETRY address fallback emits graceful shutdown summary");
    }

    if (scenario == "client-retry-cancelled") {
        const auto logPath = tempLogPath("snodec-endpoint-client-retry-cancelled.log");
        LoggerStateGuard loggerGuard(logPath.string());
        SNodeCGuard snodeGuard;
        TestConnectEventReceiver::reset({ClientAction::ErrorThenStopBeforePolicy});
        TestSocketClient client("endpoint-client-retry-cancelled");
        client.getConfig()->setRetry(true)->setRetryTimeout(30)->setRetryTries(1);
        client.connect([](const TestSocketAddress&, core::socket::State) {
        });
        runLoopOnce();
        client.getFlowController()->terminateFlow();
        logger::Logger::disableLogToFile();
        const auto log = readFile(logPath);
        result.expectEqual(1, static_cast<int>(countOccurrences(log, "retry scheduled")), "client retry cancellation starts scheduled");
        result.expectEqual(0, static_cast<int>(countOccurrences(log, "retry dispatched")), "cancelled client retry is never dispatched");
        result.expectEqual(1, static_cast<int>(countOccurrences(log, "retry cancelled")), "scheduled client retry is cancelled once");
        result.expectEqual(
            0, static_cast<int>(client.getFlowController()->getRetryCount()), "cancelled client retry does not increment counter");
    }

    if (scenario == "client-shutdown") {
        const auto logPath = tempLogPath("snodec-endpoint-client-shutdown.log");
        LoggerStateGuard loggerGuard(logPath.string());
        SNodeCGuard snodeGuard;
        TestConnectEventReceiver::reset({ClientAction::ErrorThenStopBeforePolicy});
        TestSocketClient client("endpoint-client-shutdown");
        client.getConfig()->setRetry(false);
        client.connect([](const TestSocketAddress&, core::socket::State) {});
        runLoopOnce();
        logger::Logger::disableLogToFile();
        const auto log = readFile(logPath);
        result.expectEqual(1, static_cast<int>(countOccurrences(log, "Instance terminated: connections=0 retries=0 reconnects=0")),
                           "client graceful shutdown emits exactly one summary");
        result.expectTrue(summaryAppearsBeforeShutdown(log), "client graceful summary appears before config shutdown and BYE");
    }

    if (scenario == "client-disconnect-terminal") {
        const auto logPath = tempLogPath("snodec-endpoint-client-disconnect-terminal.log");
        LoggerStateGuard loggerGuard(logPath.string());
        SNodeCGuard snodeGuard;
        TestConnectEventReceiver::reset({ClientAction::ConnectedThenDisconnect, ClientAction::ConnectedThenDisconnect});
        TestSocketClient client("endpoint-client-disconnect-terminal");
        client.getConfig()->setReconnect(false);
        client.connect([](const TestSocketAddress&, core::socket::State) {});
        client.connect([](const TestSocketAddress&, core::socket::State) {});
        runLoopOnce();
        logger::Logger::disableLogToFile();
        const auto log = readFile(logPath);
        result.expectEqual(1, static_cast<int>(countOccurrences(log, "Instance terminated: connections=1 retries=0 reconnects=0")),
                           "real client reconnect rejection during RUNNING emits exactly one summary");
        result.expectTrue(hasSummaryScope(log, "client", "endpoint-client-disconnect-terminal"),
                          "client reconnect summary is Info framework/instance core.socket.stream with client role and instance");
        result.expectTrue(log.find("conn=") == std::string::npos, "client reconnect instance summary has no connection field");
    }

    if (scenario == "client-reconnect-accepted") {
        const auto logPath = tempLogPath("snodec-endpoint-client-reconnect-accepted.log");
        LoggerStateGuard loggerGuard(logPath.string());
        SNodeCGuard snodeGuard;
        TestConnectEventReceiver::reset({ClientAction::ConnectedThenDisconnect, ClientAction::ConnectedStop});
        TestSocketClient client("endpoint-client-reconnect-accepted");
        client.getConfig()->setReconnect(true)->setReconnectTime(0);
        std::uint64_t reconnectSeenByObserver = 0;
        client.getFlowController()->setOnFlowReconnect([&](core::socket::stream::ClientFlowController* flow) {
            reconnectSeenByObserver = flow->getReconnectCount();
        });
        std::vector<std::uint64_t> connectionIds;
        client.setOnConnected([&](TestSocketConnection* connection) {
            connectionIds.push_back(connection->getConnectionId());
        });
        client.connect([](const TestSocketAddress&, core::socket::State) {});
        runLoopOnce();
        logger::Logger::disableLogToFile();
        const auto log = readFile(logPath);
        result.expectEqual(1, static_cast<int>(client.getFlowController()->getReconnectCount()),
                           "real client reconnect dispatch increments reconnectCount once");
        result.expectEqual(1, static_cast<int>(reconnectSeenByObserver), "reconnect observer sees incremented reconnectCount");
        result.expectEqual(1, static_cast<int>(countOccurrences(log, "reconnect scheduled")), "reconnect is scheduled once");
        result.expectEqual(1, static_cast<int>(countOccurrences(log, "reconnect dispatched")), "reconnect is dispatched once");
        result.expectEqual(0, static_cast<int>(countOccurrences(log, "reconnect cancelled")), "dispatched reconnect is not cancelled");
        result.expectTrue(connectionIds.size() == 2 && connectionIds[0] == 1 && connectionIds[1] == 2,
                          "reconnect-created client connections continue the same shared sequence");
    }

    if (scenario == "client-disconnect-stopping") {
        const auto logPath = tempLogPath("snodec-endpoint-client-disconnect-stopping.log");
        LoggerStateGuard loggerGuard(logPath.string());
        SNodeCGuard snodeGuard;
        TestConnectEventReceiver::reset({ClientAction::ConnectedThenDisconnectStopping});
        TestSocketClient client("endpoint-client-disconnect-stopping");
        client.getConfig()->setReconnect(false);
        client.connect([](const TestSocketAddress&, core::socket::State) {});
        runLoopOnce();
        logger::Logger::disableLogToFile();
        const auto log = readFile(logPath);
        result.expectEqual(1, static_cast<int>(countOccurrences(log, "Instance terminated: connections=1 retries=0 reconnects=0")),
                           "client disconnect while STOPPING emits graceful shutdown summary");
    }

    if (scenario == "client-reconnect-cancelled") {
        const auto logPath = tempLogPath("snodec-endpoint-client-reconnect-cancelled.log");
        LoggerStateGuard loggerGuard(logPath.string());
        SNodeCGuard snodeGuard;
        TestConnectEventReceiver::reset({ClientAction::ConnectedThenDisconnect});
        TestSocketClient client("endpoint-client-reconnect-cancelled");
        client.getConfig()->setReconnect(true)->setReconnectTime(30);
        client.connect([](const TestSocketAddress&, core::socket::State) {
        });
        core::EventReceiver::atNextTick([]() {
            core::SNodeC::stop();
        });
        runLoopOnce();
        client.getFlowController()->terminateFlow();
        logger::Logger::disableLogToFile();
        const auto log = readFile(logPath);
        result.expectEqual(1, static_cast<int>(countOccurrences(log, "reconnect scheduled")), "reconnect cancellation starts scheduled");
        result.expectEqual(0, static_cast<int>(countOccurrences(log, "reconnect dispatched")), "cancelled reconnect is never dispatched");
        result.expectEqual(1, static_cast<int>(countOccurrences(log, "reconnect cancelled")), "scheduled reconnect is cancelled once");
        result.expectEqual(
            0, static_cast<int>(client.getFlowController()->getReconnectCount()), "cancelled reconnect does not increment counter");
    }

    if (scenario == "server-sequence") {
        SNodeCGuard snodeGuard;
        TestAcceptEventReceiver::reset({ServerAction::TwoAcceptedConnectionsStop});
        std::vector<std::uint64_t> serverConnectionIds;
        TestSocketServer server("endpoint-server-sequence");
        server.setOnConnected([&](TestSocketConnection* connection) {
            serverConnectionIds.push_back(connection->getConnectionId());
        });
        server.listen([](const TestSocketAddress&, core::socket::State) {});
        runLoopOnce();
        result.expectTrue(serverConnectionIds.size() == 2 && serverConnectionIds[0] == 1 && serverConnectionIds[1] == 2,
                          "real server acceptor allocator gives conn=1 then conn=2 despite descriptor reuse");
    }

    if (scenario == "client-sequence") {
        SNodeCGuard snodeGuard;
        TestConnectEventReceiver::reset({ClientAction::ConnectedStop});
        std::vector<std::uint64_t> clientConnectionIds;
        TestSocketClient client("endpoint-client-sequence");
        client.setOnConnected([&](TestSocketConnection* connection) {
            clientConnectionIds.push_back(connection->getConnectionId());
        });
        client.connect([](const TestSocketAddress&, core::socket::State) {});
        runLoopOnce();
        result.expectTrue(clientConnectionIds.size() == 1 && clientConnectionIds[0] == 1,
                          "real client connector allocator starts its independent sequence at conn=1");
    }

    if (scenario == "expired-weak-context") {
        const auto logPath = tempLogPath("snodec-endpoint-expired-weak-context.log");
        LoggerStateGuard loggerGuard(logPath.string());
        SNodeCGuard snodeGuard;
        {
            TestSocketServer server("endpoint-expired-server");
            TestSocketClient client("endpoint-expired-client");
        }
        runLoopOnce();
        logger::Logger::disableLogToFile();
        result.expectTrue(readFile(logPath).find("Instance terminated:") == std::string::npos,
                          "expired weak endpoint contexts are ignored safely");
    }


    TestAcceptEventReceiver::cleanup();
    TestConnectEventReceiver::cleanup();

    return result.processResult();
}
