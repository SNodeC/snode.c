#include "core/socket/SocketAddress.h"
#include "core/socket/stream/SocketConnection.hpp"
#include "log/Logger.h"
#include "net/config/ConfigInstance.h"
#include "log/SemanticLogger.h"
#include "tests/support/TestResult.h"
#include "utils/Timeval.h"

#include <cerrno>
#include <filesystem>
#include <fstream>
#include <memory>
#include <ostream>
#include <string>
#include <system_error>
#include <vector>

namespace {
    struct ExpensiveValue {
        int* evaluations;
    };

    std::ostream& operator<<(std::ostream& out, const ExpensiveValue& value) {
        ++*value.evaluations;
        return out << "expensive";
    }

    class StateGuard {
    public:
        explicit StateGuard(const std::string& logFile) : savedErrno(errno) {
            logger::Logger::init();
            logger::LogManager::init();
            logger::Logger::setLogLevel(6);
            logger::Logger::setVerboseLevel(0);
            logger::Logger::setQuiet(true);
            logger::Logger::setDisableColor(true);
            logger::Logger::setTickResolver([]() { return std::string("MIG01TICK000"); });
            logger::Logger::logToFile(logFile);
        }
        ~StateGuard() {
            logger::Logger::disableLogToFile();
            logger::Logger::setTickResolver({});
            logger::Logger::init();
            logger::LogManager::init();
            errno = savedErrno;
        }
    private:
        int savedErrno;
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

    class FakeAddress : public core::socket::SocketAddress {
    public:
        struct SockAddr { int value = 0; };
        using SockLen = std::size_t;
        class BadSocketAddress : public std::runtime_error {
        public:
            explicit BadSocketAddress(const std::string& what) : std::runtime_error(what) {}
        };
        std::string toString(bool = true) const override { return "fake-address"; }
    };

    struct FakeConfigLocal {
        FakeAddress getSocketAddress(const FakeAddress::SockAddr&, FakeAddress::SockLen) { return {}; }
    };
    struct FakeConfigRemote {
        FakeAddress getSocketAddress(const FakeAddress::SockAddr&, FakeAddress::SockLen) { return {}; }
    };
    struct FakeConfig : net::config::ConfigInstance, FakeConfigLocal, FakeConfigRemote {
        FakeConfig() : net::config::ConfigInstance("migration01-instance", net::config::ConfigInstance::Role::CLIENT) {}
        using Local = FakeConfigLocal;
        using Remote = FakeConfigRemote;
        utils::Timeval getReadTimeout() const { return {}; }
        std::size_t getReadBlockSize() const { return 1; }
        utils::Timeval getTerminateTimeout() const { return {}; }
        utils::Timeval getWriteTimeout() const { return {}; }
        std::size_t getWriteBlockSize() const { return 1; }
    };

    class FakePhysicalSocket {
    public:
        using SocketAddress = FakeAddress;
        enum class SHUT { RD, WR };
        explicit FakePhysicalSocket(int fd) : fd(fd) {}
        int getFd() const { return fd; }
        int getSockName(FakeAddress::SockAddr&, FakeAddress::SockLen&) { return 0; }
        int getPeerName(FakeAddress::SockAddr&, FakeAddress::SockLen&) { return 0; }
        const FakeAddress& getBindAddress() const { return address; }
        int shutdown(SHUT) {
            errno = EACCES;
            return shutdownResult;
        }
        int shutdownResult = 0;
    private:
        int fd;
        FakeAddress address{};
    };

    class FakeReader {
    public:
        FakeReader(const std::string&, std::function<void(int)> onError, const utils::Timeval&, std::size_t, const utils::Timeval&)
            : onError(std::move(onError)) {}
        bool enable(int) { return true; }
        void disable() { enabled = false; }
        bool isEnabled() const { return enabled; }
        void setTimeout(const utils::Timeval&) {}
        std::size_t readFromPeer(char*, std::size_t) { return 0; }
        void shutdownRead() {}
        std::size_t getTotalRead() const { return 0; }
        std::size_t getTotalProcessed() const { return 0; }
        void triggerError(int errnum) { onError(errnum); }
    protected:
        virtual void onReceivedFromPeer(std::size_t) {}
        virtual bool onSignal(int) { return true; }
        virtual void readTimeout() {}
        virtual void unobservedEvent() {}
    private:
        bool enabled = true;
        std::function<void(int)> onError;
    };

    class FakeWriter {
    public:
        FakeWriter(const std::string&, std::function<void(int)> onError, const utils::Timeval&, std::size_t, const utils::Timeval&)
            : onError(std::move(onError)) {}
        bool enable(int) { return true; }
        void disable() { enabled = false; }
        bool isEnabled() const { return enabled; }
        void suspend() {}
        void setTimeout(const utils::Timeval&) {}
        void sendToPeer(const char*, std::size_t) {}
        bool streamToPeer(core::pipe::Source*) { return false; }
        void streamEof() {}
        void shutdownWrite(std::function<void()> onShutdown) { onShutdown(); }
        std::size_t getTotalSent() const { return 0; }
        std::size_t getTotalQueued() const { return 0; }
        void triggerError(int errnum) { onError(errnum); }
        bool shutdownInProgress = false;
        utils::Timeval terminateTimeout{};
    protected:
        virtual void doWriteShutdown(const std::function<void()>&) {}
        virtual void writeTimeout() {}
    private:
        bool enabled = true;
        std::function<void(int)> onError;
    };

