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

    constexpr std::string_view clientAPayload = "snodec-ipv6-client-a-request";
    constexpr std::string_view clientBPayload = "snodec-ipv6-client-b-request";
    constexpr std::string_view clientAReply = "snodec-ipv6-client-a-reply";
    constexpr std::string_view clientBReply = "snodec-ipv6-client-b-reply";

    struct TestState {
        int serverListenOkCount = 0;
        int effectiveListenPortOkCount = 0;
        int zeroEffectiveListenPortCount = 0;
        int clientConnectOkCount = 0;
        int serverFactoryCreateCount = 0;
        int clientAFactoryCreateCount = 0;
        int clientBFactoryCreateCount = 0;
        int serverConnectedCount = 0;
        int clientAConnectedCount = 0;
        int clientBConnectedCount = 0;
        int serverClientAPayloadReceivedCount = 0;
        int serverClientBPayloadReceivedCount = 0;
        int clientAReplyReceivedCount = 0;
        int clientBReplyReceivedCount = 0;
        int unexpectedStateCount = 0;
        int unexpectedPayloadCount = 0;
    };

    void stopWhenBothRepliesAreReceived(TestState& testState) {
        if (testState.clientAReplyReceivedCount == 1 && testState.clientBReplyReceivedCount == 1) {
            core::SNodeC::stop();
        }
    }

    class TestServerSocketContext : public core::socket::stream::SocketContext {
    public:
        TestServerSocketContext(core::socket::stream::SocketConnection* socketConnection, TestState& testState)
            : core::socket::stream::SocketContext(socketConnection)
            , testState(testState) {
        }

    private:
        void onConnected() override {
            ++testState.serverConnectedCount;
        }

        void onDisconnected() override {
        }

        std::size_t onReceivedFromPeer() override {
            char chunk[4096];

            const std::size_t chunkLen = readFromPeer(chunk, sizeof(chunk));

            if (chunkLen > 0) {
                const std::string payload(chunk, chunkLen);

                if (payload == clientAPayload) {
                    ++testState.serverClientAPayloadReceivedCount;
                    sendToPeer(clientAReply.data(), clientAReply.size());
                } else if (payload == clientBPayload) {
                    ++testState.serverClientBPayloadReceivedCount;
                    sendToPeer(clientBReply.data(), clientBReply.size());
                } else {
                    ++testState.unexpectedPayloadCount;
                    core::SNodeC::stop();
                }
            }

            return chunkLen;
        }

        bool onSignal([[maybe_unused]] int signum) override {
            return true;
        }

        TestState& testState;
    };

    class TestClientASocketContext : public core::socket::stream::SocketContext {
    public:
        TestClientASocketContext(core::socket::stream::SocketConnection* socketConnection, TestState& testState)
            : core::socket::stream::SocketContext(socketConnection)
            , testState(testState) {
        }

    private:
        void onConnected() override {
            ++testState.clientAConnectedCount;
            sendToPeer(clientAPayload.data(), clientAPayload.size());
        }

        void onDisconnected() override {
        }

        std::size_t onReceivedFromPeer() override {
            char chunk[4096];

            const std::size_t chunkLen = readFromPeer(chunk, sizeof(chunk));

            if (chunkLen > 0) {
                const std::string payload(chunk, chunkLen);

                if (payload == clientAReply) {
                    ++testState.clientAReplyReceivedCount;
                } else {
                    ++testState.unexpectedPayloadCount;
                }

                stopWhenBothRepliesAreReceived(testState);
            }

            return chunkLen;
        }

        bool onSignal([[maybe_unused]] int signum) override {
            return true;
        }

        TestState& testState;
    };

    class TestClientBSocketContext : public core::socket::stream::SocketContext {
    public:
        TestClientBSocketContext(core::socket::stream::SocketConnection* socketConnection, TestState& testState)
            : core::socket::stream::SocketContext(socketConnection)
            , testState(testState) {
        }

    private:
        void onConnected() override {
            ++testState.clientBConnectedCount;
            sendToPeer(clientBPayload.data(), clientBPayload.size());
        }

        void onDisconnected() override {
        }

        std::size_t onReceivedFromPeer() override {
            char chunk[4096];

            const std::size_t chunkLen = readFromPeer(chunk, sizeof(chunk));

            if (chunkLen > 0) {
                const std::string payload(chunk, chunkLen);

                if (payload == clientBReply) {
                    ++testState.clientBReplyReceivedCount;
                } else {
                    ++testState.unexpectedPayloadCount;
                }

                stopWhenBothRepliesAreReceived(testState);
            }

            return chunkLen;
        }

        bool onSignal([[maybe_unused]] int signum) override {
            return true;
        }

        TestState& testState;
    };

    class TestServerSocketContextFactory : public core::socket::stream::SocketContextFactory {
    public:
        explicit TestServerSocketContextFactory(TestState& testState)
            : testState(testState) {
        }

        core::socket::stream::SocketContext* create(core::socket::stream::SocketConnection* socketConnection) override {
            ++testState.serverFactoryCreateCount;
            core::socket::stream::SocketContext* socketContext = new TestServerSocketContext(socketConnection, testState);

            return socketContext;
        }

    private:
        TestState& testState;
    };

    class TestClientASocketContextFactory : public core::socket::stream::SocketContextFactory {
    public:
        explicit TestClientASocketContextFactory(TestState& testState)
            : testState(testState) {
        }

        core::socket::stream::SocketContext* create(core::socket::stream::SocketConnection* socketConnection) override {
            ++testState.clientAFactoryCreateCount;
            core::socket::stream::SocketContext* socketContext = new TestClientASocketContext(socketConnection, testState);

            return socketContext;
        }

    private:
        TestState& testState;
    };

    class TestClientBSocketContextFactory : public core::socket::stream::SocketContextFactory {
    public:
        explicit TestClientBSocketContextFactory(TestState& testState)
            : testState(testState) {
        }

        core::socket::stream::SocketContext* create(core::socket::stream::SocketConnection* socketConnection) override {
            ++testState.clientBFactoryCreateCount;
            core::socket::stream::SocketContext* socketContext = new TestClientBSocketContext(socketConnection, testState);

            return socketContext;
        }

    private:
        TestState& testState;
    };

} // namespace

