#include "core/socket/SocketAddress.h"
#include "core/socket/stream/SocketAcceptor.h"
#include "core/socket/stream/SocketConnector.h"
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
                return std::string("MIGRATION02TICK");
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
            return "migration02-address";
        }
    };

    class TestPhysicalSocket {
    public:
        using SocketAddress = TestSocketAddress;
    };

    template <typename PhysicalSocket, typename Config>
    class TestTemplatedSocketConnection;

    class TestConnectorOwner
        : public core::socket::stream::SocketConnector<TestPhysicalSocket, TestConfigInstance, TestTemplatedSocketConnection> {
    private:
        using Super = core::socket::stream::SocketConnector<TestPhysicalSocket, TestConfigInstance, TestTemplatedSocketConnection>;

    public:
        static logger::BoundaryLogger logFor(const std::string& instanceName) {
            return Super::makeLogScope(instanceName).logger(logger::Logger::semanticSink());
        }

        static logger::BoundaryLogger
        logFor(const std::string& instanceName, logger::BoundaryLogger::Sink sink, logger::LogLevel threshold = logger::LogLevel::Trace) {
            return Super::makeLogScope(instanceName).logger(std::move(sink), threshold);
        }
    };

    class TestAcceptorOwner
        : public core::socket::stream::SocketAcceptor<TestPhysicalSocket, TestConfigInstance, TestTemplatedSocketConnection> {
    private:
        using Super = core::socket::stream::SocketAcceptor<TestPhysicalSocket, TestConfigInstance, TestTemplatedSocketConnection>;

    public:
        static logger::BoundaryLogger logFor(const std::string& instanceName) {
            return Super::makeLogScope(instanceName).logger(logger::Logger::semanticSink());
        }

        static logger::BoundaryLogger
        logFor(const std::string& instanceName, logger::BoundaryLogger::Sink sink, logger::LogLevel threshold = logger::LogLevel::Trace) {
            return Super::makeLogScope(instanceName).logger(std::move(sink), threshold);
        }
    };
} // namespace

int main() {
    tests::support::TestResult result;

    const auto enabledPath = tempLogPath("snodec-migration02-enabled.log");
    {
        LoggerStateGuard guard(enabledPath.string());
        logger::LogManager::setGlobalLevel(logger::LogLevel::Debug);
        logger::LogManager::freeze();
        TestConnectorOwner::logFor("migration02-connector").debug("connector migrated semantic owner emitted");
        TestAcceptorOwner::logFor("migration02-acceptor").info("acceptor migrated semantic owner emitted");
    }
    const auto enabledLog = readFile(enabledPath);
    result.expectTrue(enabledLog.find("connector migrated semantic owner emitted") != std::string::npos,
                      "migrated SocketConnector semantic owner emits when enabled");
    result.expectTrue(enabledLog.find("acceptor migrated semantic owner emitted") != std::string::npos,
                      "migrated SocketAcceptor semantic owner emits when enabled");
    result.expectTrue(enabledLog.find("framework/instance core.socket.stream role=client inst=migration02-connector ") !=
                          std::string::npos,
                      "connector emitted records carry framework instance client scope");
    result.expectTrue(enabledLog.find("framework/instance core.socket.stream role=server inst=migration02-acceptor ") !=
                          std::string::npos,
                      "acceptor emitted records carry framework instance server scope");

    const auto managerFilterPath = tempLogPath("snodec-migration02-manager-filter.log");
    {
        LoggerStateGuard guard(managerFilterPath.string());
        logger::LogManager::setGlobalLevel(logger::LogLevel::Error);
        logger::LogManager::freeze();
        TestConnectorOwner::logFor("migration02-connector").debug("connector suppressed by manager");
        TestAcceptorOwner::logFor("migration02-acceptor").warn("acceptor suppressed by manager");
    }
    const auto managerFilterLog = readFile(managerFilterPath);
    result.expectTrue(managerFilterLog.find("suppressed by manager") == std::string::npos,
                      "LogManager filtering suppresses migrated connector/acceptor semantic calls");

    const auto backendFilterPath = tempLogPath("snodec-migration02-backend-filter.log");
    {
        LoggerStateGuard guard(backendFilterPath.string());
        logger::LogManager::setGlobalLevel(logger::LogLevel::Trace);
        logger::LogManager::freeze();
        logger::Logger::setLogLevel(3);
        TestConnectorOwner::logFor("migration02-connector").debug("connector suppressed by backend");
        TestAcceptorOwner::logFor("migration02-acceptor").debug("acceptor suppressed by backend");
    }
    const auto backendFilterLog = readFile(backendFilterPath);
    result.expectTrue(backendFilterLog.find("suppressed by backend") != std::string::npos,
                      "semantic output accepted by LogManager is not suppressed by Logger::setLogLevel");

    const auto jsonPath = tempLogPath("snodec-migration02-json.log");
    {
        LoggerStateGuard guard(jsonPath.string());
        logger::LogManager::setGlobalLevel(logger::LogLevel::Debug);
        logger::LogManager::setFormat(logger::LogManager::Format::Json);
        logger::LogManager::freeze();
        TestConnectorOwner::logFor("migration02-connector").debug("connector json format respected");
    }
    const auto jsonLog = readFile(jsonPath);
    result.expectTrue(jsonLog.find("{\"v\":1") != std::string::npos &&
                          jsonLog.find("\"message\":\"connector json format respected\"") != std::string::npos &&
                          jsonLog.find("\"role\":\"client\"") != std::string::npos,
                      "JSON format selection is respected");

    std::vector<logger::LogRecord> sysErrorRecords;
    TestConnectorOwner::logFor("migration02-connector", [&](logger::LogRecord record) {
        sysErrorRecords.push_back(std::move(record));
    }).sysError(logger::LogLevel::Error, EACCES, "connector preserved errno");
    result.expectTrue(sysErrorRecords.size() == 1 && sysErrorRecords[0].message == "connector preserved errno" &&
                          sysErrorRecords[0].error && sysErrorRecords[0].error->code == EACCES &&
                          sysErrorRecords[0].error->text.find("Permission") != std::string::npos,
                      "sysError/PLOG conversion preserves errno code/text when exercised through the migration owner");

    std::vector<logger::LogRecord> sinkRecords;
    TestAcceptorOwner::logFor("migration02-acceptor", [&](logger::LogRecord record) {
        sinkRecords.push_back(std::move(record));
    }).info("sink overload still works");
    result.expectTrue(sinkRecords.size() == 1 && sinkRecords[0].message == "sink overload still works" && sinkRecords[0].role &&
                          *sinkRecords[0].role == logger::LogRole::Server,
                      "sink-taking overload remains compatible");

    const auto suppressedPath = tempLogPath("snodec-migration02-suppressed.log");
    {
        LoggerStateGuard guard(suppressedPath.string());
        logger::LogManager::setGlobalLevel(logger::LogLevel::Error);
        logger::LogManager::freeze();

        int evaluations = 0;
        TestConnectorOwner::logFor("migration02-connector").debug("{}", ExpensiveValue{&evaluations});
        result.expectEqual(0, evaluations, "suppressed production formatting does not evaluate ExpensiveValue after PR #151");
    }
    result.expectTrue(readFile(suppressedPath).empty(), "suppressed production formatting emits no backend output");

    return result.processResult();
}
