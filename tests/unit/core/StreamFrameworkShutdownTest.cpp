#include "core/DescriptorEventPublisher.h"
#include "core/EventLoop.h"
#include "core/EventMultiplexer.h"
#include "core/SNodeC.h"
#include "core/Shutdown.h"
#include "core/eventreceiver/ReadEventReceiver.h"
#include "core/socket/SocketAddress.h"
#include "core/socket/stream/SocketConnection.hpp"
#include "core/socket/stream/SocketContext.h"
#include "core/socket/stream/SocketReader.h"
#include "core/socket/stream/SocketWriter.h"
#include "core/timer/Timer.h"
#include "log/Logger.h"
#include "log/SemanticLogger.h"
#include "net/config/ConfigInstance.h"
#include "tests/support/TestResult.h"

#include <algorithm>
#include <cerrno>
#include <csignal>
#include <cstdint>
#include <fcntl.h>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iterator>
#include <list>
#include <memory>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <sys/socket.h>
#include <type_traits>
#include <unistd.h>
#include <utility>
#include <vector>

namespace {

    class PublisherContainerProbe : public core::DescriptorEventPublisher {
    public:
        using ReceiverMap = decltype(observedEventReceiverLists);
    };

    static_assert(
        std::is_same_v<typename PublisherContainerProbe::ReceiverMap::mapped_type, std::list<core::DescriptorEventReceiver*>>,
        "Descriptor shutdown traversal relies on stable std::list iterators");

    struct LifecycleState {
        int contextAttached = 0;
        int contextDetached = 0;
        int activeContextDestroyed = 0;
        int pendingContextDestroyed = 0;
        int signals = 0;
        int lastSignal = 0;
        int disconnects = 0;
        int connectionDestroyed = 0;
        int configDestroyed = 0;
        int timerAfterStopping = 0;
        int peerShutdownNotifications = 0;
        int peerDestroyed = 0;
        std::size_t peerBytes = 0;
        std::size_t totalSentAtDisconnect = 0;
        std::size_t totalQueuedAtDisconnect = 0;
        bool captureLifetimeLogs = false;
    };

    struct PhysicalCounters {
        int shutdownRead = 0;
        int shutdownWrite = 0;
        int shutdownBoth = 0;
        int closes = 0;
    };

    class SocketPair {
    public:
        SocketPair() {
            if (::socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, fds) == 0 && fds[0] > fds[1]) {
                std::swap(fds[0], fds[1]);
            }
        }

        ~SocketPair() {
            closeFd(fds[0]);
            closeFd(fds[1]);
        }

        int releaseConnectionFd() {
            return std::exchange(fds[0], -1);
        }

        int releasePeerFd() {
            return std::exchange(fds[1], -1);
        }

        bool valid() const {
            return fds[0] >= 0 && fds[1] >= 0;
        }

    private:
        static void closeFd(int fd) {
            if (fd >= 0) {
                ::close(fd);
            }
        }