    class TestConnection : public core::socket::stream::SocketConnectionT<FakePhysicalSocket, FakeReader, FakeWriter, FakeConfig> {
    public:
        TestConnection(FakePhysicalSocket socket, const std::shared_ptr<FakeConfig>& config)
            : SocketConnectionT(std::move(socket), []() {}, config) {}
        ~TestConnection() override = default;
        using SocketConnectionT::log;
        using SocketConnectionT::shutdownRead;
        using SocketConnectionT::shutdownWrite;
    };
}

int main() {
    tests::support::TestResult result;
    auto config = std::make_shared<FakeConfig>();

    const auto semanticPath = tempLogPath("snodec-migration01-semantic.log");
    {
        StateGuard guard(semanticPath.string());
        logger::LogManager::setGlobalLevel(logger::LogLevel::Debug);
        logger::LogManager::freeze();
        TestConnection connection(FakePhysicalSocket(101), config);
        connection.shutdownRead();
    }
    const auto semanticLog = readFile(semanticPath);
    result.expectTrue(semanticLog.find("Shutdown (RD): success") != std::string::npos, "migrated non-error SocketConnection.hpp call emits through semantic backend");
    result.expectTrue(semanticLog.find(" framework connection core.socket.stream ") != std::string::npos,
                      "migrated call carries framework/connection/core.socket.stream scope");

    const auto filterPath = tempLogPath("snodec-migration01-filter.log");
    {
        StateGuard guard(filterPath.string());
        logger::LogManager::setGlobalLevel(logger::LogLevel::Error);
        logger::LogManager::freeze();
        TestConnection connection(FakePhysicalSocket(102), config);
        connection.shutdownRead();
    }
    result.expectTrue(readFile(filterPath).find("Shutdown (RD): success") == std::string::npos, "migrated call respects LogManager filtering");

    const auto expensivePath = tempLogPath("snodec-migration01-expensive-suppressed.log");
    {
        StateGuard guard(expensivePath.string());
        logger::LogManager::setGlobalLevel(logger::LogLevel::Error);
        logger::LogManager::freeze();
        TestConnection connection(FakePhysicalSocket(106), config);
        int evaluations = 0;
        connection.log().debug("{}", ExpensiveValue{&evaluations});
        result.expectEqual(0, evaluations, "suppressed production SocketConnection formatting is not evaluated");
    }
    result.expectTrue(readFile(expensivePath).find("expensive") == std::string::npos, "suppressed production SocketConnection formatting emits no formatted output");

    const auto gatePath = tempLogPath("snodec-migration01-backend-gate.log");
    {
        StateGuard guard(gatePath.string());
        logger::LogManager::setGlobalLevel(logger::LogLevel::Trace);
        logger::LogManager::freeze();
        logger::Logger::setLogLevel(3);
        TestConnection connection(FakePhysicalSocket(103), config);
        connection.shutdownRead();
    }
    result.expectTrue(readFile(gatePath).find("Shutdown (RD): success") == std::string::npos, "migrated call respects Logger::setLogLevel backend filtering");

    const auto jsonPath = tempLogPath("snodec-migration01-json.log");
    {
        StateGuard guard(jsonPath.string());
        logger::LogManager::setGlobalLevel(logger::LogLevel::Debug);
        logger::LogManager::setFormat(logger::LogManager::Format::Json);
        logger::LogManager::freeze();
        FakePhysicalSocket socket(104);
        socket.shutdownResult = -1;
        TestConnection connection(std::move(socket), config);
        connection.shutdownRead();
    }
    const auto jsonLog = readFile(jsonPath);
    result.expectTrue(jsonLog.find("{\"v\":1") != std::string::npos && jsonLog.find("\"message\":\"Shutdown (RD)\"") != std::string::npos,
                      "migrated call respects Json format selection");
    result.expectTrue(jsonLog.find("\"code\":13") != std::string::npos && jsonLog.find("Permission denied") != std::string::npos,
                      "migrated sysError SocketConnection.hpp call preserves errno code and text");

    TestConnection sinkConnection(FakePhysicalSocket(105), config);
    std::vector<logger::LogRecord> records;
    sinkConnection.log([&](logger::LogRecord record) { records.push_back(std::move(record)); }).debug("sink overload still works");
    result.expectEqual(1, static_cast<int>(records.size()), "sink-taking overload remains compatible");
    result.expectTrue(records[0].message == "sink overload still works", "sink-taking overload captures message");

    return result.processResult();
}
