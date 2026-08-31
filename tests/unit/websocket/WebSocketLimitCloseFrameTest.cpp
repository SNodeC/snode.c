#include "core/socket/SocketAddress.h"
#include "core/socket/stream/SocketConnection.h"
#include "core/socket/stream/SocketContext.h"
#include "net/config/ConfigInstance.h"
#include "support/TestResult.h"
#include "utils/Timeval.h"
#include "web/http/ConfigWebSocket.h"
#include "web/http/SocketContextUpgradeFactory.hpp"
#include "web/websocket/SocketContextUpgrade.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace core::socket::stream::detail {
    struct ContextLifecycleTestAccess {
        static std::size_t receive(SocketContext* context) {
            return context->readFromPeer();
        }
    };
} // namespace core::socket::stream::detail

namespace {
    struct DummyRequest {};
    struct DummyResponse {};

    struct DummySubProtocol {
        void attach() {
        }
        void detach() {
        }
        void onMessageStart(int) {
            ++messageStarts;
        }
        void onMessageData(const char*, std::size_t) {
        }
        void onMessageEnd() {
        }
        void onMessageError(uint16_t status) {
            ++messageErrors;
            lastError = status;
        }
        void onPongReceived() {
        }
        bool onSignal(int) {
            return true;
        }

        std::string name{"limit-close-frame-test"};
        int messageStarts = 0;
        int messageErrors = 0;
        uint16_t lastError = 0;
    };

    class TestUpgradeFactory : public web::http::SocketContextUpgradeFactory<DummyRequest, DummyResponse> {
    public:
        std::string name() override {
            return "limit-close-frame-test";
        }

    private:
        web::http::SocketContextUpgrade<DummyRequest, DummyResponse>*
        create(core::socket::stream::SocketConnection*, DummyRequest*, DummyResponse*) override {
            return nullptr;
        }

        void checkRefCount() override {
        }
    };

    class TestConfigInstance : public net::config::ConfigInstance {
    public:
        TestConfigInstance()
            : ConfigInstance("", Role::SERVER) {
            newSubCommand<web::http::ConfigWebSocket>()->setMaximumFrameBytes(2);
        }

        ~TestConfigInstance() override = default;
    };

    class TestSocketAddress : public core::socket::SocketAddress {
    public:
        std::string toString(bool = true) const override {
            return "websocket-limit-close-frame-test";
        }
    };

    class TestSocketConnection : public core::socket::stream::SocketConnection {
    public:
        explicit TestSocketConnection(const net::config::ConfigInstance* config)
            : SocketConnection(-1, 1009, "websocket-limit-close-frame-test", config) {
        }

        ~TestSocketConnection() override = default;

        int getFd() const override {
            return -1;
        }
        void sendToPeer(const char* chunk, std::size_t chunkLen) override {
            output.insert(output.end(), chunk, chunk + chunkLen);
        }
        bool streamToPeer(core::pipe::Source*) override {
            return false;
        }
        void streamEof() override {
        }
        std::size_t readFromPeer(char* chunk, std::size_t chunkLen) override {
            const std::size_t available = input.size() - inputOffset;
            const std::size_t readLength = std::min(chunkLen, available);
            if (readLength > 0) {
                std::memcpy(chunk, input.data() + inputOffset, readLength);
                inputOffset += readLength;
            }
            return readLength;
        }
        void shutdownRead() override {
            ++shutdownReadCount;
        }
        void shutdownWrite() override {
        }
        const core::socket::SocketAddress& getBindAddress() const override {
            return address();
        }
        const core::socket::SocketAddress& getLocalAddress() const override {
            return address();
        }
        const core::socket::SocketAddress& getRemoteAddress() const override {
            return address();
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
            return output.size();
        }
        std::size_t getTotalQueued() const override {
            return output.size();
        }
        std::size_t getTotalRead() const override {
            return inputOffset;
        }
        std::size_t getTotalProcessed() const override {
            return inputOffset;
        }

        std::vector<char> input;
        std::vector<char> output;
        int shutdownReadCount = 0;

    private:
        static const core::socket::SocketAddress& address() {
            static const TestSocketAddress testAddress;
            return testAddress;
        }

        std::size_t inputOffset = 0;
    };

    class TestSocketContextUpgrade : public web::websocket::SocketContextUpgrade<DummySubProtocol, DummyRequest, DummyResponse> {
    private:
        using Super = web::websocket::SocketContextUpgrade<DummySubProtocol, DummyRequest, DummyResponse>;

    public:
        TestSocketContextUpgrade(TestSocketConnection* connection, TestUpgradeFactory* factory)
            : Super(connection, factory, Role::SERVER) {
            subProtocol = &testSubProtocol;
        }

        DummySubProtocol testSubProtocol;
    };
} // namespace

int main() {
    tests::support::TestResult result;
    TestConfigInstance config;
    TestSocketConnection connection(&config);
    TestUpgradeFactory factory;
    TestSocketContextUpgrade context(&connection, &factory);

    // A masked client text frame declaring three payload bytes. The configured
    // frame limit is two bytes, so the receiver rejects it from the header.
    connection.input = {static_cast<char>(0x81), static_cast<char>(0x83), 0x01, 0x02, 0x03, 0x04, 'a', 'b', 'c'};

    const std::size_t consumed = core::socket::stream::detail::ContextLifecycleTestAccess::receive(&context);

    const std::vector<char> expectedCloseFrame = {
        static_cast<char>(0x88), static_cast<char>(0x02), static_cast<char>(0x03), static_cast<char>(0xF1)};
    result.expectEqual(std::size_t{2}, consumed, "resource limit is detected from the frame header");
    result.expectEqual(1, context.testSubProtocol.messageErrors, "subprotocol receives one size-limit error");
    result.expectEqual(uint16_t{1009}, context.testSubProtocol.lastError, "subprotocol receives Message Too Big status");
    result.expectEqual(0, context.testSubProtocol.messageStarts, "oversized frame never reaches application message start");
    result.expectTrue(connection.output == expectedCloseFrame, "resource violation emits an unmasked close frame carrying wire code 1009");
    result.expectEqual(1, connection.shutdownReadCount, "receiver input is shut down after the terminal resource violation");

    return result.processResult();
}
