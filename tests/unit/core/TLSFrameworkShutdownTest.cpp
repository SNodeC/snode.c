#include "core/DescriptorEventPublisher.h"
#include "core/EventLoop.h"
#include "core/EventMultiplexer.h"
#include "core/SNodeC.h"
#include "core/Shutdown.h"
#include "core/eventreceiver/ReadEventReceiver.h"
#include "core/eventreceiver/WriteEventReceiver.h"
#include "core/socket/SocketAddress.h"
#include "core/socket/stream/SocketContext.h"
#include "core/socket/stream/tls/SocketConnection.hpp"
#include "core/socket/stream/tls/detail/TLSLifecycleTestAccess.h"
#include "core/timer/Timer.h"
#include "net/config/ConfigInstance.h"
#include "tests/support/TestResult.h"

#include <chrono>
#include <csignal>
#include <cstdint>
#include <fcntl.h>
#include <functional>
#include <memory>
#include <optional>
#include <openssl/ssl.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>
#include <utility>
#include <vector>

using core::socket::stream::tls::TLSShutdown;
using core::socket::stream::tls::detail::TLSLifecycleTestAccess;
using tests::support::TestResult;

namespace {

    struct LifecycleState {
        int attached = 0;
        int detached = 0;
        int contextsDestroyed = 0;
        int signals = 0;
        int lastSignal = 0;
        int rejectedTimerCallbacks = 0;
        int disconnects = 0;
        int configsDestroyed = 0;
        int descriptorsClosed = 0;
        int shutdownWriteCalls = 0;
        int readSentinelNotifications = 0;
        int writeSentinelNotifications = 0;
        int sentinelsDestroyed = 0;
    };

    class SocketPair {
    public:
        SocketPair() {
            static_cast<void>(::socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, fds));
            for (const int fd : fds) {
                const int flags = ::fcntl(fd, F_GETFL, 0);
                if (flags >= 0) {
                    static_cast<void>(::fcntl(fd, F_SETFL, flags | O_NONBLOCK));
                }
            }
        }

        ~SocketPair() {
            closeFd(fds[0]);
            closeFd(fds[1]);
        }

        bool valid() const {
            return fds[0] >= 0 && fds[1] >= 0;
        }

        int connectionFd() const {
            return fds[0];
        }

        int releaseConnectionFd() {
            return std::exchange(fds[0], -1);
        }

    private:
        static void closeFd(int fd) {
            if (fd >= 0) {
                static_cast<void>(::close(fd));
            }
        }

