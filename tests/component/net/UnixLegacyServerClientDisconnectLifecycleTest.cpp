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
#include "net/un/SocketAddress.h"
#include "net/un/stream/legacy/SocketClient.h"
#include "net/un/stream/legacy/SocketServer.h"
#include "support/TestResult.h"
#include "utils/Timeval.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef>
#include <cstdio>
#include <string>
#include <string_view>
#include <unistd.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace {

    constexpr std::string_view clientPayload = "snodec-unix-disconnect-request";
    constexpr std::string_view serverPayload = "snodec-unix-disconnect-ack";

    struct TestState {
        int serverListenOkCount = 0;
        int clientConnectOkCount = 0;
        int serverFactoryCreateCount = 0;
        int clientFactoryCreateCount = 0;
        int serverConnectedCount = 0;
        int clientConnectedCount = 0;
        int serverPayloadReceivedCount = 0;
        int clientAckReceivedCount = 0;
        int clientCloseIssuedCount = 0;
        int serverDisconnectedCount = 0;
        int clientDisconnectedCount = 0;
        int unexpectedStateCount = 0;
        int unexpectedPayloadCount = 0;
    };

    std::string makeSocketPath() {
        return "/tmp/snodec-unix-disconnect-" + std::to_string(getpid()) + ".sock";
    }

    bool socketPathExists(const std::string& socketPath) {
        return access(socketPath.c_str(), F_OK) == 0;
    }

    void stopWhenDisconnectLifecycleIsComplete(TestState& testState) {
        if (testState.clientDisconnectedCount == 1 && testState.serverDisconnectedCount == 1) {
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
            ++testState.serverDisconnectedCount;
            stopWhenDisconnectLifecycleIsComplete(testState);
        }

        std::size_t onReceivedFromPeer() override {
            char chunk[4096];

            const std::size_t chunkLen = readFromPeer(chunk, sizeof(chunk));

            if (chunkLen > 0) {
                const std::string payload(chunk, chunkLen);

                if (payload == clientPayload) {
                    ++testState.serverPayloadReceivedCount;
                    sendToPeer(serverPayload.data(), serverPayload.size());
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

    class TestClientSocketContext : public core::socket::stream::SocketContext {
    public:
        TestClientSocketContext(core::socket::stream::SocketConnection* socketConnection, TestState& testState)
            : core::socket::stream::SocketContext(socketConnection)
            , testState(testState) {
        }

    private:
        void onConnected() override {
            ++testState.clientConnectedCount;
            sendToPeer(clientPayload.data(), clientPayload.size());
        }

        void onDisconnected() override {
            ++testState.clientDisconnectedCount;
            stopWhenDisconnectLifecycleIsComplete(testState);
        }

        std::size_t onReceivedFromPeer() override {
            char chunk[4096];

            const std::size_t chunkLen = readFromPeer(chunk, sizeof(chunk));

            if (chunkLen > 0) {
                const std::string payload(chunk, chunkLen);

                if (payload == serverPayload) {
                    ++testState.clientAckReceivedCount;
                    close();
                    ++testState.clientCloseIssuedCount;
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

    class TestClientSocketContextFactory : public core::socket::stream::SocketContextFactory {
    public:
        explicit TestClientSocketContextFactory(TestState& testState)
            : testState(testState) {
        }

        core::socket::stream::SocketContext* create(core::socket::stream::SocketConnection* socketConnection) override {
            ++testState.clientFactoryCreateCount;
            core::socket::stream::SocketContext* socketContext = new TestClientSocketContext(socketConnection, testState);

            return socketContext;
        }

    private:
        TestState& testState;
    };

} // namespace

int main(int argc, char* argv[]) {
    tests::support::TestResult testResult;
    int result = tests::support::cTestSkipReturnCode;
    const std::string socketPath = makeSocketPath();

    if (tests::support::shouldSkipRootWithoutSNodeCGroup()) {
        tests::support::printRootWithoutSNodeCGroupSkipMessage("UnixLegacyServerClientDisconnectLifecycleTest");
    } else {
        TestState testState;
        std::remove(socketPath.c_str());
        const bool socketPathAbsentBeforeListen = !socketPathExists(socketPath);

        core::SNodeC::init(argc, argv);

        net::un::stream::legacy::SocketClient<TestClientSocketContextFactory, TestState&> socketClient("unix-disconnect-client", testState);
        const net::un::stream::legacy::SocketServer<TestServerSocketContextFactory, TestState&> socketServer("unix-disconnect-server",
                                                                                                             testState);

        socketClient.getConfig()->Instance::forceUnrequired();
        socketServer.getConfig()->Instance::forceUnrequired();

        socketServer.listen(socketPath,
                            [&socketClient, &socketPath, &testState](const net::un::SocketAddress&, core::socket::State state) {
                                if (state == core::socket::State::OK) {
                                    ++testState.serverListenOkCount;
                                    socketClient.connect(socketPath,
                                                         [&testState](const net::un::SocketAddress&, core::socket::State connectState) {
                                                             if (connectState == core::socket::State::OK) {
                                                                 ++testState.clientConnectOkCount;
                                                             } else {
                                                                 ++testState.unexpectedStateCount;
                                                                 core::SNodeC::stop();
                                                             }
                                                         });
                                } else {
                                    ++testState.unexpectedStateCount;
                                    core::SNodeC::stop();
                                }
                            });

        const int startResult = core::SNodeC::start(utils::Timeval({1, 0}));

        std::remove(socketPath.c_str());
        const bool socketPathAbsentAfterCleanup = !socketPathExists(socketPath);

        testResult.expectTrue(socketPathAbsentBeforeListen, "Unix-domain socket path is absent before listen");
        testResult.expectEqual(0, startResult, "event loop stops successfully after controlled Unix-domain disconnect lifecycle");
        testResult.expectEqual(1, testState.serverListenOkCount, "Unix-domain legacy server listen callback reports OK exactly once");
        testResult.expectEqual(1, testState.clientConnectOkCount, "Unix-domain legacy client connect callback reports OK exactly once");
        testResult.expectEqual(1, testState.serverFactoryCreateCount, "server socket context factory creates exactly one context");
        testResult.expectEqual(1, testState.clientFactoryCreateCount, "client socket context factory creates exactly one context");
        testResult.expectEqual(1, testState.serverConnectedCount, "server socket context reaches onConnected exactly once");
        testResult.expectEqual(1, testState.clientConnectedCount, "client socket context reaches onConnected exactly once");
        testResult.expectEqual(1, testState.serverPayloadReceivedCount, "server receives and verifies the client payload exactly once");
        testResult.expectEqual(1, testState.clientAckReceivedCount, "client receives and verifies the server ack exactly once");
        testResult.expectEqual(1, testState.clientCloseIssuedCount, "client issues controlled close exactly once after ack");
        testResult.expectEqual(1, testState.serverDisconnectedCount, "server socket context reaches onDisconnected exactly once");
        testResult.expectEqual(1, testState.clientDisconnectedCount, "client socket context reaches onDisconnected exactly once");
        testResult.expectEqual(0, testState.unexpectedStateCount, "listen and connect callbacks report no unexpected states");
        testResult.expectEqual(0, testState.unexpectedPayloadCount, "server and client receive no unexpected payloads");
        testResult.expectTrue(socketPathAbsentAfterCleanup, "Unix-domain socket path is absent after cleanup");

        result = testResult.processResult();

        core::SNodeC::free();
    }

    std::remove(socketPath.c_str());

    return result;
}
