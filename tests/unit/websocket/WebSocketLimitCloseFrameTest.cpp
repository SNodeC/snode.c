#include "net/in/stream/legacy/SocketServer.h"
#include "tests/support/NetworkTest.h"
#include "web/http/ConfigWebSocket.h"
#include "web/http/SocketContextUpgradeFactory.hpp"
#include "web/websocket/SocketContextUpgrade.hpp"
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

    class Context : public web::websocket::SocketContextUpgrade<DummySubProtocol, DummyRequest, DummyResponse> {
        using Super = web::websocket::SocketContextUpgrade<DummySubProtocol, DummyRequest, DummyResponse>;

    public:
        Context(core::socket::stream::SocketConnection* connection, TestUpgradeFactory& factory, DummySubProtocol& protocol)
            : Super(connection, &factory, Role::SERVER) {
            subProtocol = &protocol;
        }
    };
    class Factory : public core::socket::stream::SocketContextFactory {
    public:
        Factory(TestUpgradeFactory& factory, DummySubProtocol& protocol)
            : factory(factory)
            , protocol(protocol) {
        }
        core::socket::stream::SocketContext* create(core::socket::stream::SocketConnection* connection) override {
            return new Context(connection, factory, protocol);
        }

    private:
        TestUpgradeFactory& factory;
        DummySubProtocol& protocol;
    };
} // namespace
int main() {
    using tests::support::require;
    tests::support::TestResult result;
    tests::support::Runtime runtime;
    tests::support::Peer peer;
    TestUpgradeFactory upgrade;
    DummySubProtocol protocol;
    net::in::stream::legacy::SocketServer<Factory, TestUpgradeFactory&, DummySubProtocol&> server("frame-limit", upgrade, protocol);
    server.getConfig()->Instance::forceUnrequired();
    server.getConfig()->net::config::ConfigInstance::template newSubCommand<web::http::ConfigWebSocket>()->setMaximumFrameBytes(2);
    server.listen(net::in::SocketAddress("127.0.0.1", 0), [&](const auto& addr, auto status) {
        require(status == core::socket::State::OK, "listen");
        peer.connect(addr.getPort());
        const char frame[] = {char(0x81), char(0x83), 1, 2, 3, 4, 'a', 'b', 'c'};
        require(peer.write(std::string(frame, sizeof(frame))) == sizeof(frame), "send oversized masked frame");
    });
    std::string wire;
    result.expectTrue(tests::support::runUntil(
                          [&] {
                              return wire.size() >= 4;
                          },
                          [&] {
                              if (peer.fd < 0)
                                  return;
                              char data[128];
                              int n = peer.read(data, sizeof(data));
                              if (n > 0)
                                  wire.append(data, static_cast<std::size_t>(n));
                          }),
                      "size violation emits close frame on real socket");
    const char expected[] = {char(0x88), 2, 3, char(0xf1)};
    result.expectTrue(wire == std::string(expected, sizeof(expected)), "wire close code is 1009");
    result.expectEqual(1, protocol.messageErrors, "one protocol error");
    result.expectEqual(1009, protocol.lastError, "application receives message-too-big status");
    result.expectEqual(0, protocol.messageStarts, "oversized message never reaches application");
    return result.processResult();
}
