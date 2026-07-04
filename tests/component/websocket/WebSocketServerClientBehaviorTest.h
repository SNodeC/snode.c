/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
 */

#ifndef TESTS_COMPONENT_WEBSOCKET_WEBSOCKETSERVERCLIENTBEHAVIORTEST_H
#define TESTS_COMPONENT_WEBSOCKET_WEBSOCKETSERVERCLIENTBEHAVIORTEST_H

#include "core/SNodeC.h"
#include "support/TestResult.h"
#include "web/http/client/Request.h"
#include "web/http/client/Response.h"
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

namespace tests::component::websocket::behavior {

    constexpr const char* subProtocolName = "component-behavior";
    constexpr const char* path = "/component/websocket/behavior";

    enum class Scenario { MultiText, BinaryEcho, CloseHandshake, PingPong };

    struct State {
        Scenario scenario;
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
        int clientPongReceivedCount = 0;
        int serverTextOpcodeCount = 0;
        int clientTextOpcodeCount = 0;
        int serverBinaryOpcodeCount = 0;
        int clientBinaryOpcodeCount = 0;
        std::string currentServerMessage;
        std::string currentClientMessage;
        std::vector<std::string> serverMessages;
        std::vector<std::string> clientMessages;
        std::vector<unsigned char> serverBinary;
        std::vector<unsigned char> clientBinary;
    };

    inline const std::vector<std::string> textMessages = {"message-1", "message-2", "message-3"};
    inline const std::vector<std::string> textReplies = {"reply-1", "reply-2", "reply-3"};
    inline const std::vector<unsigned char> binaryRequest = {0x00, 0x01, 0x02, 0x7f, 0x80, 0xff};
    inline const std::vector<unsigned char> binaryReply = {0xff, 0x80, 0x7f, 0x02, 0x01, 0x00};

    class ServerSubProtocol : public web::websocket::server::SubProtocol {
    public:
        ServerSubProtocol(web::websocket::SubProtocolContext* context, State& state)
            : web::websocket::server::SubProtocol(context, subProtocolName, 0, 3)
            , state(state) {
        }

    private:
        void onConnected() override { ++state.serverWsConnectedCount; }

        void onMessageStart(int opCode) override {
            ++state.serverMessageStartCount;
            state.currentServerMessage.clear();
            state.serverBinary.clear();
            if (opCode == 1) ++state.serverTextOpcodeCount;
            if (opCode == 2) ++state.serverBinaryOpcodeCount;
        }

        void onMessageData(const char* chunk, std::size_t chunkLen) override {
            state.currentServerMessage.append(chunk, chunkLen);
            const auto* begin = reinterpret_cast<const unsigned char*>(chunk);
            state.serverBinary.insert(state.serverBinary.end(), begin, begin + chunkLen);
        }

        void onMessageEnd() override {
            ++state.serverMessageEndCount;
            if (state.scenario == Scenario::MultiText) {
                state.serverMessages.push_back(state.currentServerMessage);
                const std::size_t index = state.serverMessages.size() - 1;
                if (index < textMessages.size() && state.currentServerMessage == textMessages[index]) {
                    sendMessage(textReplies[index]);
                } else {
                    ++state.unexpectedStateCount;
                    core::SNodeC::stop();
                }
            } else if (state.scenario == Scenario::BinaryEcho) {
                if (state.serverBinary == binaryRequest) {
                    sendMessage(reinterpret_cast<const char*>(binaryReply.data()), binaryReply.size());
                } else {
                    ++state.unexpectedStateCount;
                    core::SNodeC::stop();
                }
            } else {
                ++state.unexpectedStateCount;
                core::SNodeC::stop();
            }
        }

        void onMessageError(uint16_t) override { ++state.messageErrorCount; core::SNodeC::stop(); }
        void onDisconnected() override { ++state.serverWsDisconnectedCount; }
        bool onSignal(int) override { sendClose(); return false; }
        State& state;
    };

