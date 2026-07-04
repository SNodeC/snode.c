/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
 */

#ifndef TESTS_COMPONENT_WEBSOCKET_WEBSOCKETSERVERCLIENTTEXTECHOTEST_H
#define TESTS_COMPONENT_WEBSOCKET_WEBSOCKETSERVERCLIENTTEXTECHOTEST_H

#include "core/SNodeC.h"
#include "support/TestResult.h"
#include "utils/Timeval.h"
#include "web/http/client/Request.h"
#include "web/http/client/Response.h"
#include "web/http/server/Request.h"
#include "web/http/server/Response.h"
#include "web/websocket/SubProtocolContext.h"
#include "web/websocket/SubProtocolFactory.h"
#include "web/websocket/client/SocketContextUpgradeFactory.h"
#include "web/websocket/client/SubProtocol.h"
#include "web/websocket/client/SubProtocolFactorySelector.h"
#include "web/websocket/server/SocketContextUpgradeFactory.h"
#include "web/websocket/server/SubProtocol.h"
#include "web/websocket/server/SubProtocolFactorySelector.h"

#include <cstddef>
#include <memory>
#include <string>

namespace tests::component::websocket {

    constexpr const char* subProtocolName = "component-echo";
    constexpr const char* requestMessage = "snode.c websocket component request";
    constexpr const char* replyMessage = "snode.c websocket component reply";

    struct EchoState {
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

        bool serverSawTextOpcode = false;
        bool clientSawTextOpcode = false;
        std::string serverMessage;
        std::string clientMessage;
    };

    class ServerEchoSubProtocol : public web::websocket::server::SubProtocol {
    public:
        ServerEchoSubProtocol(web::websocket::SubProtocolContext* context, EchoState& state)
            : web::websocket::server::SubProtocol(context, tests::component::websocket::subProtocolName, 0, 3)
            , state(state) {
        }

    private:
        void onConnected() override {
            ++state.serverWsConnectedCount;
        }

        void onMessageStart(int opCode) override {
            ++state.serverMessageStartCount;
            state.serverSawTextOpcode = opCode == 1;
            state.serverMessage.clear();
        }

        void onMessageData(const char* chunk, std::size_t chunkLen) override {
            state.serverMessage.append(chunk, chunkLen);
        }

