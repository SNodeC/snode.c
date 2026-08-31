/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
 */

#include "core/SNodeC.h"
#include "core/socket/State.h"
#include "core/socket/stream/SocketConnection.h"
#include "core/socket/stream/SocketContext.h"
#include "core/socket/stream/SocketContextFactory.h"
#include "express/legacy/in/WebApp.h"
#include "net/in/SocketAddress.h"
#include "net/in/stream/legacy/SocketClient.h"
#include "support/TestResult.h"
#include "utils/Timeval.h"
#include "web/http/ConfigHttpParser.h"
#include "web/http/server/ConfigHttpServer.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

namespace {

    constexpr std::string_view requests = "POST /bounded HTTP/1.1\r\n"
                                          "Host: 127.0.0.1\r\n"
                                          "Content-Length: 4\r\n"
                                          "\r\n"
                                          "okay"
                                          "POST /oversized HTTP/1.1\r\n"
                                          "Host: 127.0.0.1\r\n"
                                          "Content-Length: 5\r\n"
                                          "Connection: close\r\n"
                                          "\r\n"
                                          "large";

    struct State {
        int listenOkCount = 0;
        int effectiveListenEndpointOkCount = 0;
        int clientConnectOkCount = 0;
        int rawClientConnectedCount = 0;
        int rawClientDisconnectedCount = 0;
        int middlewareCount = 0;
        int boundedHandlerCount = 0;
        int oversizedHandlerCount = 0;
        int unexpectedStateCount = 0;
        std::string responseBuffer;
    };

    class RawClientContext : public core::socket::stream::SocketContext {
    public:
        RawClientContext(core::socket::stream::SocketConnection* socketConnection, State& state)
            : core::socket::stream::SocketContext(socketConnection)
            , state(state) {
        }

    private:
        void onConnected() override {
            ++state.rawClientConnectedCount;
            sendToPeer(requests.data(), requests.size());
        }

        void onDisconnected() override {
            ++state.rawClientDisconnectedCount;
            core::SNodeC::stop();
        }

        std::size_t onReceivedFromPeer() override {
            char chunk[4096];
            const std::size_t chunkLen = readFromPeer(chunk, sizeof(chunk));
            state.responseBuffer.append(chunk, chunkLen);
            if (state.responseBuffer.find(" 413 ") != std::string::npos) {
                core::SNodeC::stop();
            }
            return chunkLen;
        }

        bool onSignal([[maybe_unused]] int signum) override {
            return true;
        }

        State& state;
    };

    class RawClientFactory : public core::socket::stream::SocketContextFactory {
    public:
        explicit RawClientFactory(State& state)
            : state(state) {
        }

    private:
        core::socket::stream::SocketContext* create(core::socket::stream::SocketConnection* socketConnection) override {
            return new RawClientContext(socketConnection, state);
        }

        State& state;
    };

} // namespace

int main(int argc, char* argv[]) {
    tests::support::TestResult testResult;
    int result = tests::support::cTestSkipReturnCode;

    if (tests::support::shouldSkipRootWithoutSNodeCGroup()) {
        tests::support::printRootWithoutSNodeCGroupSkipMessage("InetExpressHttpParserLimitTest");
    } else {
        State state;
        core::SNodeC::init(argc, argv);

        const express::legacy::in::WebApp app("ipv4-express-http-parser-limit-server");
        app.use([&state] MIDDLEWARE(req, res, next) {
            ++state.middlewareCount;
            next();
        });
        app.post("/bounded", [&state] APPLICATION(req, res) {
            ++state.boundedHandlerCount;
            res->status(200).send("bounded");
        });
        app.post("/oversized", [&state] APPLICATION(req, res) {
            ++state.oversizedHandlerCount;
            res->status(200).send("unexpected oversized handler");
        });

        auto* httpConfig = app.getConfig()->Instance::getSubCommand<web::http::server::ConfigHttpServer>();
        httpConfig->getParserConfig()->setMaximumBodyBytes(4);
        app.getConfig()->Instance::forceUnrequired();

        net::in::stream::legacy::SocketClient<RawClientFactory, State&> client("ipv4-express-http-parser-limit-client", state);
        client.getConfig()->Instance::forceUnrequired();

        app.listen(net::in::SocketAddress("127.0.0.1", 0), [&client, &state](const auto& socketAddress, core::socket::State listenState) {
            if (listenState == core::socket::State::OK) {
                ++state.listenOkCount;
                if (socketAddress.getPort() != 0) {
                    ++state.effectiveListenEndpointOkCount;
                    client.connect(net::in::SocketAddress("127.0.0.1", socketAddress.getPort()),
                                   [&state](const auto&, core::socket::State connectState) {
                                       if (connectState == core::socket::State::OK) {
                                           ++state.clientConnectOkCount;
                                       } else {
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
        testResult.expectEqual(0, startResult, "event loop stops after the parser limit response");
        testResult.expectEqual(1, state.listenOkCount, "Express server listen succeeds once");
        testResult.expectEqual(1, state.effectiveListenEndpointOkCount, "Express server reports an effective endpoint");
        testResult.expectEqual(1, state.clientConnectOkCount, "raw client connects once");
        testResult.expectEqual(1, state.rawClientConnectedCount, "raw client context connects once");
        testResult.expectEqual(1, state.middlewareCount, "middleware receives the valid bounded request only");
        testResult.expectEqual(1, state.boundedHandlerCount, "valid request at the body boundary reaches its route");
        testResult.expectEqual(0, state.oversizedHandlerCount, "oversized request is rejected before Express routing");
        testResult.expectEqual(0, state.unexpectedStateCount, "parser limit test reports no unexpected states");
        testResult.expectTrue(state.responseBuffer.find("unexpected oversized handler") == std::string::npos,
                              "oversized request never produces the application response");

        result = testResult.processResult();
        core::SNodeC::free();
    }

    return result;
}
