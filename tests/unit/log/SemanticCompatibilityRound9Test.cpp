#include "core/socket/SocketAddress.h"
#include "core/socket/stream/SocketConnection.h"
#include "core/socket/stream/SocketContext.h"
#include "log/Logger.h"
#include "log/SemanticLogger.h"
#include "tests/support/TestResult.h"

#include <cerrno>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <system_error>
#include <vector>

namespace {
    std::chrono::system_clock::time_point fixedTimestamp() {
        using namespace std::chrono;
        return system_clock::time_point{seconds{1783254896} + milliseconds{789}};
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
            logger::Logger::setTickResolver([]() { return std::string("ROUND9TICK000"); });
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

    logger::LogScope scope(std::string_view component = "round9.component", std::string_view instance = "round9-instance") {
        return {logger::LogOrigin::Framework, logger::LogBoundary::System, component, instance, logger::LogRole::Server, "round9-connection"};
    }

    logger::LogRecord record(logger::LogLevel level, std::string message) {
        logger::LogRecordOptions options;
        options.ts = fixedTimestamp();
        return logger::materialize(scope(), level, std::move(message), std::move(options));
    }

    class TestSocketConnection : public core::socket::stream::SocketConnection {
    public:
        explicit TestSocketConnection(const std::string& instanceName) : SocketConnection(9, instanceName, nullptr) {}
        ~TestSocketConnection() override = default;
        using core::socket::stream::SocketConnection::setSocketContext;
        int getFd() const override { return 9; }
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
        explicit TestSocketContext(core::socket::stream::SocketConnection* socketConnection) : SocketContext(socketConnection) {}
        ~TestSocketContext() override = default;
        void exposeReadError(int errnum) { onReadError(errnum); }
        void exposeWriteError(int errnum) { onWriteError(errnum); }
    private:
        std::size_t onReceivedFromPeer() override { return 0; }
        bool onSignal(int) override { return false; }
        void onConnected() override {}
        void onDisconnected() override {}
    };
}

int main() {
    tests::support::TestResult result;

    const auto legacyPath = tempLogPath("snodec-round9-legacy.log");
    {
        StateGuard guard(legacyPath.string());
        LOG(INFO) << "round9 legacy info";
        errno = EACCES;
        PLOG(ERROR) << "round9 legacy plog";
        VLOG(1) << "round9 verbose hidden";
        logger::Logger::setVerboseLevel(1);
        VLOG(1) << "round9 verbose visible";
        logger::Logger::setLogLevel(3);
        LOG(INFO) << "round9 numeric info hidden";
        LOG(WARNING) << "round9 numeric warning visible";
    }
    const auto legacyLog = readFile(legacyPath);
    result.expectTrue(legacyLog.find("round9 legacy info") != std::string::npos, "LOG(INFO) emits through legacy backend");
    result.expectTrue(legacyLog.find("round9 legacy plog") != std::string::npos && legacyLog.find("Permission denied") != std::string::npos,
                      "PLOG(ERROR) preserves errno text through legacy backend");
    result.expectTrue(legacyLog.find("round9 verbose hidden") == std::string::npos && legacyLog.find("round9 verbose visible") != std::string::npos,
                      "VLOG respects Logger::setVerboseLevel");
    result.expectTrue(legacyLog.find("round9 numeric info hidden") == std::string::npos && legacyLog.find("round9 numeric warning visible") != std::string::npos,
                      "Logger::setLogLevel numeric thresholds remain compatible");

    const auto independencePath = tempLogPath("snodec-round9-independent.log");
    {
        StateGuard guard(independencePath.string());
        logger::LogManager::setGlobalLevel(logger::LogLevel::Off);
        logger::LogManager::freeze();
        LOG(ERROR) << "legacy unaffected by semantic off";
        logger::Logger::emitSemantic(record(logger::LogLevel::Error, "semantic filtered by off"));
    }
    const auto independenceLog = readFile(independencePath);
    result.expectTrue(independenceLog.find("legacy unaffected by semantic off") != std::string::npos && independenceLog.find("semantic filtered by off") == std::string::npos,
                      "LogManager filtering remains independent from legacy macros");

    const auto semanticPath = tempLogPath("snodec-round9-semantic.log");
    {
        StateGuard guard(semanticPath.string());
        logger::BoundaryLogger::createForTest(scope(), logger::Logger::semanticSink(), logger::LogLevel::Trace, fixedTimestamp).info("default semantic backend");
    }
    result.expectTrue(readFile(semanticPath).find("default semantic backend") != std::string::npos, "default no-argument semantic sink emits through backend");

    std::vector<logger::LogRecord> captured;
    logger::BoundaryLogger::createForTest(scope(), [&](logger::LogRecord capturedRecord) { captured.push_back(std::move(capturedRecord)); }, logger::LogLevel::Trace, fixedTimestamp)
        .info("captured only");
    result.expectEqual(1, static_cast<int>(captured.size()), "sink-taking semantic overload captures records");

    bool froze = false;
    logger::LogManager::init();
    logger::LogManager::setGlobalLevel(logger::LogLevel::Warn);
    logger::LogManager::freeze();
    try { logger::LogManager::setGlobalLevel(logger::LogLevel::Trace); } catch (const std::logic_error&) { froze = true; }
    result.expectTrue(froze && logger::LogManager::effectiveLevel(scope()) == logger::LogLevel::Warn, "LogManager freeze remains immutable");
    logger::LogManager::init();

    const auto text = logger::formatText(record(logger::LogLevel::Info, "text format message"));
    result.expectTrue(logger::LogManager::formatRecord(record(logger::LogLevel::Info, "text format message")) == text, "Text format uses formatText");
    logger::LogManager::setFormat(logger::LogManager::Format::Json);
    const auto jsonRecord = record(logger::LogLevel::Info, "json format message");
    const auto json = logger::LogManager::formatRecord(jsonRecord);
    result.expectTrue(json == logger::formatJsonV1(jsonRecord), "Json format uses formatJsonV1");
    result.expectTrue(json.find("\"v\":1") != std::string::npos && json.find("\"ts\":") != std::string::npos && json.find("\"level\":\"info\"") != std::string::npos &&
                          json.find("\"origin\":\"framework\"") != std::string::npos && json.find("\"boundary\":\"system\"") != std::string::npos &&
                          json.find("\"component\":\"round9.component\"") != std::string::npos && json.find("\"message\":\"json format message\"") != std::string::npos,
                      "JSON v1 mandatory fields are present");
    const auto sparseJson = logger::formatJsonV1(logger::materialize(scope("round9.sparse", ""), logger::LogLevel::Info, "sparse", {std::nullopt, std::nullopt, std::nullopt, fixedTimestamp()}));
    result.expectTrue(sparseJson.find("\"instance\"") == std::string::npos && sparseJson.find("\"event\"") == std::string::npos && sparseJson.find("null") == std::string::npos,
                      "JSON v1 optional fields are omitted when absent");
    logger::LogManager::init();

    std::vector<logger::LogRecord> errors;
    logger::BoundaryLogger::createForTest(scope(), [&](logger::LogRecord errorRecord) { errors.push_back(std::move(errorRecord)); }, logger::LogLevel::Trace, fixedTimestamp)
        .sysError(logger::LogLevel::Error, EACCES, "semantic sysError");
    result.expectTrue(errors.size() == 1 && errors[0].error && errors[0].error->code == EACCES && errors[0].error->text.find("Permission denied") != std::string::npos,
                      "sysError preserves errno code and error text");

    std::vector<logger::LogRecord> borrowed;
    std::string component = "borrowed.component";
    auto borrowedLogger = logger::BoundaryLogger::createForTest(scope(component, "borrowed-instance"), [&](logger::LogRecord borrowedRecord) { borrowed.push_back(std::move(borrowedRecord)); }, logger::LogLevel::Trace, fixedTimestamp);
    component.assign("mutated.component");
    borrowedLogger.info("borrowed safe");
    result.expectTrue(borrowed.size() == 1 && borrowed[0].component == "borrowed.component", "borrowed LogScope string_views are copied before sink use");

    const auto migratedPath = tempLogPath("snodec-round9-migrated.log");
    {
        StateGuard guard(migratedPath.string());
        logger::LogManager::setGlobalLevel(logger::LogLevel::Debug);
        logger::LogManager::setFormat(logger::LogManager::Format::Json);
        logger::LogManager::freeze();
        TestSocketConnection connection("round9-instance");
        auto* first = new TestSocketContext(&connection);
        auto* second = new TestSocketContext(&connection);
        connection.setSocketContext(first);
        connection.setSocketContext(second);
        TestSocketContext context(&connection);
        context.exposeReadError(0);
        context.exposeWriteError(EACCES);
        delete first;
        delete second;
    }
    const auto migratedLog = readFile(migratedPath);
    result.expectTrue(migratedLog.find("SocketContext: switch") != std::string::npos, "Round 8 SocketConnection migrated call emits semantically");
    result.expectTrue(migratedLog.find("SocketContext: EOF received") != std::string::npos && migratedLog.find("\"origin\":\"framework\"") != std::string::npos,
                      "Round 8 SocketContext frameworkLog call emits with framework origin");
    result.expectTrue(migratedLog.find("SocketContext: onWriteError") != std::string::npos && migratedLog.find("\"code\":13") != std::string::npos &&
                          migratedLog.find("Permission denied") != std::string::npos,
                      "Round 8 migrated sysError call preserves errno code/text");

    return result.processResult();
}
