#include "core/socket/SocketAddress.h"
#include "core/socket/stream/SocketAcceptor.h"
#include "core/socket/stream/SocketAcceptor.hpp"
#include "log/Logger.h"
#include "log/SemanticLogger.h"
#include "net/config/ConfigInstance.h"
#include "tests/support/TestResult.h"
#include "utils/Timeval.h"

#include <cerrno>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <functional>
#include <memory>
#include <nlohmann/json.hpp>
#include <string>
#include <system_error>
#include <utility>

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
            logger::Logger::logToFile(logFile);
            logger::LogManager::setGlobalLevel(logger::LogLevel::Debug);
            logger::LogManager::setFormat(logger::LogManager::Format::Json);
            logger::LogManager::freeze();
        }

        ~LoggerStateGuard() {
            logger::Logger::disableLogToFile();
            logger::Logger::init();
            logger::LogManager::init();
            errno = 0;
        }
    };

    std::filesystem::path tempLogPath() {
        const auto path = std::filesystem::temp_directory_path() / "snodec-phase2-listener-start-failure.jsonl";
        std::error_code error;
        std::filesystem::remove(path, error);
        std::filesystem::remove(path.string() + ".1", error);
        return path;
    }

    class TestSocketAddress : public core::socket::SocketAddress {
    public:
        std::string toString(bool = true) const override {
            return "deterministic-listener-address";
        }
    };

    struct TestAddressConfig {
        TestSocketAddress getSocketAddress() const {
            return {};
        }
    };

    class TestConfig
        : public net::config::ConfigInstance
        , public TestAddressConfig {
    public:
        using Local = TestAddressConfig;

        explicit TestConfig(const std::string& instanceName)
            : ConfigInstance(instanceName, Role::SERVER) {
        }

        ~TestConfig() override = default;

        int getSocketOptions() const {
            return 0;
        }

        int getBacklog() const {
            return 1;
        }

        int getAcceptsPerTick() const {
            return 1;
        }

        utils::Timeval getAcceptTimeout() const {
            return {};
        }
    };

    class FailingPhysicalSocketServer {
    public:
        using SocketAddress = TestSocketAddress;

        enum class Flags { NONBLOCK };

        FailingPhysicalSocketServer() = default;

        FailingPhysicalSocketServer(int, const SocketAddress&) {
        }

        int open(int, Flags) {
            errno = EACCES;
            return -1;
        }

        int bind(const SocketAddress&) {
            return 0;
        }

        int listen(int) {
            return 0;
        }

        int accept4(Flags) {
            errno = EAGAIN;
            return -1;
        }

        bool isValid() const {
            return false;
        }

        int getFd() const {
            return -1;
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

    class FailingAcceptor : public core::socket::stream::SocketAcceptor<FailingPhysicalSocketServer, TestConfig, TestSocketConnection> {
    private:
        using Super = core::socket::stream::SocketAcceptor<FailingPhysicalSocketServer, TestConfig, TestSocketConnection>;

    public:
        FailingAcceptor(const std::shared_ptr<TestConfig>& config, int& statusCallbacks, int& initStateCallbacks)
            : Super(
                  {},
                  {},
                  {},
                  [&initStateCallbacks](core::eventreceiver::AcceptEventReceiver*) {
                      ++initStateCallbacks;
                  },
                  [&statusCallbacks](const TestSocketAddress&, core::socket::State) {
                      ++statusCallbacks;
                  },
                  [] {
                      return std::uint64_t{1};
                  },
                  config) {
        }

    private:
        void useNextSocketAddress() override {
        }
    };
} // namespace

int main() {
    tests::support::TestResult result;
    const auto logPath = tempLogPath();
    int statusCallbacks = 0;
    int initStateCallbacks = 0;

    {
        LoggerStateGuard guard(logPath.string());
        auto config = std::make_shared<TestConfig>("phase2-failing-listener");
        auto* acceptor = new FailingAcceptor(config, statusCallbacks, initStateCallbacks);
        acceptor->init();
    }

    int startFailed = 0;
    int started = 0;
    int stopped = 0;
    std::ifstream input(logPath);
    std::string line;
    while (std::getline(input, line)) {
        if (line.empty()) {
            continue;
        }
        const auto record = nlohmann::json::parse(line);
        const std::string message = record.at("message").get<std::string>();
        if (message == "listener start failed") {
            ++startFailed;
            result.expectTrue(record.at("level") == "debug" && record.at("origin") == "framework" && record.at("boundary") == "instance" &&
                                  record.at("component") == "core.socket.stream" && record.at("instance") == "phase2-failing-listener" &&
                                  record.at("role") == "server" && !record.contains("connection"),
                              "listener startup failure carries server endpoint identity at Debug");
        } else if (message == "listener started") {
            ++started;
        } else if (message == "listener stopped") {
            ++stopped;
        }
    }

    result.expectEqual(1, statusCallbacks, "deterministic open failure reports status exactly once");
    result.expectEqual(1, initStateCallbacks, "failed acceptor cleanup reports final init state exactly once");
    result.expectEqual(1, startFailed, "failed listener emits exactly one startup terminal record");
    result.expectEqual(0, started, "failed listener emits no listener started record");
    result.expectEqual(0, stopped, "failed listener emits no listener stopped record");

    return result.processResult();
}
