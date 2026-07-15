#include "core/socket/SocketAddress.h"
#include "core/socket/stream/SocketConnection.h"
#include "core/socket/stream/SocketContext.h"
#include "log/Logger.h"
#include "tests/support/TestResult.h"

#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <optional>
#include <ostream>
#include <string>
#include <utility>
#include <vector>

namespace core::socket::stream::detail {

    struct ContextLifecycleTestAccess {
        static void attach(SocketContext* context) {
            context->attach();
        }

        static void detachForContextSwitch(SocketContext* context) {
            context->detach(SocketContext::DetachReason::ContextSwitch);
        }

        static void detachForConnectionClose(SocketContext* context) {
            context->detach(SocketContext::DetachReason::ConnectionClose);
        }
    };

} // namespace core::socket::stream::detail

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
            logger::Logger::setTickResolver([]() {
                return std::string("CONTEXTLIFECYCLEPHASE1TICK");
            });
            logger::Logger::logToFile(logFile);
            logger::LogManager::setGlobalLevel(logger::LogLevel::Debug);
            logger::LogManager::setFormat(logger::LogManager::Format::Json);
            logger::LogManager::freeze();
        }

        ~LoggerStateGuard() {
            logger::Logger::disableLogToFile();
            logger::Logger::setTickResolver({});
            logger::Logger::init();
            logger::LogManager::init();
            errno = 0;
        }
    };

    class TestSocketAddress : public core::socket::SocketAddress {
    public:
        explicit TestSocketAddress(std::string name)
            : name(std::move(name)) {
        }

        std::string toString(bool = true) const override {
            return name;
        }

    private:
        std::string name;
    };

    class TestSocketConnection : public core::socket::stream::SocketConnection {
    public:
        TestSocketConnection(std::string instanceName, std::uint64_t connectionId)
            : SocketConnection(42, connectionId, instanceName, nullptr)
            , bindAddress("bind")
            , localAddress("local")
            , remoteAddress("remote") {
        }

        ~TestSocketConnection() override = default;

        int getFd() const override {
            return 42;
        }

        void sendToPeer(const char*, std::size_t chunkLen) override {
            totalQueued += chunkLen;
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
            return bindAddress;
        }

        const core::socket::SocketAddress& getLocalAddress() const override {
            return localAddress;
        }

        const core::socket::SocketAddress& getRemoteAddress() const override {
            return remoteAddress;
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
            return totalQueued;
        }

        std::size_t getTotalRead() const override {
            return 0;
        }

        std::size_t getTotalProcessed() const override {
            return totalProcessed;
        }

    private:
        TestSocketAddress bindAddress;
        TestSocketAddress localAddress;
        TestSocketAddress remoteAddress;
        std::size_t totalQueued = 13;
        std::size_t totalProcessed = 7;
    };

    class TestSocketContext : public core::socket::stream::SocketContext {
    public:
        using ObservedDetachReason = DetachReason;

        struct ObservedLifecycleState {
            std::optional<ObservedDetachReason> reasonSeenDuringCallback;
            int connectedCalls = 0;
            int disconnectedCalls = 0;
        };

        TestSocketContext(core::socket::stream::SocketConnection* socketConnection, std::string label, ObservedLifecycleState& state)
            : SocketContext(socketConnection)
            , label(std::move(label))
            , state(state) {
        }

        ~TestSocketContext() override = default;

    private:
        std::size_t onReceivedFromPeer() override {
            return 0;
        }

        bool onSignal(int) override {
            return true;
        }

        void onConnected() override {
            ++state.connectedCalls;
            log().debug("{}: context attached", label);
        }

        void onDisconnected() override {
            ++state.disconnectedCalls;
            state.reasonSeenDuringCallback = getDetachReason();
            log().debug(
                "{}: context detached for {}",
                label,
                getDetachReason() == DetachReason::ContextSwitch ? "context switch" : "connection close");
        }

        std::string label;
        ObservedLifecycleState& state;
    };

    struct CapturedRecord {
        std::string level;
        std::string origin;
        std::string boundary;
        std::string component;
        std::optional<std::string> instance;
        std::optional<std::string> connection;
        std::string message;
    };

    std::filesystem::path tempLogPath() {
        const auto path = std::filesystem::temp_directory_path() / "snodec-context-lifecycle-phase1.jsonl";
        std::error_code error;
        std::filesystem::remove(path, error);
        std::filesystem::remove(path.string() + ".1", error);
        return path;
    }

    std::vector<CapturedRecord> readRecords(const std::filesystem::path& path) {
        std::vector<CapturedRecord> records;
        std::ifstream in(path);
        std::string line;
        while (std::getline(in, line)) {
            if (line.empty()) {
                continue;
            }
            const auto json = nlohmann::json::parse(line);
            records.push_back(CapturedRecord{.level = json.at("level").get<std::string>(),
                                             .origin = json.at("origin").get<std::string>(),
                                             .boundary = json.at("boundary").get<std::string>(),
                                             .component = json.at("component").get<std::string>(),
                                             .instance = json.contains("instance") ? std::optional<std::string>(json.at("instance").get<std::string>())
                                                                                   : std::nullopt,
                                             .connection = json.contains("connection")
                                                               ? std::optional<std::string>(json.at("connection").get<std::string>())
                                                               : std::nullopt,
                                             .message = json.at("message").get<std::string>()});
        }
        return records;
    }

    int countMessage(const std::vector<CapturedRecord>& records, const std::string& message) {
        int count = 0;
        for (const auto& record : records) {
            if (record.message == message) {
                ++count;
            }
        }
        return count;
    }

    std::optional<std::size_t> findMessage(const std::vector<CapturedRecord>& records, const std::string& message, std::size_t start = 0) {
        for (std::size_t i = start; i < records.size(); ++i) {
            if (records[i].message == message) {
                return i;
            }
        }
        return std::nullopt;
    }

    void expectContextIdentity(tests::support::TestResult& result,
                               const CapturedRecord& record,
                               const std::string& expectedOrigin,
                               const std::string& expectedInstance,
                               const std::string& expectedConnection,
                               const std::string& label) {
        result.expectTrue(record.level == "debug", label + " uses Debug level");
        result.expectTrue(record.origin == expectedOrigin, label + " uses expected origin");
        result.expectTrue(record.boundary == "context", label + " uses context boundary");
        result.expectTrue(record.component == "core.socket.context", label + " uses context component");
        result.expectTrue(record.instance && *record.instance == expectedInstance, label + " carries instance identity");
        result.expectTrue(record.connection && *record.connection == expectedConnection, label + " carries connection identity");
    }

} // namespace

