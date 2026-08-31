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
#include "net/in/SocketAddress.h"
#include "net/in/stream/legacy/SocketServer.h"
#include "support/TestResult.h"
#include "utils/Timeval.h"
#include "web/http/ConfigHttpParser.h"
#include "web/http/client/ConfigHTTP.h"
#include "web/http/legacy/in/Client.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

namespace {

    constexpr std::string_view oversizedResponse = "HTTP/1.1 200 OK\r\n"
                                                   "Content-Length: 5\r\n"
                                                   "Connection: close\r\n"
                                                   "\r\n"
                                                   "hello";

    struct State {
        int listenOkCount = 0;
        int effectiveListenEndpointOkCount = 0;
        int clientConnectOkCount = 0;
        int rawServerRequestCount = 0;
        int httpConnectedCount = 0;
        int responseCount = 0;
        int parseErrorCount = 0;
        int unexpectedStateCount = 0;
        std::string parseError;
    };

    class RawServerContext : public core::socket::stream::SocketContext {
    public:
        RawServerContext(core::socket::stream::SocketConnection* socketConnection, State& state)
            : core::socket::stream::SocketContext(socketConnection)
            , state(state) {
        }

    private:
        void onConnected() override {
        }

        void onDisconnected() override {
        }

        std::size_t onReceivedFromPeer() override {
            char chunk[4096];
            const std::size_t chunkLen = readFromPeer(chunk, sizeof(chunk));
            request.append(chunk, chunkLen);
            if (!responseSent && request.find("\r\n\r\n") != std::string::npos) {
                responseSent = true;
                ++state.rawServerRequestCount;
                sendToPeer(oversizedResponse.data(), oversizedResponse.size());
                shutdownWrite();
            }
            return chunkLen;
        }

        bool onSignal([[maybe_unused]] int signum) override {
            return true;
        }

        State& state;
        std::string request;
        bool responseSent = false;
    };

    class RawServerFactory : public core::socket::stream::SocketContextFactory {
    public:
        explicit RawServerFactory(State& state)
            : state(state) {
        }

    private:
        core::socket::stream::SocketContext* create(core::socket::stream::SocketConnection* socketConnection) override {
            return new RawServerContext(socketConnection, state);
        }

        State& state;
    };

} // namespace

int main(int argc, char* argv[]) {
    tests::support::TestResult testResult;
    int result = tests::support::cTestSkipReturnCode;

    if (tests::support::shouldSkipRootWithoutSNodeCGroup()) {
        tests::support::printRootWithoutSNodeCGroupSkipMessage("InetHttpClientParserLimitTest");
    } else {
        State state;
        core::SNodeC::init(argc, argv);

        net::in::stream::legacy::SocketServer<RawServerFactory, State&> server("ipv4-http-parser-limit-raw-server", state);
        web::http::legacy::in::Client client(
            "ipv4-http-parser-limit-client",
            [&state](const auto& request) {
                ++state.httpConnectedCount;
                request->method = "GET";
                request->url = "/oversized-response";
                request->set("Connection", "close");
                request->end(
                    [&state](const auto&, const auto&) {
                        ++state.responseCount;
                        core::SNodeC::stop();
                    },
                    [&state](const auto&, const std::string& message) {
                        ++state.parseErrorCount;
                        state.parseError = message;
                        core::SNodeC::stop();
                    });
            },
            [](const auto&) {
            });

        auto* clientHttpConfig = client.getConfig()->Instance::getSubCommand<web::http::client::ConfigHTTP>();
        clientHttpConfig->getParserConfig()->setMaximumBodyBytes(4);
        client.getConfig()->Instance::forceUnrequired();
        server.getConfig()->Instance::forceUnrequired();

        server.listen(net::in::SocketAddress("127.0.0.1", 0),
                      [&client, &state](const auto& socketAddress, core::socket::State listenState) {
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
        testResult.expectEqual(0, startResult, "event loop stops after client parser rejects oversized response");
        testResult.expectEqual(1, state.listenOkCount, "raw server listen succeeds once");
        testResult.expectEqual(1, state.effectiveListenEndpointOkCount, "raw server reports an effective endpoint");
        testResult.expectEqual(1, state.clientConnectOkCount, "HTTP client connects once");
        testResult.expectEqual(1, state.rawServerRequestCount, "raw server receives one request");
        testResult.expectEqual(1, state.httpConnectedCount, "HTTP client callback runs once");
        testResult.expectEqual(0, state.responseCount, "oversized response is not delivered as valid");
        testResult.expectEqual(1, state.parseErrorCount, "configured client parser reports one size error");
        testResult.expectEqual(0, state.unexpectedStateCount, "client parser limit test reports no unexpected states");
        testResult.expectTrue(!state.parseError.empty(), "client parser size failure has an error message");

        result = testResult.processResult();
        core::SNodeC::free();
    }

    return result;
}
