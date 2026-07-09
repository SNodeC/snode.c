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
                return std::string("MIGRATION03TICK");
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

    class TestConfigInstance : public net::config::ConfigInstance {
    public:
        explicit TestConfigInstance(const std::string& instanceName)
            : ConfigInstance(instanceName, Role::SERVER) {
        }
        ~TestConfigInstance() override = default;
    };

    class TestSocketAddress : public core::socket::SocketAddress {
    public:
        std::string toString(bool = true) const override {
            return "migration03-address";
        }
    };

    class TestSocketConnection {
    public:
        std::string getConnectionName() const {
            return "migration03-connection";
        }

        const TestSocketAddress& getLocalAddress() const {
            return address;
        }

        const TestSocketAddress& getRemoteAddress() const {
            return address;
        }

        std::string getOnlineSince() const {
            return "migration03-online-since";
        }

        double getOnlineDuration() const {
            return 1.0;
        }

        std::size_t getTotalQueued() const {
            return 2;
        }

        std::size_t getTotalSent() const {
            return 1;
        }

        std::size_t getTotalRead() const {
            return 4;
        }

        std::size_t getTotalProcessed() const {
            return 3;
        }

    private:
        TestSocketAddress address;
    };

    class TestSocketContextFactory : public core::socket::stream::SocketContextFactory {
    public:
        core::socket::stream::SocketContext* create(core::socket::stream::SocketConnection*) override {
            return nullptr;
        }
    };

    class TestAcceptEventReceiver : public core::eventreceiver::AcceptEventReceiver {
    public:
        using Config = TestConfigInstance;
        using SocketAddress = TestSocketAddress;
        using SocketConnection = TestSocketConnection;
    };

    class TestConnectEventReceiver : public core::eventreceiver::ConnectEventReceiver {
    public:
        using Config = TestConfigInstance;
        using SocketAddress = TestSocketAddress;
        using SocketConnection = TestSocketConnection;
    };

    using TestSocketServer = core::socket::stream::SocketServer<TestAcceptEventReceiver, TestSocketContextFactory>;
    using TestSocketClient = core::socket::stream::SocketClient<TestConnectEventReceiver, TestSocketContextFactory>;
} // namespace

int main() {
    tests::support::TestResult result;

    const auto enabledPath = tempLogPath("snodec-migration03-enabled.log");
    {
        LoggerStateGuard guard(enabledPath.string());
        logger::LogManager::setGlobalLevel(logger::LogLevel::Debug);
        logger::LogManager::freeze();
        TestSocketServer("migration03-server").log().debug("server semantic owner emitted");
        TestSocketClient("migration03-client").log().info("client semantic owner emitted");
    }
    const auto enabledLog = readFile(enabledPath);
    result.expectTrue(enabledLog.find("server semantic owner emitted") != std::string::npos,
                      "SocketServer semantic owner emits when enabled");
    result.expectTrue(enabledLog.find("client semantic owner emitted") != std::string::npos,
                      "SocketClient semantic owner emits when enabled");
    result.expectTrue(enabledLog.find("framework/instance core.socket.stream role=server inst=migration03-server ") != std::string::npos,
                      "server emitted records carry framework instance core.socket.stream scope and server role");
    result.expectTrue(enabledLog.find("framework/instance core.socket.stream role=client inst=migration03-client ") != std::string::npos,
                      "client emitted records carry framework instance core.socket.stream scope and client role");

    const auto managerFilterPath = tempLogPath("snodec-migration03-manager-filter.log");
    {
        LoggerStateGuard guard(managerFilterPath.string());
        logger::LogManager::setGlobalLevel(logger::LogLevel::Error);
        logger::LogManager::freeze();
        TestSocketServer("migration03-server").log().debug("server suppressed by manager");
        TestSocketClient("migration03-client").log().warn("client suppressed by manager");
    }
    result.expectTrue(readFile(managerFilterPath).find("suppressed by manager") == std::string::npos,
                      "LogManager filtering suppresses server/client semantic calls");

    const auto backendFilterPath = tempLogPath("snodec-migration03-backend-filter.log");
    {
        LoggerStateGuard guard(backendFilterPath.string());
        logger::LogManager::setGlobalLevel(logger::LogLevel::Trace);
        logger::LogManager::freeze();
        logger::Logger::setLogLevel(3);
        TestSocketServer("migration03-server").log().debug("server suppressed by backend");
        TestSocketClient("migration03-client").log().debug("client suppressed by backend");
    }
    result.expectTrue(readFile(backendFilterPath).find("suppressed by backend") != std::string::npos,
                      "semantic output accepted by LogManager is not suppressed by Logger::setLogLevel");

    const auto jsonPath = tempLogPath("snodec-migration03-json.log");
    {
        LoggerStateGuard guard(jsonPath.string());
        logger::LogManager::setGlobalLevel(logger::LogLevel::Debug);
        logger::LogManager::setFormat(logger::LogManager::Format::Json);
        logger::LogManager::freeze();
        TestSocketClient("migration03-client").log().debug("client json format respected");
    }
    const auto jsonLog = readFile(jsonPath);
    result.expectTrue(jsonLog.find("{\"v\":1") != std::string::npos &&
                          jsonLog.find("\"message\":\"client json format respected\"") != std::string::npos &&
                          jsonLog.find("\"role\":\"client\"") != std::string::npos,
                      "JSON format selection is respected");

    std::vector<logger::LogRecord> sysErrorRecords;
    TestSocketServer("migration03-server")
        .log([&](logger::LogRecord record) {
            sysErrorRecords.push_back(std::move(record));
        })
        .sysError(logger::LogLevel::Error, EACCES, "server preserved errno");
    result.expectTrue(sysErrorRecords.size() == 1 && sysErrorRecords[0].message == "server preserved errno" && sysErrorRecords[0].error &&
                          sysErrorRecords[0].error->code == EACCES &&
                          sysErrorRecords[0].error->text.find("Permission") != std::string::npos,
                      "sysError errno code/text preservation through the owner path");

    std::vector<logger::LogRecord> sinkRecords;
    TestSocketClient("migration03-client")
        .log([&](logger::LogRecord record) {
            sinkRecords.push_back(std::move(record));
        })
        .info("sink overload still works");
    result.expectTrue(sinkRecords.size() == 1 && sinkRecords[0].message == "sink overload still works" && sinkRecords[0].role &&
                          *sinkRecords[0].role == logger::LogRole::Client,
                      "sink-taking overload remains compatible");

    const auto suppressedPath = tempLogPath("snodec-migration03-suppressed.log");
    {
        LoggerStateGuard guard(suppressedPath.string());
        logger::LogManager::setGlobalLevel(logger::LogLevel::Error);
        logger::LogManager::freeze();

        int evaluations = 0;
        TestSocketServer("migration03-server").log().debug("{}", ExpensiveValue{&evaluations});
        result.expectEqual(0, evaluations, "suppressed production formatting does not evaluate ExpensiveValue after PR #151");
    }
    result.expectTrue(readFile(suppressedPath).empty(), "suppressed production formatting emits no backend output");

    return result.processResult();
}
