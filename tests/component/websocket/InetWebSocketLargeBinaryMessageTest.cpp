#include "core/SNodeC.h"
#include "net/in/SocketAddress.h"
#include "support/TestResult.h"
#include "utils/Timeval.h"
#include "web/http/client/Request.h"
#include "web/http/client/Response.h"
#include "web/http/legacy/in/Client.h"
#include "web/http/legacy/in/Server.h"
#include "web/http/server/Request.h"
#include "web/http/server/Response.h"
#include "web/websocket/SubProtocolFactory.h"
#include "web/websocket/client/SocketContextUpgradeFactory.h"
#include "web/websocket/client/SubProtocol.h"
#include "web/websocket/client/SubProtocolFactorySelector.h"
#include "web/websocket/server/SocketContextUpgradeFactory.h"
#include "web/websocket/server/SubProtocol.h"
#include "web/websocket/server/SubProtocolFactorySelector.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace {

    constexpr const char* protocolName = "component-large-binary";
    constexpr const char* path = "/component/websocket/large-binary";
    constexpr std::size_t payloadSize = 16 * 1024;

    std::vector<unsigned char> largeBinaryMessage() {
        std::vector<unsigned char> message(payloadSize);
        for (std::size_t i = 0; i < message.size(); ++i) {
            message[i] = static_cast<unsigned char>(i % 251);
        }
        return message;
    }
    const std::string largeBinaryReply = "large-binary-ok";

    struct State {
        int listenOkCount = 0;
        int effectiveListenEndpointOkCount = 0;
        int clientConnectOkCount = 0;
        int serverRequestCount = 0;
        int serverUpgradeOkCount = 0;
        int clientHttpConnectedCount = 0;
        int clientUpgradeInitiatedCount = 0;
        int clientUpgradeResponseCount = 0;
        int serverWsConnectedCount = 0;
        int clientWsConnectedCount = 0;
        int serverMessageStartCount = 0;
        int serverMessageEndCount = 0;
        int clientMessageStartCount = 0;
        int clientMessageEndCount = 0;
        int serverWsDisconnectedCount = 0;
        int clientWsDisconnectedCount = 0;
        int httpDisconnectedCount = 0;
        int parseErrorCount = 0;
        int messageErrorCount = 0;
        int unexpectedStateCount = 0;
        int serverTextOpcodeCount = 0;
        int clientTextOpcodeCount = 0;
        int serverBinaryOpcodeCount = 0;
        int clientBinaryOpcodeCount = 0;
        std::string currentServerMessage;
        std::string currentClientMessage;
        std::vector<unsigned char> serverBinary;
        std::vector<unsigned char> clientBinary;
    };

    class ServerSubProtocol : public web::websocket::server::SubProtocol {
    public:
        ServerSubProtocol(web::websocket::SubProtocolContext* context, State& state)
            : web::websocket::server::SubProtocol(context, protocolName, 0, 3)
            , state(state) {
        }

    private:
        void onConnected() override {
            ++state.serverWsConnectedCount;
        }
        void onMessageStart(int opCode) override {
            ++state.serverMessageStartCount;
            state.currentServerMessage.clear();
            state.serverBinary.clear();
            if (opCode == 1)
                ++state.serverTextOpcodeCount;
            if (opCode == 2)
                ++state.serverBinaryOpcodeCount;
        }
        void onMessageData(const char* chunk, std::size_t chunkLen) override {
            state.currentServerMessage.append(chunk, chunkLen);
            const auto* begin = reinterpret_cast<const unsigned char*>(chunk);
            state.serverBinary.insert(state.serverBinary.end(), begin, begin + chunkLen);
        }
        void onMessageEnd() override {
            ++state.serverMessageEndCount;
            if (state.serverBinary == largeBinaryMessage())
                sendMessage(largeBinaryReply);
            else {
                ++state.unexpectedStateCount;
                core::SNodeC::stop();
            }
        }
        void onMessageError(uint16_t) override {
            ++state.messageErrorCount;
            core::SNodeC::stop();
        }
        void onDisconnected() override {
            ++state.serverWsDisconnectedCount;
        }
        bool onSignal(int) override {
            sendClose();
            return false;
        }
        State& state;
    };

    class ClientSubProtocol : public web::websocket::client::SubProtocol {
    public:
        ClientSubProtocol(web::websocket::SubProtocolContext* context, State& state)
            : web::websocket::client::SubProtocol(context, protocolName, 0, 3)
            , state(state) {
        }

    private:
        void onConnected() override {
            ++state.clientWsConnectedCount;
            const auto message = largeBinaryMessage();
            sendMessage(reinterpret_cast<const char*>(message.data()), message.size());
        }
        void onMessageStart(int opCode) override {
            ++state.clientMessageStartCount;
            state.currentClientMessage.clear();
            state.clientBinary.clear();
            if (opCode == 1)
                ++state.clientTextOpcodeCount;
            if (opCode == 2)
                ++state.clientBinaryOpcodeCount;
        }
        void onMessageData(const char* chunk, std::size_t chunkLen) override {
            state.currentClientMessage.append(chunk, chunkLen);
            const auto* begin = reinterpret_cast<const unsigned char*>(chunk);
            state.clientBinary.insert(state.clientBinary.end(), begin, begin + chunkLen);
        }
        void onMessageEnd() override {
            ++state.clientMessageEndCount;
            if (state.currentClientMessage == largeBinaryReply)
                sendClose();
            else {
                ++state.unexpectedStateCount;
                core::SNodeC::stop();
            }
        }
        void onMessageError(uint16_t) override {
            ++state.messageErrorCount;
            core::SNodeC::stop();
        }
        void onDisconnected() override {
            ++state.clientWsDisconnectedCount;
            core::SNodeC::stop();
        }
        bool onSignal(int) override {
            sendClose();
            return false;
        }
        State& state;
    };

    class ServerFactory : public web::websocket::SubProtocolFactory<web::websocket::server::SubProtocol> {
    public:
        explicit ServerFactory(State& state)
            : web::websocket::SubProtocolFactory<web::websocket::server::SubProtocol>(protocolName)
            , state(state) {
        }

    private:
        web::websocket::server::SubProtocol* create(web::websocket::SubProtocolContext* context) override {
            return new ServerSubProtocol(context, state);
        }
        State& state;
    };
    class ClientFactory : public web::websocket::SubProtocolFactory<web::websocket::client::SubProtocol> {
    public:
        explicit ClientFactory(State& state)
            : web::websocket::SubProtocolFactory<web::websocket::client::SubProtocol>(protocolName)
            , state(state) {
        }

    private:
        web::websocket::client::SubProtocol* create(web::websocket::SubProtocolContext* context) override {
            return new ClientSubProtocol(context, state);
        }
        State& state;
    };

    State* linkedState = nullptr;
    web::websocket::SubProtocolFactory<web::websocket::server::SubProtocol>* createServerFactory() {
        return new ServerFactory(*linkedState);
    }
    web::websocket::SubProtocolFactory<web::websocket::client::SubProtocol>* createClientFactory() {
        return new ClientFactory(*linkedState);
    }

    int runTest(int argc, char* argv[]) {
        tests::support::TestResult testResult;
        State state;
        linkedState = &state;
        core::SNodeC::init(argc, argv);
        using Server = web::http::legacy::in::Server;
        using Client = web::http::legacy::in::Client;
        const Server server("InetWebSocketLargeBinaryMessageTest-server", [&state](const auto& request, const auto& response) {
            ++state.serverRequestCount;
            if (request->url == path && request->get("upgrade") == "websocket") {
                response->upgrade(request, [&state, response](const std::string& selectedProtocol) {
                    if (selectedProtocol == "websocket") {
                        ++state.serverUpgradeOkCount;
                        response->end();
                    } else {
                        ++state.unexpectedStateCount;
                        response->sendStatus(400);
                        core::SNodeC::stop();
                    }
                });
            } else {
                ++state.unexpectedStateCount;
                response->sendStatus(404);
                core::SNodeC::stop();
            }
        });
        Client client(
            "InetWebSocketLargeBinaryMessageTest-client",
            [&state](const auto& request) {
                ++state.clientHttpConnectedCount;
                request->set("Sec-WebSocket-Protocol", protocolName);
                request->upgrade(
                    path,
                    "websocket",
                    [&state](bool success) {
                        if (success)
                            ++state.clientUpgradeInitiatedCount;
                        else {
                            ++state.unexpectedStateCount;
                            core::SNodeC::stop();
                        }
                    },
                    [&state](const auto&, const auto& response, bool success) {
                        if (success && response->get("upgrade") == "websocket")
                            ++state.clientUpgradeResponseCount;
                        else {
                            ++state.unexpectedStateCount;
                            core::SNodeC::stop();
                        }
                    },
                    [&state](const auto&, const std::string&) {
                        ++state.parseErrorCount;
                        core::SNodeC::stop();
                    });
            },
            [&state](const auto&) {
                ++state.httpDisconnectedCount;
            });
        web::websocket::server::SocketContextUpgradeFactory::link();
        web::websocket::client::SocketContextUpgradeFactory::link();
        web::websocket::server::SubProtocolFactorySelector::link(protocolName, createServerFactory);
        web::websocket::client::SubProtocolFactorySelector::link(protocolName, createClientFactory);
        server.getConfig()->Instance::forceUnrequired();
        client.getConfig()->Instance::forceUnrequired();
        server.listen(net::in::SocketAddress("127.0.0.1", 0),
                      [&client, &state](const net::in::SocketAddress& socketAddress, core::socket::State listenState) {
                          if (listenState == core::socket::State::OK) {
                              ++state.listenOkCount;
                              const std::uint16_t effectivePort = socketAddress.getPort();
                              if (effectivePort != 0) {
                                  ++state.effectiveListenEndpointOkCount;
                                  client.connect(net::in::SocketAddress("127.0.0.1", effectivePort),
                                                 [&state](const net::in::SocketAddress&, core::socket::State connectState) {
                                                     if (connectState == core::socket::State::OK)
                                                         ++state.clientConnectOkCount;
                                                     else {
                                                         ++state.unexpectedStateCount;
                                                         core::SNodeC::stop();
                                                     }
                                                 });
                              } else {
                                  ++state.unexpectedStateCount;
                                  core::SNodeC::stop();
                              }
                          } else {
                              ++state.unexpectedStateCount;
                              core::SNodeC::stop();
                          }
                      });
        const int startResult = core::SNodeC::start(utils::Timeval({1, 0}));
        testResult.expectEqual(0, startResult, "IPv4 legacy WebSocket large binary event loop stops successfully");
        testResult.expectEqual(1, state.listenOkCount, "server listen callback reports OK once");
        testResult.expectEqual(1, state.effectiveListenEndpointOkCount, "reports one effective endpoint");
        testResult.expectEqual(1, state.clientConnectOkCount, "client connect callback reports OK once");
        testResult.expectEqual(1, state.serverRequestCount, "server observes one upgrade request");
        testResult.expectEqual(1, state.serverUpgradeOkCount, "server selects websocket upgrade once");
        testResult.expectEqual(1, state.clientHttpConnectedCount, "client reaches HTTP-connected callback once");
        testResult.expectEqual(1, state.clientUpgradeInitiatedCount, "client initiates upgrade once");
        testResult.expectEqual(1, state.clientUpgradeResponseCount, "client observes upgrade response once");
        testResult.expectEqual(1, state.serverWsConnectedCount, "server subprotocol connects once");
        testResult.expectEqual(1, state.clientWsConnectedCount, "client subprotocol connects once");
        testResult.expectEqual(1, state.clientWsDisconnectedCount, "client observes disconnect once");
        testResult.expectEqual(0, state.parseErrorCount, "reports no HTTP parse errors");
        testResult.expectEqual(0, state.messageErrorCount, "reports no message errors");
        testResult.expectEqual(0, state.unexpectedStateCount, "reports no unexpected states");
        testResult.expectEqual(1, state.serverMessageStartCount, "server observes one large binary message start");
        testResult.expectEqual(1, state.serverMessageEndCount, "server observes one large binary message end");
        testResult.expectEqual(1, state.serverBinaryOpcodeCount, "server observes binary opcode");
        testResult.expectEqual(largeBinaryMessage().size(), state.serverBinary.size(), "server receives expected large binary size");
        testResult.expectTrue(state.serverBinary == largeBinaryMessage(), "server receives exact large binary content");
        testResult.expectEqual(1, state.clientMessageStartCount, "client observes one acknowledgement message start");
        testResult.expectEqual(1, state.clientMessageEndCount, "client observes one acknowledgement message end");
        const int result = testResult.processResult();
        core::SNodeC::free();
        return result;
    }

} // namespace

int main(int argc, char* argv[]) {
    int result = tests::support::cTestSkipReturnCode;
    if (tests::support::shouldSkipRootWithoutSNodeCGroup()) {
        tests::support::printRootWithoutSNodeCGroupSkipMessage("InetWebSocketLargeBinaryMessageTest");
    } else {
        result = runTest(argc, argv);
    }
    return result;
}