        void onMessageEnd() override {
            ++state.serverMessageEndCount;
            if (state.serverMessage == requestMessage) {
                sendMessage(replyMessage);
                sendClose();
            } else {
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

        EchoState& state;
    };

    class ClientEchoSubProtocol : public web::websocket::client::SubProtocol {
    public:
        ClientEchoSubProtocol(web::websocket::SubProtocolContext* context, EchoState& state)
            : web::websocket::client::SubProtocol(context, tests::component::websocket::subProtocolName, 0, 3)
            , state(state) {
        }

    private:
        void onConnected() override {
            ++state.clientWsConnectedCount;
            sendMessage(requestMessage);
        }

        void onMessageStart(int opCode) override {
            ++state.clientMessageStartCount;
            state.clientSawTextOpcode = opCode == 1;
            state.clientMessage.clear();
        }

        void onMessageData(const char* chunk, std::size_t chunkLen) override {
            state.clientMessage.append(chunk, chunkLen);
        }

        void onMessageEnd() override {
            ++state.clientMessageEndCount;
            if (state.clientMessage != replyMessage) {
                ++state.unexpectedStateCount;
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

        EchoState& state;
    };

    class ServerEchoSubProtocolFactory : public web::websocket::SubProtocolFactory<web::websocket::server::SubProtocol> {
    public:
        explicit ServerEchoSubProtocolFactory(EchoState& state)
            : web::websocket::SubProtocolFactory<web::websocket::server::SubProtocol>(tests::component::websocket::subProtocolName)
            , state(state) {
        }

    private:
        web::websocket::server::SubProtocol* create(web::websocket::SubProtocolContext* context) override {
            return new ServerEchoSubProtocol(context, state);
        }

        EchoState& state;
    };

    class ClientEchoSubProtocolFactory : public web::websocket::SubProtocolFactory<web::websocket::client::SubProtocol> {
    public:
        explicit ClientEchoSubProtocolFactory(EchoState& state)
            : web::websocket::SubProtocolFactory<web::websocket::client::SubProtocol>(tests::component::websocket::subProtocolName)
            , state(state) {
        }

    private:
        web::websocket::client::SubProtocol* create(web::websocket::SubProtocolContext* context) override {
            return new ClientEchoSubProtocol(context, state);
        }

        EchoState& state;
    };

    inline EchoState* linkedState = nullptr;

    inline web::websocket::SubProtocolFactory<web::websocket::server::SubProtocol>* createServerEchoFactory() {
        return new ServerEchoSubProtocolFactory(*linkedState);
    }

    inline web::websocket::SubProtocolFactory<web::websocket::client::SubProtocol>* createClientEchoFactory() {
        return new ClientEchoSubProtocolFactory(*linkedState);
    }

    inline void linkWebSocketFactories(EchoState& state) {
        linkedState = &state;
        web::websocket::server::SocketContextUpgradeFactory::link();
        web::websocket::client::SocketContextUpgradeFactory::link();
        web::websocket::server::SubProtocolFactorySelector::link(subProtocolName, createServerEchoFactory);
        web::websocket::client::SubProtocolFactorySelector::link(subProtocolName, createClientEchoFactory);
    }

    template <typename Server, typename Client>
    void configureWebSocketEcho(Server& server, Client& client, EchoState& state) {
        linkWebSocketFactories(state);
        server.getConfig()->Instance::forceUnrequired();
        client.getConfig()->Instance::forceUnrequired();
    }

    template <typename Request, typename Response>
    void handleUpgradeRequest(const std::shared_ptr<Request>& request, const std::shared_ptr<Response>& response, EchoState& state) {
        ++state.serverRequestCount;
        if (request->url == "/component/websocket" && request->get("upgrade") == "websocket") {
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
    }

    template <typename MasterRequest>
    void startUpgrade(const std::shared_ptr<MasterRequest>& request, EchoState& state) {
        ++state.clientHttpConnectedCount;
        request->set("Sec-WebSocket-Protocol", subProtocolName);
        request->upgrade(
            "/component/websocket",
            "websocket",
            [&state](bool success) {
                if (success) {
                    ++state.clientUpgradeInitiatedCount;
                } else {
                    ++state.unexpectedStateCount;
                    core::SNodeC::stop();
                }
            },
            [&state](const auto&, const auto& response, bool success) {
                if (success && response->get("upgrade") == "websocket") {
                    ++state.clientUpgradeResponseCount;
                } else {
                    ++state.unexpectedStateCount;
                    core::SNodeC::stop();
                }
            },
            [&state](const auto&, const std::string&) {
                ++state.parseErrorCount;
                core::SNodeC::stop();
            });
    }

    inline void expectTextEcho(tests::support::TestResult& testResult,
                               const EchoState& state,
                               const std::string& transportName,
                               int startResult) {
        testResult.expectEqual(0, startResult, "event loop stops successfully after " + transportName + " WebSocket text echo");
        testResult.expectEqual(1, state.listenOkCount, transportName + " WebSocket server listen callback reports OK exactly once");
        testResult.expectEqual(1, state.effectiveListenEndpointOkCount, transportName + " WebSocket server reports one effective endpoint");
        testResult.expectEqual(1, state.clientConnectOkCount, transportName + " WebSocket client connect callback reports OK exactly once");
        testResult.expectEqual(1, state.serverRequestCount, transportName + " WebSocket server observes one upgrade request");
        testResult.expectEqual(1, state.serverUpgradeOkCount, transportName + " WebSocket server selects websocket upgrade once");
        testResult.expectEqual(1, state.clientHttpConnectedCount, transportName + " WebSocket client reaches HTTP-connected callback once");
        testResult.expectEqual(1, state.clientUpgradeInitiatedCount, transportName + " WebSocket client initiates upgrade once");
        testResult.expectEqual(1, state.clientUpgradeResponseCount, transportName + " WebSocket client observes upgrade response once");
        testResult.expectEqual(1, state.serverWsConnectedCount, transportName + " WebSocket server subprotocol connects once");
        testResult.expectEqual(1, state.clientWsConnectedCount, transportName + " WebSocket client subprotocol connects once");
        testResult.expectEqual(1, state.serverMessageStartCount, transportName + " WebSocket server observes one text message start");
        testResult.expectEqual(1, state.serverMessageEndCount, transportName + " WebSocket server observes one text message end");
        testResult.expectEqual(1, state.clientMessageStartCount, transportName + " WebSocket client observes one text message start");
        testResult.expectEqual(1, state.clientMessageEndCount, transportName + " WebSocket client observes one text message end");
        testResult.expectEqual(1, state.clientWsDisconnectedCount, transportName + " WebSocket client observes close lifecycle once");
        testResult.expectEqual(0, state.parseErrorCount, transportName + " WebSocket client reports no HTTP parse errors");
        testResult.expectEqual(0, state.messageErrorCount, transportName + " WebSocket reports no message errors");
        testResult.expectEqual(0, state.unexpectedStateCount, transportName + " WebSocket echo reports no unexpected states");
        testResult.expectTrue(state.serverSawTextOpcode, transportName + " WebSocket server observes text opcode");
        testResult.expectTrue(state.clientSawTextOpcode, transportName + " WebSocket client observes text opcode");
        testResult.expectTrue(state.serverMessage == requestMessage, transportName + " WebSocket server receives deterministic text");
        testResult.expectTrue(state.clientMessage == replyMessage, transportName + " WebSocket client receives deterministic reply");
    }

} // namespace tests::component::websocket

#endif // TESTS_COMPONENT_WEBSOCKET_WEBSOCKETSERVERCLIENTTEXTECHOTEST_H