    class ClientSubProtocol : public web::websocket::client::SubProtocol {
    public:
        ClientSubProtocol(web::websocket::SubProtocolContext* context, State& state)
            : web::websocket::client::SubProtocol(context, subProtocolName, 0, 3)
            , state(state) {
        }

    private:
        void onConnected() override {
            ++state.clientWsConnectedCount;
            if (state.scenario == Scenario::MultiText) sendMessage(textMessages.front());
            if (state.scenario == Scenario::BinaryEcho) sendMessage(reinterpret_cast<const char*>(binaryRequest.data()), binaryRequest.size());
            if (state.scenario == Scenario::CloseHandshake) sendClose(1000, "done", 4);
            if (state.scenario == Scenario::PingPong) sendPing("ping", 4);
        }

        void onMessageStart(int opCode) override {
            ++state.clientMessageStartCount;
            state.currentClientMessage.clear();
            state.clientBinary.clear();
            if (opCode == 1) ++state.clientTextOpcodeCount;
            if (opCode == 2) ++state.clientBinaryOpcodeCount;
        }

        void onMessageData(const char* chunk, std::size_t chunkLen) override {
            state.currentClientMessage.append(chunk, chunkLen);
            const auto* begin = reinterpret_cast<const unsigned char*>(chunk);
            state.clientBinary.insert(state.clientBinary.end(), begin, begin + chunkLen);
        }

        void onMessageEnd() override {
            ++state.clientMessageEndCount;
            if (state.scenario == Scenario::MultiText) {
                state.clientMessages.push_back(state.currentClientMessage);
                const std::size_t index = state.clientMessages.size() - 1;
                if (index >= textReplies.size() || state.currentClientMessage != textReplies[index]) {
                    ++state.unexpectedStateCount;
                    core::SNodeC::stop();
                } else if (state.clientMessages.size() < textMessages.size()) {
                    sendMessage(textMessages[state.clientMessages.size()]);
                } else {
                    sendClose();
                }
            } else if (state.scenario == Scenario::BinaryEcho) {
                if (state.clientBinary == binaryReply) sendClose(); else ++state.unexpectedStateCount;
            } else {
                ++state.unexpectedStateCount;
                core::SNodeC::stop();
            }
        }

        void onMessageError(uint16_t) override { ++state.messageErrorCount; core::SNodeC::stop(); }
        void onPongReceived() override { ++state.clientPongReceivedCount; sendClose(); }
        void onDisconnected() override { ++state.clientWsDisconnectedCount; core::SNodeC::stop(); }
        bool onSignal(int) override { sendClose(); return false; }
        State& state;
    };

    class ServerFactory : public web::websocket::SubProtocolFactory<web::websocket::server::SubProtocol> {
    public:
        explicit ServerFactory(State& state) : web::websocket::SubProtocolFactory<web::websocket::server::SubProtocol>(tests::component::websocket::behavior::subProtocolName), state(state) {}
    private:
        web::websocket::server::SubProtocol* create(web::websocket::SubProtocolContext* context) override { return new ServerSubProtocol(context, state); }
        State& state;
    };

    class ClientFactory : public web::websocket::SubProtocolFactory<web::websocket::client::SubProtocol> {
    public:
        explicit ClientFactory(State& state) : web::websocket::SubProtocolFactory<web::websocket::client::SubProtocol>(tests::component::websocket::behavior::subProtocolName), state(state) {}
    private:
        web::websocket::client::SubProtocol* create(web::websocket::SubProtocolContext* context) override { return new ClientSubProtocol(context, state); }
        State& state;
    };

    inline State* linkedState = nullptr;
    inline web::websocket::SubProtocolFactory<web::websocket::server::SubProtocol>* createServerFactory() { return new ServerFactory(*linkedState); }
    inline web::websocket::SubProtocolFactory<web::websocket::client::SubProtocol>* createClientFactory() { return new ClientFactory(*linkedState); }