int main() {
    tests::support::TestResult result;

    const std::string expectedInstance = "phase1-instance";
    const std::string expectedConnection = "99";
    TestSocketConnection connection(expectedInstance, 99);
    TestSocketContext::ObservedLifecycleState oldState;
    TestSocketContext::ObservedLifecycleState replacementState;

    const auto logPath = tempLogPath();
    {
        LoggerStateGuard guard(logPath.string());

        auto* oldContext = new TestSocketContext(&connection, "Old context", oldState);
        core::socket::stream::detail::ContextLifecycleTestAccess::attach(oldContext);
        core::socket::stream::detail::ContextLifecycleTestAccess::detachForContextSwitch(oldContext);

        auto* replacementContext = new TestSocketContext(&connection, "Replacement context", replacementState);
        core::socket::stream::detail::ContextLifecycleTestAccess::attach(replacementContext);
        core::socket::stream::detail::ContextLifecycleTestAccess::detachForConnectionClose(replacementContext);
    }

    const auto records = readRecords(logPath);
    result.expectEqual(12, static_cast<int>(records.size()), "real attach/detach lifecycle emits twelve records including context-switch diagnostics");

    const auto firstGenericAttach = findMessage(records, "Context attached");
    const auto oldAttach = findMessage(records, "Old context: context attached");
    const auto oldDetach = findMessage(records, "Old context: context detached for context switch");
    const auto genericSwitchDetach = findMessage(records, "Context detached for context switch");
    const auto secondGenericAttach = firstGenericAttach ? findMessage(records, "Context attached", *firstGenericAttach + 1) : std::nullopt;
    const auto replacementAttach = findMessage(records, "Replacement context: context attached");
    const auto replacementDetach = findMessage(records, "Replacement context: context detached for connection close");
    const auto genericCloseDetach = findMessage(records, "Context detached for connection close");

    result.expectTrue(firstGenericAttach && oldAttach && *firstGenericAttach < *oldAttach, "initial attach is generic before derived");
    result.expectTrue(oldDetach && genericSwitchDetach && *oldDetach < *genericSwitchDetach, "context-switch detach is derived before generic");
    result.expectTrue(secondGenericAttach && replacementAttach && *secondGenericAttach < *replacementAttach,
                      "replacement attach is generic before derived");
    result.expectTrue(replacementDetach && genericCloseDetach && *replacementDetach < *genericCloseDetach,
                      "connection-close detach is derived before generic");

    for (const std::string& message : {"Context attached",
                                       "Old context: context attached",
                                       "Old context: context detached for context switch",
                                       "Context detached for context switch",
                                       "Replacement context: context attached",
                                       "Replacement context: context detached for connection close",
                                       "Context detached for connection close"}) {
        result.expectEqual(message == std::string("Context attached") ? 2 : 1, countMessage(records, message), "exact lifecycle count for " + message);
    }

    result.expectEqual(1, oldState.connectedCalls, "old context connected callback ran once");
    result.expectEqual(1, oldState.disconnectedCalls, "old context disconnected callback ran once");
    result.expectEqual(1, replacementState.connectedCalls, "replacement context connected callback ran once");
    result.expectEqual(1, replacementState.disconnectedCalls, "replacement context disconnected callback ran once");
    result.expectTrue(oldState.reasonSeenDuringCallback && *oldState.reasonSeenDuringCallback == TestSocketContext::ObservedDetachReason::ContextSwitch,
                      "old context observed ContextSwitch during onDisconnected");
    result.expectTrue(replacementState.reasonSeenDuringCallback &&
                          *replacementState.reasonSeenDuringCallback == TestSocketContext::ObservedDetachReason::ConnectionClose,
                      "replacement context observed ConnectionClose during onDisconnected");

    for (const auto& record : records) {
        if (record.message.rfind("Old context:", 0) == 0 || record.message.rfind("Replacement context:", 0) == 0) {
            expectContextIdentity(result, record, "application", expectedInstance, expectedConnection, record.message);
        } else if (record.message == "Context attached" || record.message == "Context detached for context switch" ||
                   record.message == "Context detached for connection close" || record.message.rfind("  Context ", 0) == 0) {
            expectContextIdentity(result, record, "framework", expectedInstance, expectedConnection, record.message);
        }
    }

    for (const std::string& obsolete : {"HTTP: Received disconnect",
                                        "HTTP: Connected",
                                        "Echo connected",
                                        "Echo disconnected",
                                        "TLS legacy connected",
                                        "TLS legacy disconnected",
                                        "WebSocketContext: Subprotocol",
                                        "SocketContext: detached"}) {
        bool found = false;
        for (const auto& record : records) {
            found = found || record.message.find(obsolete) != std::string::npos;
        }
        result.expectTrue(!found, "obsolete wording absent from runtime records: " + obsolete);
    }

    return result.processResult();
}