int main(int argc, char* argv[]) {
    tests::support::TestResult testResult;
    int result = tests::support::cTestSkipReturnCode;

    if (tests::support::shouldSkipRootWithoutSNodeCGroup()) {
        tests::support::printRootWithoutSNodeCGroupSkipMessage("Inet6LegacyServerMultipleClientsPayloadExchangeTest");
    } else {
        TestState testState;

        core::SNodeC::init(argc, argv);

        net::in6::stream::legacy::SocketClient<TestClientASocketContextFactory, TestState&> socketClientA("ipv6-multi-payload-client-a", testState);
        net::in6::stream::legacy::SocketClient<TestClientBSocketContextFactory, TestState&> socketClientB("ipv6-multi-payload-client-b", testState);
        const net::in6::stream::legacy::SocketServer<TestServerSocketContextFactory, TestState&> socketServer("ipv6-multi-payload-server",
                                                                                                             testState);

        socketClientA.getConfig()->Instance::forceUnrequired();
        socketClientB.getConfig()->Instance::forceUnrequired();
        socketServer.getConfig()->Instance::forceUnrequired();

        socketServer.listen(net::in6::SocketAddress("::1", 0),
                            [&socketClientA, &socketClientB, &testState](const net::in6::SocketAddress& socketAddress, core::socket::State state) {
                                if (state == core::socket::State::OK) {
                                    ++testState.serverListenOkCount;

                                    const std::uint16_t effectivePort = socketAddress.getPort();
                                    if (effectivePort != 0) {
                                        ++testState.effectiveListenPortOkCount;
                                        const auto connectCallback = [&testState](const net::in6::SocketAddress&, core::socket::State connectState) {
                                            if (connectState == core::socket::State::OK) {
                                                ++testState.clientConnectOkCount;
                                            } else {
                                                ++testState.unexpectedStateCount;
                                                core::SNodeC::stop();
                                            }
                                        };

                                        socketClientA.connect(net::in6::SocketAddress("::1", effectivePort), connectCallback);
                                        socketClientB.connect(net::in6::SocketAddress("::1", effectivePort), connectCallback);
                                    } else {
                                        ++testState.zeroEffectiveListenPortCount;
                                        ++testState.unexpectedStateCount;
                                        core::SNodeC::stop();
                                    }
                                } else {
                                    ++testState.unexpectedStateCount;
                                    core::SNodeC::stop();
                                }
                            });

        const int startResult = core::SNodeC::start(utils::Timeval({1, 0}));

        testResult.expectEqual(0, startResult, "event loop stops successfully after both IPv6 client replies");
        testResult.expectEqual(1, testState.serverListenOkCount, "IPv6 legacy server listen callback reports OK exactly once");
        testResult.expectEqual(1, testState.effectiveListenPortOkCount, "IPv6 legacy listen callback reports one non-zero effective port");
        testResult.expectEqual(0, testState.zeroEffectiveListenPortCount, "IPv6 legacy listen callback never reports port zero");
        testResult.expectEqual(2, testState.clientConnectOkCount, "IPv6 legacy client connect callback reports OK exactly twice");
        testResult.expectEqual(2, testState.serverFactoryCreateCount, "server socket context factory creates exactly two contexts");
        testResult.expectEqual(1, testState.clientAFactoryCreateCount, "client-A socket context factory creates exactly one context");
        testResult.expectEqual(1, testState.clientBFactoryCreateCount, "client-B socket context factory creates exactly one context");
        testResult.expectEqual(2, testState.serverConnectedCount, "server socket context reaches onConnected exactly twice");
        testResult.expectEqual(1, testState.clientAConnectedCount, "client-A socket context reaches onConnected exactly once");
        testResult.expectEqual(1, testState.clientBConnectedCount, "client-B socket context reaches onConnected exactly once");
        testResult.expectEqual(1, testState.serverClientAPayloadReceivedCount, "server receives and verifies client A payload exactly once");
        testResult.expectEqual(1, testState.serverClientBPayloadReceivedCount, "server receives and verifies client B payload exactly once");
        testResult.expectEqual(1, testState.clientAReplyReceivedCount, "client A receives and verifies its reply exactly once");
        testResult.expectEqual(1, testState.clientBReplyReceivedCount, "client B receives and verifies its reply exactly once");
        testResult.expectEqual(0, testState.unexpectedStateCount, "listen and connect callbacks report no unexpected states");
        testResult.expectEqual(0, testState.unexpectedPayloadCount, "server and clients receive no unexpected payloads");

        result = testResult.processResult();

        core::SNodeC::free();
    }

    return result;
}