        int fds[2] = {-1, -1};
    };

    class TestSocketAddress : public core::socket::SocketAddress {
    public:
        using SockAddr = sockaddr_storage;
        using SockLen = socklen_t;

        std::string toString(bool = true) const override {
            return "stream-framework-shutdown-address";
        }
    };

    class TestPhysicalSocket {
    public:
        using SocketAddress = TestSocketAddress;
        enum class SHUT { RD, WR, RDWR };

        TestPhysicalSocket(int fd, std::shared_ptr<PhysicalCounters> counters)
            : fd(fd)
            , counters(std::move(counters)) {
            const int flags = ::fcntl(fd, F_GETFL, 0);
            if (flags >= 0) {
                static_cast<void>(::fcntl(fd, F_SETFL, flags | O_NONBLOCK));
            }
        }

        TestPhysicalSocket(const TestPhysicalSocket&) = delete;
        TestPhysicalSocket& operator=(const TestPhysicalSocket&) = delete;

        TestPhysicalSocket(TestPhysicalSocket&& other) noexcept
            : fd(std::exchange(other.fd, -1))
            , counters(std::move(other.counters)) {
        }

        TestPhysicalSocket& operator=(TestPhysicalSocket&& other) noexcept {
            if (this != &other) {
                closeOwned();
                fd = std::exchange(other.fd, -1);
                counters = std::move(other.counters);
            }
            return *this;
        }

        ~TestPhysicalSocket() {
            closeOwned();
        }

        int getFd() const {
            return fd;
        }

        int getSockName(sockaddr_storage&, socklen_t& length) {
            length = sizeof(sockaddr_storage);
            return 0;
        }

        int getPeerName(sockaddr_storage&, socklen_t& length) {
            length = sizeof(sockaddr_storage);
            return 0;
        }

        const TestSocketAddress& getBindAddress() const {
            return address;
        }

        int shutdown(SHUT how) {
            if (how == SHUT::RD) {
                ++counters->shutdownRead;
            } else if (how == SHUT::WR) {
                ++counters->shutdownWrite;
            } else {
                ++counters->shutdownBoth;
            }
            return 0;
        }

    private:
        void closeOwned() {
            if (fd >= 0) {
                ++counters->closes;
                ::close(fd);
                fd = -1;
            }
        }

        int fd = -1;
        std::shared_ptr<PhysicalCounters> counters;
        TestSocketAddress address;
    };

    struct TestLocalConfig {
        static TestSocketAddress getSocketAddress(const sockaddr_storage&, socklen_t) {
            return {};
        }
    };

    struct TestRemoteConfig {
        static TestSocketAddress getSocketAddress(const sockaddr_storage&, socklen_t) {
            return {};
        }
    };

    class TestConfig
        : public net::config::ConfigInstance
        , public TestLocalConfig
        , public TestRemoteConfig {
    public:
        using Local = TestLocalConfig;
        using Remote = TestRemoteConfig;

        explicit TestConfig(const utils::Timeval& terminateTimeout)
            : ConfigInstance("stream-framework-shutdown", Role::CLIENT)
            , terminateTimeout(terminateTimeout) {
        }

        ~TestConfig() override = default;

        utils::Timeval getReadTimeout() const {
            return {1, 0};
        }

        utils::Timeval getWriteTimeout() const {
            return {1, 0};
        }

        utils::Timeval getTerminateTimeout() const {
            return terminateTimeout;
        }

        std::size_t getReadBlockSize() const {
            return 1024;
        }

        std::size_t getWriteBlockSize() const {
            return 256;
        }

    private:
        utils::Timeval terminateTimeout;
    };

    class TestReader : public core::socket::stream::SocketReader {
    protected:
        using core::socket::stream::SocketReader::SocketReader;

        ssize_t read(char* chunk, std::size_t chunkLen) override {
            return ::read(getRegisteredFd(), chunk, chunkLen);
        }

    public:
        ~TestReader() override = default;
    };

    class TestWriter : public core::socket::stream::SocketWriter {
    protected:
        using core::socket::stream::SocketWriter::SocketWriter;

        ssize_t write(const char* chunk, std::size_t chunkLen) override {
            return ::write(getRegisteredFd(), chunk, chunkLen);
        }

    public:
        ~TestWriter() override = default;
    };

    class TestConnection final
        : public core::socket::stream::SocketConnectionT<TestPhysicalSocket, TestReader, TestWriter, TestConfig> {
    private:
        using Super = core::socket::stream::SocketConnectionT<TestPhysicalSocket, TestReader, TestWriter, TestConfig>;

    public:
        TestConnection(TestPhysicalSocket&& physicalSocket,
                       const std::function<void(TestConnection*)>& onDisconnect,
                       std::uint64_t connectionId,
                       const std::shared_ptr<TestConfig>& config,
                       LifecycleState& state)
            : Super(
                  std::move(physicalSocket),
                  [this, onDisconnect]() {
                      onDisconnect(this);
                  },
                  connectionId,
                  config)
            , state(state) {
        }

        ~TestConnection() override {
            if (state.captureLifetimeLogs) {
                Super::log().info("final connection destructor cleanup");
            }
            ++state.connectionDestroyed;
        }

    private:
        LifecycleState& state;
    };

    class CountingContext final : public core::socket::stream::SocketContext {
    public:
        CountingContext(TestConnection* connection, LifecycleState& state, bool pending)
            : SocketContext(connection)
            , state(state)
            , pending(pending) {
        }

        ~CountingContext() override {
            if (state.captureLifetimeLogs) {
                frameworkLog().info(pending ? "final pending context destructor cleanup" : "final active context destructor cleanup");
            }
            if (pending) {
                ++state.pendingContextDestroyed;
            } else {
                ++state.activeContextDestroyed;
            }
        }

    private:
        void onConnected() override {
            ++state.contextAttached;
        }

        void onDisconnected() override {
            if (getDetachReason() == DetachReason::ConnectionClose) {
                ++state.contextDetached;
            }
            if (state.captureLifetimeLogs) {
                log().info("final application context onDisconnected");
                frameworkLog().info("final framework context onDisconnected");
            }
        }

        std::size_t onReceivedFromPeer() override {
            char bytes[1024];
            return readFromPeer(bytes, sizeof(bytes));
        }

        bool onSignal(int signum) override {
            ++state.signals;
            state.lastSignal = signum;
            return false;
        }

        void onWriteError(int errnum) override {
            core::socket::stream::SocketContext::onWriteError(errnum);
        }

        void onReadError(int errnum) override {
            core::socket::stream::SocketContext::onReadError(errnum);
        }

        LifecycleState& state;
        bool pending;
    };

    class CooperativePeer final : public core::eventreceiver::ReadEventReceiver {
    public:
        CooperativePeer(int fd,
                        std::size_t expectedBytes,
                        LifecycleState& state,
                        const std::shared_ptr<PhysicalCounters>& physical)
            : ReadEventReceiver("stream framework shutdown peer", utils::Timeval({0, 10000}))
            , fd(fd)
            , expectedBytes(expectedBytes)
            , state(state)
            , physical(physical) {
            const int flags = ::fcntl(fd, F_GETFL, 0);
            if (flags >= 0) {
                static_cast<void>(::fcntl(fd, F_SETFL, flags | O_NONBLOCK));
            }
            ReadEventReceiver::enable(fd);
        }

    private:
        ~CooperativePeer() override {
            if (fd >= 0) {
                ::close(fd);
            }
            ++state.peerDestroyed;
        }

        void readEvent() override {
            char bytes[4096];
            for (;;) {
                const ssize_t count = ::read(fd, bytes, sizeof(bytes));
                if (count > 0) {
                    state.peerBytes += static_cast<std::size_t>(count);
                } else if (count == 0) {
                    finish();
                    return;
                } else if (errno != EINTR && errno != EAGAIN && errno != EWOULDBLOCK) {
                    finish();
                    return;
                } else {
                    break;
                }
            }
            finishWhenComplete();
        }

        void readTimeout() override {
            finishWhenComplete();
        }

        void onShutdown(const core::ShutdownContext&) override {
            ++state.peerShutdownNotifications;
            finishWhenComplete();
        }

        void unobservedEvent() override {
            delete this;
        }

        void finishWhenComplete() {
            if (physical->shutdownWrite == 1 && state.peerBytes == expectedBytes) {
                finish();
            }
        }

        void finish() {
            if (ReadEventReceiver::isEnabled()) {
                ReadEventReceiver::disable();
            }
        }

        int fd;
        std::size_t expectedBytes;
        LifecycleState& state;
        std::shared_ptr<PhysicalCounters> physical;
    };

    struct Fixture {
        SocketPair pair;
        LifecycleState lifecycle;
        std::shared_ptr<PhysicalCounters> physical = std::make_shared<PhysicalCounters>();
        TestConnection* connection = nullptr;

        Fixture(bool pendingContext, const utils::Timeval& terminateTimeout, bool captureLifetimeLogs = false) {
            lifecycle.captureLifetimeLogs = captureLifetimeLogs;
            auto config = std::make_shared<TestConfig>(terminateTimeout);
            config->setOnDestroy([this](net::config::ConfigInstance* configInstance) {
                if (lifecycle.captureLifetimeLogs) {
                    configInstance->log().info("final config destroy callback");
                }
                ++lifecycle.configDestroyed;
            });
            connection = new TestConnection(
                TestPhysicalSocket(pair.releaseConnectionFd(), physical),
                [this](TestConnection* closingConnection) {
                    ++lifecycle.disconnects;
                    lifecycle.totalSentAtDisconnect = closingConnection->getTotalSent();
                    lifecycle.totalQueuedAtDisconnect = closingConnection->getTotalQueued();
                    connection = nullptr;
                },
                41,
                config,
                lifecycle);
            connection->setSocketContext(new CountingContext(connection, lifecycle, false));
            if (pendingContext) {
                connection->setSocketContext(new CountingContext(connection, lifecycle, true));
            }
        }
    };

    enum class Trigger { Requested, Signal, NoObserver };

    int runPlainScenario(Trigger trigger, bool queued, bool cooperative, bool pendingContext) {
        tests::support::TestResult result;
        char arg0[] = "StreamFrameworkShutdownTest";
        char* args[] = {arg0, nullptr};
        core::SNodeC::init(1, args);

        const utils::Timeval terminateTimeout = cooperative ? utils::Timeval({1, 0}) : utils::Timeval({0, 50000});
        Fixture fixture(pendingContext, terminateTimeout);
        result.expectTrue(fixture.pair.valid() || fixture.connection != nullptr, "socketpair and connection are available");

        const std::string payload = queued ? std::string(32 * 1024, 'Q') : std::string();
        if (queued) {
            fixture.connection->sendToPeer(payload);
        }
        if (cooperative) {
            static_cast<void>(new CooperativePeer(
                fixture.pair.releasePeerFd(), payload.size(), fixture.lifecycle, fixture.physical));
        }

        core::timer::Timer stopTimer = core::timer::Timer::singleshotTimer(
            [&fixture, trigger]() {
                if (trigger == Trigger::Signal) {
                    static_cast<void>(::kill(::getpid(), SIGTERM));
                    return;
                }

                core::SNodeC::stop();
                core::timer::Timer rejectedTimer = core::timer::Timer::singleshotTimer(
                    [&fixture]() {
                        ++fixture.lifecycle.timerAfterStopping;
                    },
                    utils::Timeval());
                if (trigger == Trigger::NoObserver) {
                    core::EventLoop::instance().getEventMultiplexer().shutdown({core::ShutdownReason::NoObserver, 0});
                }
            },
            utils::Timeval({0, 1000}));

        const int startResult = core::SNodeC::start(utils::Timeval({1, 0}));

        result.expectEqual(trigger == Trigger::Signal ? -SIGTERM : 0, startResult, "start result preserves shutdown trigger");
        result.expectEqual(1, fixture.lifecycle.contextAttached, "active context attaches once");
        result.expectEqual(1, fixture.lifecycle.contextDetached, "active context detaches once");
        result.expectEqual(1, fixture.lifecycle.activeContextDestroyed, "active context is destroyed once");
        result.expectEqual(pendingContext ? 1 : 0,
                           fixture.lifecycle.pendingContextDestroyed,
                           "pending context is destroyed exactly once when present");
        result.expectEqual(trigger == Trigger::Signal ? 1 : 0, fixture.lifecycle.signals, "signal callback count matches trigger");
        result.expectEqual(trigger == Trigger::Signal ? SIGTERM : 0, fixture.lifecycle.lastSignal, "signal number is preserved");
        result.expectEqual(1, fixture.physical->shutdownWrite, "all shutdown reasons use the write-shutdown path once");
        result.expectEqual(1, fixture.lifecycle.disconnects, "connection disconnect callback runs once");
        result.expectEqual(1, fixture.lifecycle.connectionDestroyed, "connection destructor runs once");
        result.expectEqual(1, fixture.physical->closes, "physical descriptor closes once");
        result.expectEqual(1, fixture.lifecycle.configDestroyed, "connection releases config before Config termination");
        result.expectEqual(0, fixture.lifecycle.timerAfterStopping, "new core timer is rejected while STOPPING");
        result.expectTrue(fixture.connection == nullptr, "connection is destroyed before start returns");

        if (queued) {
            result.expectEqual(static_cast<int>(payload.size()),
                               static_cast<int>(fixture.lifecycle.totalQueuedAtDisconnect),
                               "queued payload is recorded once");
            result.expectEqual(static_cast<int>(payload.size()),
                               static_cast<int>(fixture.lifecycle.totalSentAtDisconnect),
                               "queued payload drains before write shutdown");
            result.expectEqual(static_cast<int>(payload.size()),
                               static_cast<int>(fixture.lifecycle.peerBytes),
                               "cooperative peer receives the complete queued payload");
        }

        return result.processResult();
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
            logger::Logger::logToFile(logFile);
        }

        ~LoggerStateGuard() {
            logger::Logger::disableLogToFile();
            logger::Logger::init();
            logger::LogManager::init();
            errno = 0;
        }
    };

    struct CapturedRecord {
        std::string origin;
        std::string boundary;
        std::string component;
        std::optional<std::string> instance;
        std::optional<std::string> role;
        std::optional<std::string> connection;
        std::string message;
    };

    std::filesystem::path lifetimeLogPath() {
        const auto path = std::filesystem::temp_directory_path() / "snodec-stream-framework-shutdown-lifetime.jsonl";
        std::error_code error;
        std::filesystem::remove(path, error);
        std::filesystem::remove(path.string() + ".1", error);
        return path;
    }

    std::vector<CapturedRecord> readRecords(const std::filesystem::path& path) {
        std::vector<CapturedRecord> records;
        std::ifstream input(path);
        std::string line;
        while (std::getline(input, line)) {
            if (line.empty()) {
                continue;
            }
            const auto json = nlohmann::json::parse(line);
            records.push_back(CapturedRecord{
                .origin = json.at("origin").get<std::string>(),
                .boundary = json.at("boundary").get<std::string>(),
                .component = json.at("component").get<std::string>(),
                .instance = json.contains("instance") ? std::optional<std::string>(json.at("instance").get<std::string>()) : std::nullopt,
                .role = json.contains("role") ? std::optional<std::string>(json.at("role").get<std::string>()) : std::nullopt,
                .connection =
                    json.contains("connection") ? std::optional<std::string>(json.at("connection").get<std::string>()) : std::nullopt,
                .message = json.at("message").get<std::string>(),
            });
        }
        return records;
    }

    std::optional<std::size_t> findRecord(const std::vector<CapturedRecord>& records, const std::string& message) {
        for (std::size_t index = 0; index < records.size(); ++index) {
            if (records[index].message == message) {
                return index;
            }
        }
        return std::nullopt;
    }

    int countRecords(const std::vector<CapturedRecord>& records, const std::string& message) {
        return static_cast<int>(std::count_if(records.begin(), records.end(), [&message](const CapturedRecord& record) {
            return record.message == message;
        }));
    }

    void expectIdentity(tests::support::TestResult& result,
                        const CapturedRecord& record,
                        const std::string& origin,
                        const std::string& boundary,
                        const std::string& component,
                        const std::optional<std::string>& role,
                        const std::optional<std::string>& connection,
                        const std::string& label) {
        result.expectTrue(record.origin == origin, label + " retains origin identity");
        result.expectTrue(record.boundary == boundary, label + " retains boundary identity");
        result.expectTrue(record.component == component, label + " retains component identity");
        result.expectTrue(record.instance && *record.instance == "stream-framework-shutdown", label + " retains instance identity");
        result.expectTrue(record.role == role, label + " retains role identity where applicable");
        result.expectTrue(record.connection == connection, label + " retains connection identity where applicable");
    }

    int runLoggingLifetimeScenario() {
        tests::support::TestResult result;
        const auto logPath = lifetimeLogPath();
        LoggerStateGuard loggerGuard(logPath.string());

        char arg0[] = "StreamFrameworkShutdownTest";
        char arg1[] = "--log-level=6";
        char arg2[] = "--log-format=json";
        char arg3[] = "--quiet";
        char* args[] = {arg0, arg1, arg2, arg3, nullptr};
        core::SNodeC::init(4, args);

        Fixture fixture(true, utils::Timeval({1, 0}), true);
        static_cast<void>(new CooperativePeer(fixture.pair.releasePeerFd(), 0, fixture.lifecycle, fixture.physical));
        core::timer::Timer stopTimer = core::timer::Timer::singleshotTimer(
            []() {
                core::SNodeC::stop();
            },
            utils::Timeval({0, 1000}));

        result.expectEqual(0, core::SNodeC::start(utils::Timeval({1, 0})), "requested logging-lifetime shutdown completes");
        result.expectEqual(1, fixture.lifecycle.contextDetached, "active context detaches once before config termination");
        result.expectEqual(1, fixture.lifecycle.activeContextDestroyed, "active context is destroyed once before config termination");
        result.expectEqual(1, fixture.lifecycle.pendingContextDestroyed, "pending context is destroyed once before config termination");
        result.expectEqual(1, fixture.lifecycle.disconnects, "disconnect callback runs once before config termination");
        result.expectEqual(1, fixture.lifecycle.connectionDestroyed, "connection is destroyed once before config termination");
        result.expectEqual(1, fixture.lifecycle.configDestroyed, "connection-owned config is destroyed once before Config::terminate");
        result.expectTrue(fixture.connection == nullptr, "connection does not survive Config::terminate");

        logger::Logger::disableLogToFile();
        const std::vector<CapturedRecord> records = readRecords(logPath);
        const std::vector<std::string> lifetimeMessages = {
            "final application context onDisconnected",
            "final framework context onDisconnected",
            "final active context destructor cleanup",
            "final pending context destructor cleanup",
            "disconnected",
            "final connection destructor cleanup",
            "final config destroy callback",
        };
        const auto configShutdown = findRecord(records, "Core: Shutdown config system");
        result.expectTrue(configShutdown.has_value(), "Config::terminate phase is observable in captured semantic logs");
        for (const std::string& message : lifetimeMessages) {
            const auto lifetimeRecord = findRecord(records, message);
            result.expectEqual(1, countRecords(records, message), message + " occurs exactly once");
            result.expectTrue(lifetimeRecord && configShutdown && *lifetimeRecord < *configShutdown,
                              message + " occurs before the Config::terminate phase");
        }

        const auto expectRecordIdentity = [&](const std::string& message,
                                              const std::string& origin,
                                              const std::string& boundary,
                                              const std::string& component,
                                              const std::optional<std::string>& role,
                                              const std::optional<std::string>& connection) {
            const auto index = findRecord(records, message);
            result.expectTrue(index.has_value(), message + " is available for identity assertions");
            if (index) {
                expectIdentity(result, records[*index], origin, boundary, component, role, connection, message);
            }
        };

        expectRecordIdentity("final application context onDisconnected",
                             "application",
                             "context",
                             "core.socket.context",
                             std::nullopt,
                             std::optional<std::string>("41"));
        for (const std::string& message : {"final framework context onDisconnected",
                                          "final active context destructor cleanup",
                                          "final pending context destructor cleanup"}) {
            expectRecordIdentity(
                message, "framework", "context", "core.socket.context", std::nullopt, std::optional<std::string>("41"));
        }
        for (const std::string& message : {"disconnected", "final connection destructor cleanup"}) {
            expectRecordIdentity(
                message, "framework", "connection", "core.socket.stream", std::nullopt, std::optional<std::string>("41"));
        }
        expectRecordIdentity("final config destroy callback",
                             "framework",
                             "configuration",
                             "configuration",
                             std::optional<std::string>("client"),
                             std::nullopt);

        return result.processResult();
    }

} // namespace

int main(int argc, char* argv[]) {
    const std::string scenario = argc > 1 ? argv[1] : "plain_requested_empty";

    if (scenario == "plain_requested_empty") {
        return runPlainScenario(Trigger::Requested, false, true, true);
    }
    if (scenario == "plain_requested_queued") {
        return runPlainScenario(Trigger::Requested, true, true, false);
    }
    if (scenario == "plain_signal_false_veto") {
        return runPlainScenario(Trigger::Signal, false, true, false);
    }
    if (scenario == "plain_no_observer") {
        return runPlainScenario(Trigger::NoObserver, false, true, false);
    }
    if (scenario == "plain_noncooperating_timeout") {
        return runPlainScenario(Trigger::Requested, false, false, false);
    }
    if (scenario == "destruction_logging_order") {
        return runLoggingLifetimeScenario();
    }

    return 2;
}