    template <typename Server, typename Client>
    void configure(Server& server, Client& client, State& state) {
        linkedState = &state;
        web::websocket::server::SocketContextUpgradeFactory::link();
        web::websocket::client::SocketContextUpgradeFactory::link();
        web::websocket::server::SubProtocolFactorySelector::link(subProtocolName, createServerFactory);
        web::websocket::client::SubProtocolFactorySelector::link(subProtocolName, createClientFactory);
        server.getConfig()->Instance::forceUnrequired();
        client.getConfig()->Instance::forceUnrequired();
    }

    template <typename Request, typename Response>
    void handleUpgradeRequest(const std::shared_ptr<Request>& request, const std::shared_ptr<Response>& response, State& state) {
        ++state.serverRequestCount;
        if (request->url == path && request->get("upgrade") == "websocket") {
            response->upgrade(request, [&state, response](const std::string& selectedProtocol) {
                if (selectedProtocol == "websocket") { ++state.serverUpgradeOkCount; response->end(); }
                else { ++state.unexpectedStateCount; response->sendStatus(400); core::SNodeC::stop(); }
            });
        } else { ++state.unexpectedStateCount; response->sendStatus(404); core::SNodeC::stop(); }
    }

    template <typename MasterRequest>
    void startUpgrade(const std::shared_ptr<MasterRequest>& request, State& state) {
        ++state.clientHttpConnectedCount;
        request->set("Sec-WebSocket-Protocol", subProtocolName);
        request->upgrade(path, "websocket", [&state](bool success) { if (success) ++state.clientUpgradeInitiatedCount; else { ++state.unexpectedStateCount; core::SNodeC::stop(); } },
                         [&state](const auto&, const auto& response, bool success) { if (success && response->get("upgrade") == "websocket") ++state.clientUpgradeResponseCount; else { ++state.unexpectedStateCount; core::SNodeC::stop(); } },
                         [&state](const auto&, const std::string&) { ++state.parseErrorCount; core::SNodeC::stop(); });
    }

    inline void expectCommon(tests::support::TestResult& testResult, const State& state, const std::string& name, int startResult) {
        testResult.expectEqual(0, startResult, name + " event loop stops successfully");
        testResult.expectEqual(1, state.listenOkCount, name + " server listen callback reports OK once");
        testResult.expectEqual(1, state.effectiveListenEndpointOkCount, name + " reports one effective endpoint");
        testResult.expectEqual(1, state.clientConnectOkCount, name + " client connect callback reports OK once");
        testResult.expectEqual(1, state.serverRequestCount, name + " server observes one upgrade request");
        testResult.expectEqual(1, state.serverUpgradeOkCount, name + " server selects websocket upgrade once");
        testResult.expectEqual(1, state.clientHttpConnectedCount, name + " client reaches HTTP-connected callback once");
        testResult.expectEqual(1, state.clientUpgradeInitiatedCount, name + " client initiates upgrade once");
        testResult.expectEqual(1, state.clientUpgradeResponseCount, name + " client observes upgrade response once");
        testResult.expectEqual(1, state.serverWsConnectedCount, name + " server subprotocol connects once");
        testResult.expectEqual(1, state.clientWsConnectedCount, name + " client subprotocol connects once");
        testResult.expectEqual(1, state.clientWsDisconnectedCount, name + " client observes disconnect once");
        testResult.expectEqual(0, state.parseErrorCount, name + " reports no HTTP parse errors");
        testResult.expectEqual(0, state.messageErrorCount, name + " reports no message errors");
        testResult.expectEqual(0, state.unexpectedStateCount, name + " reports no unexpected states");
    }

} // namespace tests::component::websocket::behavior

#endif // TESTS_COMPONENT_WEBSOCKET_WEBSOCKETSERVERCLIENTBEHAVIORTEST_H