        int fds[2] = {-1, -1};
    };

    class TestSocketAddress : public core::socket::SocketAddress {
    public:
        using SockAddr = sockaddr_storage;
        using SockLen = socklen_t;

        std::string toString(bool = true) const override {
            return "tls-framework-shutdown-address";
        }
    };

    class TestPhysicalSocket {
    public:
        using SocketAddress = TestSocketAddress;
        enum class SHUT { RD, WR, RDWR };

        TestPhysicalSocket(int fd, const std::shared_ptr<LifecycleState>& state)
            : fd(fd)
            , state(state) {
        }

        TestPhysicalSocket(const TestPhysicalSocket&) = delete;
        TestPhysicalSocket& operator=(const TestPhysicalSocket&) = delete;

        TestPhysicalSocket(TestPhysicalSocket&& other) noexcept
            : fd(std::exchange(other.fd, -1))
            , state(std::move(other.state)) {
        }

        TestPhysicalSocket& operator=(TestPhysicalSocket&& other) noexcept {
            if (this != &other) {
                closeOwned();
                fd = std::exchange(other.fd, -1);
                state = std::move(other.state);
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
            if (how == SHUT::WR) {
                ++state->shutdownWriteCalls;
            }
            return 0;
        }

    private:
        void closeOwned() {
            if (fd >= 0) {
                ++state->descriptorsClosed;
                static_cast<void>(::close(fd));
                fd = -1;
            }
        }

        int fd = -1;
        std::shared_ptr<LifecycleState> state;
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

        TestConfig(const utils::Timeval& shutdownTimeout, const utils::Timeval& terminateTimeout)
            : ConfigInstance(nextInstanceName(), Role::CLIENT)
            , shutdownTimeout(shutdownTimeout)
            , terminateTimeout(terminateTimeout) {
        }

        utils::Timeval getInitTimeout() const {
            return {1, 0};
        }

        utils::Timeval getShutdownTimeout() const {
            return shutdownTimeout;
        }

        bool getNoCloseNotifyIsEOF() const {
            return false;
        }

        utils::Timeval getReadTimeout() const {
            return {10, 0};
        }

        utils::Timeval getWriteTimeout() const {
            return {10, 0};
        }

        utils::Timeval getTerminateTimeout() const {
            return terminateTimeout;
        }

        std::size_t getReadBlockSize() const {
            return 1024;
        }

        std::size_t getWriteBlockSize() const {
            return 1024;
        }

    private:
        static std::string nextInstanceName() {
            static int nextId = 0;
            return "tls-framework-shutdown-" + std::to_string(++nextId);
        }

        utils::Timeval shutdownTimeout;
        utils::Timeval terminateTimeout;
    };

    using TestConnection = core::socket::stream::tls::SocketConnection<TestPhysicalSocket, TestConfig>;

    class CountingContext final : public core::socket::stream::SocketContext {
    public:
        CountingContext(TestConnection* connection, const std::shared_ptr<LifecycleState>& state)
            : SocketContext(connection)
            , state(state) {
        }

        ~CountingContext() override {
            ++state->contextsDestroyed;
        }

    private:
        void onConnected() override {
            ++state->attached;
        }

        void onDisconnected() override {
            if (getDetachReason() == DetachReason::ConnectionClose) {
                ++state->detached;
            }
        }

        std::size_t onReceivedFromPeer() override {
            char buffer[1024];
            return readFromPeer(buffer, sizeof(buffer));
        }

        bool onSignal(int signum) override {
            ++state->signals;
            state->lastSignal = signum;
            rejectedTimer.emplace(core::timer::Timer::singleshotTimer(
                [state = state]() {
                    ++state->rejectedTimerCallbacks;
                },
                utils::Timeval()));
            return false;
        }

        void onWriteError(int errnum) override {
            core::socket::stream::SocketContext::onWriteError(errnum);
        }

        void onReadError(int errnum) override {
            core::socket::stream::SocketContext::onReadError(errnum);
        }

        std::shared_ptr<LifecycleState> state;
        std::optional<core::timer::Timer> rejectedTimer;
    };

    class ReadSentinel final : public core::eventreceiver::ReadEventReceiver {
    public:
        ReadSentinel(int fd, const std::shared_ptr<LifecycleState>& state)
            : ReadEventReceiver("TLS shutdown read traversal sentinel", utils::Timeval())
            , state(state) {
            ReadEventReceiver::enable(fd);
        }

    private:
        ~ReadSentinel() override {
            ++state->sentinelsDestroyed;
        }

        void readEvent() override {
        }

        void readTimeout() override {
        }

        void onShutdown(const core::ShutdownContext&) override {
            ++state->readSentinelNotifications;
        }

        void unobservedEvent() override {
            delete this;
        }

        std::shared_ptr<LifecycleState> state;
    };

    class WriteSentinel final : public core::eventreceiver::WriteEventReceiver {
    public:
        WriteSentinel(int fd, const std::shared_ptr<LifecycleState>& state)
            : WriteEventReceiver("TLS shutdown write traversal sentinel", utils::Timeval())
            , state(state) {
            WriteEventReceiver::enable(fd);
        }

    private:
        ~WriteSentinel() override {
            ++state->sentinelsDestroyed;
        }

        void writeEvent() override {
        }

        void writeTimeout() override {
        }

        void onShutdown(const core::ShutdownContext&) override {
            ++state->writeSentinelNotifications;
        }

        void unobservedEvent() override {
            delete this;
        }

        std::shared_ptr<LifecycleState> state;
    };

    void resetTlsState() {
        TLSLifecycleTestAccess::resetHandshake();
        TLSLifecycleTestAccess::resetShutdown();
        TLSLifecycleTestAccess::resetReader();
        TLSLifecycleTestAccess::resetWriter();
    }

    void releaseDisabledEvents() {
        const utils::Timeval now = utils::Timeval::currentTime();
        core::EventLoop::instance()
            .getEventMultiplexer()
            .getDescriptorEventPublisher(core::EventMultiplexer::DISP_TYPE::RD)
            .releaseDisabledEvents(now);
        core::EventLoop::instance()
            .getEventMultiplexer()
            .getDescriptorEventPublisher(core::EventMultiplexer::DISP_TYPE::WR)
            .releaseDisabledEvents(now);
    }

    void initializeFramework() {
        static char arg0[] = "TLSFrameworkShutdownTest";
        static char* args[] = {arg0, nullptr};
        core::SNodeC::init(1, args);
    }

    struct Fixture {
        SocketPair pair;
        std::shared_ptr<LifecycleState> state;
        TestConnection* connection = nullptr;
        SSL_CTX* sslContext = SSL_CTX_new(TLS_method());

        Fixture(const std::shared_ptr<LifecycleState>& state,
                std::uint64_t connectionId,
                const utils::Timeval& shutdownTimeout,
                const utils::Timeval& terminateTimeout,
                bool installSentinels = false)
            : state(state) {
            if (installSentinels) {
                static_cast<void>(new ReadSentinel(pair.connectionFd(), state));
                static_cast<void>(new WriteSentinel(pair.connectionFd(), state));
            }

            auto config = std::make_shared<TestConfig>(shutdownTimeout, terminateTimeout);
            config->setOnDestroy([state](net::config::ConfigInstance*) {
                ++state->configsDestroyed;
            });
            connection = new TestConnection(
                TestPhysicalSocket(pair.releaseConnectionFd(), state),
                [this](TestConnection*) {
                    ++this->state->disconnects;
                    connection = nullptr;
                },
                connectionId,
                config);
            connection->setSocketContext(new CountingContext(connection, state));
            TLSLifecycleTestAccess::startSSL(*connection, connection->getFd(), sslContext);
            TLSLifecycleTestAccess::transitionTo(*connection, 2);
            TLSLifecycleTestAccess::transitionTo(*connection, 3);
        }

        ~Fixture() {
            if (connection != nullptr) {
                connection->close();
                releaseDisabledEvents();
            }
            SSL_CTX_free(sslContext);
        }
    };

    void terminateRemainingResources() {
        core::EventLoop::instance().getEventMultiplexer().terminate();
        core::EventLoop::instance().getEventMultiplexer().clearEventQueue();
    }

    int runActiveHelper(core::ShutdownReason reason, bool forceTermination = false) {
        TestResult result;
        initializeFramework();
        resetTlsState();

        const auto state = std::make_shared<LifecycleState>();
        Fixture fixture(state, 101, utils::Timeval({5, 0}), utils::Timeval({0, 50000}));
        result.expectTrue(fixture.connection != nullptr, "active-helper connection is available");

        TLSLifecycleTestAccess::enqueueShutdownResult(-1, SSL_ERROR_WANT_READ);
        TLSLifecycleTestAccess::doSSLShutdown(*fixture.connection);
        TLSShutdown* const helper = TLSLifecycleTestAccess::lastShutdown();
        result.expectTrue(helper != nullptr, "protocol shutdown creates one helper before framework shutdown");
        result.expectEqual(1, TLSLifecycleTestAccess::shutdownCounters().constructed, "one active helper is constructed");
        result.expectTrue(TLSLifecycleTestAccess::readEnabled(helper), "active helper is observing reads");

        TLSLifecycleTestAccess::enqueueShutdownResult(-1, SSL_ERROR_WANT_WRITE);
        TLSLifecycleTestAccess::readEvent(helper);
        result.expectTrue(TLSLifecycleTestAccess::readEnabled(helper), "active helper retains its read receiver");
        result.expectTrue(TLSLifecycleTestAccess::writeEnabled(helper), "active helper also registers its write receiver");

        core::SNodeC::stop();
        const int shutdownSignal = reason == core::ShutdownReason::Signal ? SIGTERM : 0;
        core::EventLoop::instance().getEventMultiplexer().shutdown({reason, shutdownSignal});

        const bool helperRetained = TLSLifecycleTestAccess::lastShutdown() == helper &&
                                    TLSLifecycleTestAccess::shutdownCounters().active == 1;
        result.expectTrue(helperRetained, "framework shutdown joins the existing helper");
        result.expectEqual(1, TLSLifecycleTestAccess::shutdownCounters().constructed, "framework shutdown does not duplicate helper");
        result.expectEqual(2, TLSLifecycleTestAccess::shutdownCounters().operationCalls, "dual helper notification does not restart helper");
        result.expectEqual(1, TLSLifecycleTestAccess::shutdownCounters().active, "active helper remains alive after notification");
        result.expectTrue(helperRetained && TLSLifecycleTestAccess::readEnabled(helper),
                          "Requested/NoObserver notification leaves helper enabled");
        result.expectEqual(reason == core::ShutdownReason::Signal ? 1 : 0,
                           state->signals,
                           "active-helper signal callback count matches the framework reason");
        result.expectEqual(shutdownSignal, state->lastSignal, "active-helper signal metadata is preserved");

        if (!forceTermination) {
            TLSLifecycleTestAccess::enqueueShutdownResult(1, SSL_ERROR_NONE);
            if (helperRetained) {
                TLSLifecycleTestAccess::writeEvent(helper);
            }
            releaseDisabledEvents();

            result.expectEqual(1, TLSLifecycleTestAccess::shutdownCounters().successes, "joined helper completes cooperatively");
            result.expectEqual(1, state->shutdownWriteCalls, "joined helper performs physical write shutdown once");
        } else {
            result.expectEqual(0, TLSLifecycleTestAccess::shutdownCounters().successes, "forced helper remains mid-shutdown");
        }

        terminateRemainingResources();
        result.expectEqual(1, TLSLifecycleTestAccess::shutdownCounters().releases, "joined helper release callback runs once");
        result.expectEqual(1, TLSLifecycleTestAccess::shutdownCounters().destroyed, "joined helper is destroyed once");
        result.expectEqual(0, TLSLifecycleTestAccess::shutdownCounters().active, "joined helper leaves no active helper");
        result.expectTrue(fixture.connection == nullptr, "forced local fallback releases the connection");
        result.expectEqual(1, state->detached, "active context detaches once");
        result.expectEqual(1, state->contextsDestroyed, "active context is destroyed once");
        result.expectEqual(1, state->disconnects, "disconnect callback runs once");
        result.expectEqual(1, state->descriptorsClosed, "physical descriptor closes once");
        result.expectEqual(1, state->configsDestroyed, "connection-owned configuration reference is released once");

        core::SNodeC::free();
        return result.processResult();
    }

    int runPublisherMutation(bool crossPublisher) {
        TestResult result;
        initializeFramework();
        resetTlsState();

        const auto state = std::make_shared<LifecycleState>();
        Fixture fixture(state, 102, utils::Timeval({5, 0}), utils::Timeval({0, 50000}), true);
        result.expectTrue(fixture.connection != nullptr, "publisher-mutation connection is available");

        TLSLifecycleTestAccess::enqueueShutdownResult(
            -1, crossPublisher ? SSL_ERROR_WANT_WRITE : SSL_ERROR_WANT_READ);
        core::SNodeC::stop();
        core::EventLoop::instance().getEventMultiplexer().shutdown({core::ShutdownReason::Requested, 0});

        TLSShutdown* const helper = TLSLifecycleTestAccess::lastShutdown();
        const bool helperCreated = helper != nullptr;
        result.expectTrue(helperCreated, "connection notification creates TLSShutdown during traversal");
        result.expectEqual(1, state->readSentinelNotifications, "pre-existing read receiver is notified exactly once");
        result.expectEqual(1, state->writeSentinelNotifications, "pre-existing write receiver is notified exactly once");
        result.expectEqual(1, TLSLifecycleTestAccess::shutdownCounters().constructed, "live insertion constructs one helper");
        result.expectEqual(1, TLSLifecycleTestAccess::shutdownCounters().operationCalls, "helper notification is continuation-safe");
        result.expectEqual(1, TLSLifecycleTestAccess::shutdownCounters().active, "inserted helper survives publisher shutdown");
        result.expectTrue(helperCreated &&
                              (crossPublisher ? TLSLifecycleTestAccess::writeEnabled(helper)
                                              : TLSLifecycleTestAccess::readEnabled(helper)),
                          "inserted helper remains enabled in its selected publisher");

        TLSLifecycleTestAccess::enqueueShutdownResult(1, SSL_ERROR_NONE);
        if (helperCreated) {
            if (crossPublisher) {
                TLSLifecycleTestAccess::writeEvent(helper);
            } else {
                TLSLifecycleTestAccess::readEvent(helper);
            }
        }
        releaseDisabledEvents();

        result.expectEqual(1, TLSLifecycleTestAccess::shutdownCounters().successes, "inserted helper progresses after shutdown traversal");
        result.expectEqual(1, TLSLifecycleTestAccess::shutdownCounters().releases, "inserted helper releases once");
        result.expectEqual(1, TLSLifecycleTestAccess::shutdownCounters().destroyed, "inserted helper is destroyed once");
        result.expectEqual(0, TLSLifecycleTestAccess::shutdownCounters().active, "inserted helper is not leaked");

        terminateRemainingResources();
        result.expectEqual(2, state->sentinelsDestroyed, "both traversal sentinels are destroyed once");
        result.expectEqual(1, state->disconnects, "publisher mutation connection disconnects once");
        result.expectEqual(1, state->contextsDestroyed, "publisher mutation context is destroyed once");
        result.expectEqual(1, state->descriptorsClosed, "publisher mutation descriptor closes once");

        core::SNodeC::free();
        return result.processResult();
    }

    int runSignalFalseTls() {
        TestResult result;
        initializeFramework();
        resetTlsState();

        const auto state = std::make_shared<LifecycleState>();
        Fixture fixture(state, 103, utils::Timeval({5, 0}), utils::Timeval({0, 50000}));

        TLSLifecycleTestAccess::enqueueShutdownResult(-1, SSL_ERROR_WANT_WRITE);
        TLSLifecycleTestAccess::enqueueShutdownResult(1, SSL_ERROR_NONE);
        core::timer::Timer signalTimer = core::timer::Timer::singleshotTimer(
            []() {
                static_cast<void>(::kill(::getpid(), SIGTERM));
            },
            utils::Timeval({0, 1000}));

        const int startResult = core::SNodeC::start(utils::Timeval({1, 0}));

        result.expectEqual(-SIGTERM, startResult, "signal-triggered start result is preserved");
        result.expectEqual(1, state->signals, "SocketContext signal callback runs exactly once");
        result.expectEqual(SIGTERM, state->lastSignal, "SocketContext receives SIGTERM metadata");
        result.expectEqual(0, state->rejectedTimerCallbacks, "new core timer is rejected during STOPPING");
        result.expectEqual(1, TLSLifecycleTestAccess::shutdownCounters().constructed, "false signal result still starts TLS shutdown");
        result.expectEqual(1, TLSLifecycleTestAccess::shutdownCounters().maxConcurrent, "signal shutdown never duplicates helper");
        result.expectEqual(2, TLSLifecycleTestAccess::shutdownCounters().operationCalls, "descriptor helper progresses without a core timer");
        result.expectEqual(1, TLSLifecycleTestAccess::shutdownCounters().successes, "signal TLS shutdown completes its injected operation");
        result.expectEqual(1, TLSLifecycleTestAccess::shutdownCounters().releases, "signal helper releases once");
        result.expectEqual(1, TLSLifecycleTestAccess::shutdownCounters().destroyed, "signal helper is destroyed once");
        result.expectEqual(1, state->shutdownWriteCalls, "false cannot veto physical write shutdown while STOPPING");
        result.expectEqual(1, state->detached, "signal context detaches once");
        result.expectEqual(1, state->contextsDestroyed, "signal context is destroyed once");
        result.expectEqual(1, state->disconnects, "signal connection disconnects once");
        result.expectEqual(1, state->descriptorsClosed, "signal connection descriptor closes once");
        result.expectTrue(fixture.connection == nullptr, "signal connection is gone before start returns");

        return result.processResult();
    }

    int runOuterTimeout(std::size_t connectionCount) {
        TestResult result;
        initializeFramework();
        resetTlsState();

        const auto state = std::make_shared<LifecycleState>();
        std::vector<std::unique_ptr<Fixture>> fixtures;
        fixtures.reserve(connectionCount);
        for (std::size_t index = 0; index < connectionCount; ++index) {
            fixtures.push_back(std::make_unique<Fixture>(
                state, 200 + index, utils::Timeval({5, 0}), utils::Timeval({5, 0})));
            TLSLifecycleTestAccess::enqueueShutdownResult(-1, SSL_ERROR_WANT_READ);
        }

        core::timer::Timer stopTimer = core::timer::Timer::singleshotTimer(
            []() {
                core::SNodeC::stop();
            },
            utils::Timeval({0, 1000}));

        const auto startedAt = std::chrono::steady_clock::now();
        const int startResult = core::SNodeC::start(utils::Timeval({1, 0}));
        const auto elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - startedAt).count();

        result.expectEqual(0, startResult, "explicit stop preserves start result");
        result.expectTrue(elapsed >= 1.5, "outer drain, rather than the longer TLS timeout, controls shutdown");
        result.expectTrue(elapsed < 4.0, "EventLoop outer drain remains bounded");
        result.expectEqual(static_cast<int>(connectionCount),
                           TLSLifecycleTestAccess::shutdownCounters().constructed,
                           "one helper is constructed per non-cooperating TLS connection");
        result.expectEqual(static_cast<int>(connectionCount),
                           TLSLifecycleTestAccess::shutdownCounters().maxConcurrent,
                           "all non-cooperating helpers coexist before forced terminate");
        result.expectEqual(static_cast<int>(connectionCount),
                           TLSLifecycleTestAccess::shutdownCounters().releases,
                           "every forced helper releases once");
        result.expectEqual(static_cast<int>(connectionCount),
                           TLSLifecycleTestAccess::shutdownCounters().destroyed,
                           "every forced helper is destroyed once");
        result.expectEqual(0, TLSLifecycleTestAccess::shutdownCounters().active, "forced termination leaves no active helper");
        result.expectEqual(0, TLSLifecycleTestAccess::shutdownCounters().timeouts, "five-second helper timeout does not extend outer drain");
        result.expectEqual(static_cast<int>(connectionCount), state->detached, "every forced context detaches once");
        result.expectEqual(static_cast<int>(connectionCount), state->contextsDestroyed, "every forced context is destroyed once");
        result.expectEqual(static_cast<int>(connectionCount), state->disconnects, "every forced connection disconnects once");
        result.expectEqual(static_cast<int>(connectionCount), state->descriptorsClosed, "every forced descriptor closes once");
        result.expectEqual(static_cast<int>(connectionCount), state->configsDestroyed, "every connection releases its config before return");
        for (const auto& fixture : fixtures) {
            result.expectTrue(fixture->connection == nullptr, "no forced TLS connection escapes cleanup");
        }

        return result.processResult();
    }

} // namespace

int main(int argc, char* argv[]) {
    const std::string scenario = argc > 1 ? argv[1] : "active_helper_requested";

    if (scenario == "active_helper_requested") {
        return runActiveHelper(core::ShutdownReason::Requested);
    }
    if (scenario == "active_helper_no_observer") {
        return runActiveHelper(core::ShutdownReason::NoObserver);
    }
    if (scenario == "active_helper_signal") {
        return runActiveHelper(core::ShutdownReason::Signal);
    }
    if (scenario == "active_helper_forced") {
        return runActiveHelper(core::ShutdownReason::Requested, true);
    }
    if (scenario == "publisher_mutation_same_list") {
        return runPublisherMutation(false);
    }
    if (scenario == "publisher_mutation_cross_publisher") {
        return runPublisherMutation(true);
    }
    if (scenario == "signal_false_tls") {
        return runSignalFalseTls();
    }
    if (scenario == "outer_timeout_long") {
        return runOuterTimeout(1);
    }
    if (scenario == "outer_timeout_multiple") {
        return runOuterTimeout(2);
    }

    return 2;
}
