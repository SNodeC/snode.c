/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "core/SNodeC.h"
#include "core/socket/State.h"
#include "core/socket/stream/SocketConnection.h"
#include "core/socket/stream/SocketContext.h"
#include "core/socket/stream/SocketContextFactory.h"
#include "net/in6/SocketAddress.h"
#include "net/in6/stream/legacy/SocketClient.h"
#include "net/in6/stream/legacy/SocketServer.h"
#include "support/TestResult.h"
#include "utils/Timeval.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace {

    constexpr std::string_view clientPayload = "snodec-ipv6-large-client";
    constexpr std::string_view serverReply = "snodec-ipv6-large-reply";
    constexpr std::size_t clientPayloadSize = 64 * 1024;
    constexpr std::size_t serverReplySize = 64 * 1024;

    std::string makeRepeatedPayload(std::string_view prefix, std::size_t targetSize) {
        std::string payload;
        payload.reserve(targetSize);
        while (payload.size() < targetSize) {
            payload += prefix;
            payload += ":";
            payload += std::to_string(payload.size());
            payload += ";";
        }
        payload.resize(targetSize);
        return payload;
    }

    struct TestState {
        int serverListenOkCount = 0;
        int effectiveListenPortOkCount = 0;
        int zeroEffectiveListenPortCount = 0;
        int clientConnectOkCount = 0;
        int serverFactoryCreateCount = 0;
        int clientFactoryCreateCount = 0;
        int serverConnectedCount = 0;
        int clientConnectedCount = 0;
        int serverLargePayloadReceivedCount = 0;
        int clientLargeReplyReceivedCount = 0;
        int unexpectedStateCount = 0;
        int unexpectedPayloadCount = 0;
    };
    class TestServerSocketContext : public core::socket::stream::SocketContext {
    public:
        TestServerSocketContext(core::socket::stream::SocketConnection* c, TestState& s)
            : SocketContext(c)
            , testState(s)
            , expected(makeRepeatedPayload(clientPayload, clientPayloadSize))
            , reply(makeRepeatedPayload(serverReply, serverReplySize)) {
        }

    private:
        void onConnected() override {
            ++testState.serverConnectedCount;
        }
        void onDisconnected() override {
        }
        std::size_t onReceivedFromPeer() override {
            char chunk[4096];
            const std::size_t n = readFromPeer(chunk, sizeof(chunk));
            if (n > 0) {
                received.append(chunk, n);
                if (received.size() > expected.size()) {
                    ++testState.unexpectedPayloadCount;
                    core::SNodeC::stop();
                } else if (received.size() == expected.size()) {
                    if (received == expected) {
                        ++testState.serverLargePayloadReceivedCount;
                        sendToPeer(reply.data(), reply.size());
                    } else {
                        ++testState.unexpectedPayloadCount;
                        core::SNodeC::stop();
                    }
                }
            }
            return n;
        }
        bool onSignal([[maybe_unused]] int signum) override {
            return true;
        }
        TestState& testState;
        std::string received;
        std::string expected;
        std::string reply;
    };
    class TestClientSocketContext : public core::socket::stream::SocketContext {
    public:
        TestClientSocketContext(core::socket::stream::SocketConnection* c, TestState& s)
            : SocketContext(c)
            , testState(s)
            , payload(makeRepeatedPayload(clientPayload, clientPayloadSize))
            , expected(makeRepeatedPayload(serverReply, serverReplySize)) {
        }

    private:
        void onConnected() override {
            ++testState.clientConnectedCount;
            sendToPeer(payload.data(), payload.size());
        }
        void onDisconnected() override {
        }
        std::size_t onReceivedFromPeer() override {
            char chunk[4096];
            const std::size_t n = readFromPeer(chunk, sizeof(chunk));
            if (n > 0) {
                received.append(chunk, n);
                if (received.size() > expected.size()) {
                    ++testState.unexpectedPayloadCount;
                    core::SNodeC::stop();
                } else if (received.size() == expected.size()) {
                    if (received == expected) {
                        ++testState.clientLargeReplyReceivedCount;
                    } else {
                        ++testState.unexpectedPayloadCount;
                    }
                    core::SNodeC::stop();
                }
            }
            return n;
        }
        bool onSignal([[maybe_unused]] int signum) override {
            return true;
        }
        TestState& testState;
        std::string payload;
        std::string received;
        std::string expected;
    };
    class TestServerSocketContextFactory : public core::socket::stream::SocketContextFactory {
    public:
        explicit TestServerSocketContextFactory(TestState& s)
            : testState(s) {
        }
        core::socket::stream::SocketContext* create(core::socket::stream::SocketConnection* c) override {
            ++testState.serverFactoryCreateCount;
            return new TestServerSocketContext(c, testState);
        }

    private:
        TestState& testState;
    };
    class TestClientSocketContextFactory : public core::socket::stream::SocketContextFactory {
    public:
        explicit TestClientSocketContextFactory(TestState& s)
            : testState(s) {
        }
        core::socket::stream::SocketContext* create(core::socket::stream::SocketConnection* c) override {
            ++testState.clientFactoryCreateCount;
            return new TestClientSocketContext(c, testState);
        }

    private:
        TestState& testState;
    };
} // namespace
int main(int argc, char* argv[]) {
    tests::support::TestResult testResult;
    int result = tests::support::cTestSkipReturnCode;
    if (tests::support::shouldSkipRootWithoutSNodeCGroup()) {
        tests::support::printRootWithoutSNodeCGroupSkipMessage("Inet6LegacyServerClientLargePayloadExchangeTest");
    } else {
        TestState testState;
        core::SNodeC::init(argc, argv);

        net::in6::stream::legacy::SocketClient<TestClientSocketContextFactory, TestState&> socketClient("ipv6-large-payload-client",
                                                                                                        testState);
        const net::in6::stream::legacy::SocketServer<TestServerSocketContextFactory, TestState&> socketServer("ipv6-large-payload-server",
                                                                                                              testState);
        socketClient.getConfig()->Instance::forceUnrequired();
        socketServer.getConfig()->Instance::forceUnrequired();
        socketServer.listen(net::in6::SocketAddress("::1", 0),
                            [&socketClient, &testState](const net::in6::SocketAddress& socketAddress, core::socket::State state) {
                                if (state == core::socket::State::OK) {
                                    ++testState.serverListenOkCount;
                                    const std::uint16_t effectivePort = socketAddress.getPort();
                                    if (effectivePort != 0) {
                                        ++testState.effectiveListenPortOkCount;
                                        const auto connectCallback = [&testState](const net::in6::SocketAddress&,
                                                                                  core::socket::State connectState) {
                                            if (connectState == core::socket::State::OK) {
                                                ++testState.clientConnectOkCount;
                                            } else {
                                                ++testState.unexpectedStateCount;
                                                core::SNodeC::stop();
                                            }
                                        };
                                        socketClient.connect(net::in6::SocketAddress("::1", effectivePort), connectCallback);
                                    } else {
                                        ++testState.zeroEffectiveListenPortCount;
                                        core::SNodeC::stop();
                                    }
                                } else {
                                    ++testState.unexpectedStateCount;
                                    core::SNodeC::stop();
                                }
                            });

        const int startResult = core::SNodeC::start(utils::Timeval({1, 0}));
        testResult.expectEqual(0, startResult, "event loop stops successfully");
        testResult.expectEqual(1, testState.serverListenOkCount, "listen ok");
        testResult.expectEqual(1, testState.effectiveListenPortOkCount, "effective port");
        testResult.expectEqual(0, testState.zeroEffectiveListenPortCount, "zero port");
        testResult.expectEqual(1, testState.clientConnectOkCount, "connect ok");
        testResult.expectEqual(1, testState.serverFactoryCreateCount, "server factory");
        testResult.expectEqual(1, testState.clientFactoryCreateCount, "client factory");
        testResult.expectEqual(1, testState.serverConnectedCount, "server connected");
        testResult.expectEqual(1, testState.clientConnectedCount, "client connected");
        testResult.expectEqual(1, testState.serverLargePayloadReceivedCount, "server large payload");
        testResult.expectEqual(1, testState.clientLargeReplyReceivedCount, "client large reply");
        testResult.expectEqual(0, testState.unexpectedStateCount, "unexpected states");
        testResult.expectEqual(0, testState.unexpectedPayloadCount, "unexpected payloads");
        core::SNodeC::free();
        result = testResult.processResult();
    }
    return result;
}
