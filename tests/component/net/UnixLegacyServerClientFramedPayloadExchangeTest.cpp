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
#include "core/timer/Timer.h"
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
#include <vector>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace {

    constexpr std::string_view firstClientPayload = "snodec-unix-framed-request-one";
    constexpr std::string_view secondClientPayload = "snodec-unix-framed-request-two";
    constexpr std::string_view firstServerReply = "snodec-unix-framed-reply-one";
    constexpr std::string_view secondServerReply = "snodec-unix-framed-reply-two";

    enum class FrameParseResult {
        Incomplete,
        Complete,
        Malformed
    };

    std::string encodeFrame(std::string_view payload) {
        return std::to_string(payload.size()) + "\n" + std::string(payload);
    }

    std::vector<std::string> makeFragmentPlan(const std::string& stream) {
        return {stream.substr(0, 1), stream.substr(1, 4), stream.substr(5, 11), stream.substr(16)};
    }

    FrameParseResult tryExtractFrame(std::string& buffer, std::string& framePayload) {
        const std::size_t newlinePos = buffer.find('\n');
        if (newlinePos == std::string::npos) {
            return FrameParseResult::Incomplete;
        }
        if (newlinePos == 0) {
            return FrameParseResult::Malformed;
        }
        std::size_t payloadSize = 0;
        for (std::size_t i = 0; i < newlinePos; ++i) {
            const char ch = buffer[i];
            if (ch < '0' || ch > '9') {
                return FrameParseResult::Malformed;
            }
            payloadSize = payloadSize * 10 + static_cast<std::size_t>(ch - '0');
        }
        const std::size_t frameSize = newlinePos + 1 + payloadSize;
        if (buffer.size() < frameSize) {
            return FrameParseResult::Incomplete;
        }
        framePayload = buffer.substr(newlinePos + 1, payloadSize);
        buffer.erase(0, frameSize);
        return FrameParseResult::Complete;
    }

    struct TestState {
        int serverListenOkCount = 0;
        int clientConnectOkCount = 0;
        int serverFactoryCreateCount = 0;
        int clientFactoryCreateCount = 0;
        int serverConnectedCount = 0;
        int clientConnectedCount = 0;
        int clientRequestFragmentSentCount = 0;
        int serverReplyFragmentSentCount = 0;
        int serverFirstFrameReceivedCount = 0;
        int serverSecondFrameReceivedCount = 0;
        int clientFirstReplyFrameReceivedCount = 0;
        int clientSecondReplyFrameReceivedCount = 0;
        int malformedFrameCount = 0;
        int unexpectedStateCount = 0;
        int unexpectedPayloadCount = 0;
    };

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
                receiveBuffer.append(chunk, chunkLen);
                while (true) {
                    std::string framePayload;
                    const FrameParseResult parseResult = tryExtractFrame(receiveBuffer, framePayload);
                    if (parseResult == FrameParseResult::Complete) {
                        handleFrame(framePayload);
                    } else if (parseResult == FrameParseResult::Incomplete) {
                        break;
                    } else {
                        ++testState.malformedFrameCount;
                        core::SNodeC::stop();
                        break;
                    }
                }
            }
            return chunkLen;
        }
        bool onSignal([[maybe_unused]] int signum) override {
            return true;
        }
        void handleFrame(const std::string& framePayload) {
            if (testState.serverFirstFrameReceivedCount == 0 && framePayload == firstClientPayload) {
                ++testState.serverFirstFrameReceivedCount;
            } else if (testState.serverFirstFrameReceivedCount == 1 && testState.serverSecondFrameReceivedCount == 0 && framePayload == secondClientPayload) {
                ++testState.serverSecondFrameReceivedCount;
                const std::string replyStream = encodeFrame(firstServerReply) + encodeFrame(secondServerReply);
                replyFragments = makeFragmentPlan(replyStream);
                sendNextReplyFragment();
            } else {
                ++testState.unexpectedPayloadCount;
                core::SNodeC::stop();
            }
        }
        void sendNextReplyFragment() {
            if (nextReplyFragmentIndex < replyFragments.size()) {
                const std::string& fragment = replyFragments[nextReplyFragmentIndex++];
                sendToPeer(fragment.data(), fragment.size());
                ++testState.serverReplyFragmentSentCount;
                if (nextReplyFragmentIndex < replyFragments.size()) {
                    sendTimer = core::timer::Timer::singleshotTimer([this]() { sendNextReplyFragment(); }, utils::Timeval({0, 1000}));
                }
            }
        }
        TestState& testState;
        std::string receiveBuffer;
        std::vector<std::string> replyFragments;
        std::size_t nextReplyFragmentIndex = 0;
        core::timer::Timer sendTimer;
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
            const std::string requestStream = encodeFrame(firstClientPayload) + encodeFrame(secondClientPayload);
            requestFragments = makeFragmentPlan(requestStream);
            sendNextRequestFragment();
        }
        void onDisconnected() override {
        }
        std::size_t onReceivedFromPeer() override {
            char chunk[4096];
            const std::size_t chunkLen = readFromPeer(chunk, sizeof(chunk));
            if (chunkLen > 0) {
                receiveBuffer.append(chunk, chunkLen);
                while (true) {
                    std::string framePayload;
                    const FrameParseResult parseResult = tryExtractFrame(receiveBuffer, framePayload);
                    if (parseResult == FrameParseResult::Complete) {
                        handleFrame(framePayload);
                    } else if (parseResult == FrameParseResult::Incomplete) {
                        break;
                    } else {
                        ++testState.malformedFrameCount;
                        core::SNodeC::stop();
                        break;
                    }
                }
            }
            return chunkLen;
        }
        bool onSignal([[maybe_unused]] int signum) override {
            return true;
        }
        void handleFrame(const std::string& framePayload) {
            if (testState.clientFirstReplyFrameReceivedCount == 0 && framePayload == firstServerReply) {
                ++testState.clientFirstReplyFrameReceivedCount;
            } else if (testState.clientFirstReplyFrameReceivedCount == 1 && testState.clientSecondReplyFrameReceivedCount == 0 && framePayload == secondServerReply) {
                ++testState.clientSecondReplyFrameReceivedCount;
                core::SNodeC::stop();
            } else {
                ++testState.unexpectedPayloadCount;
                core::SNodeC::stop();
            }
        }
        void sendNextRequestFragment() {
            if (nextRequestFragmentIndex < requestFragments.size()) {
                const std::string& fragment = requestFragments[nextRequestFragmentIndex++];
                sendToPeer(fragment.data(), fragment.size());
                ++testState.clientRequestFragmentSentCount;
                if (nextRequestFragmentIndex < requestFragments.size()) {
                    sendTimer = core::timer::Timer::singleshotTimer([this]() { sendNextRequestFragment(); }, utils::Timeval({0, 1000}));
                }
            }
        }
        TestState& testState;
        std::string receiveBuffer;
        std::vector<std::string> requestFragments;
        std::size_t nextRequestFragmentIndex = 0;
        core::timer::Timer sendTimer;
    };

    class TestServerSocketContextFactory : public core::socket::stream::SocketContextFactory {
    public:
        explicit TestServerSocketContextFactory(TestState& testState)
            : testState(testState) {
        }
        core::socket::stream::SocketContext* create(core::socket::stream::SocketConnection* socketConnection) override {
            ++testState.serverFactoryCreateCount;
            return new TestServerSocketContext(socketConnection, testState);
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
            return new TestClientSocketContext(socketConnection, testState);
        }
    private:
        TestState& testState;
    };

    std::string makeSocketPath() {
        return "/tmp/snodec-unix-framed-" + std::to_string(getpid()) + ".sock";
    }

    bool socketPathExists(const std::string& socketPath) {
        return ::access(socketPath.c_str(), F_OK) == 0;
    }

} // namespace

int main(int argc, char* argv[]) {
    tests::support::TestResult testResult;
    int result = tests::support::cTestSkipReturnCode;
    const std::string socketPath = makeSocketPath();
    if (tests::support::shouldSkipRootWithoutSNodeCGroup()) {
        tests::support::printRootWithoutSNodeCGroupSkipMessage("UnixLegacyServerClientFramedPayloadExchangeTest");
    } else {
        TestState testState;
        std::remove(socketPath.c_str());
        const bool socketPathAbsentBeforeListen = !socketPathExists(socketPath);
        core::SNodeC::init(argc, argv);
        net::un::stream::legacy::SocketClient<TestClientSocketContextFactory, TestState&> socketClient("unix-framed-client", testState);
        const net::un::stream::legacy::SocketServer<TestServerSocketContextFactory, TestState&> socketServer("unix-framed-server", testState);
        socketClient.getConfig()->Instance::forceUnrequired();
        socketServer.getConfig()->Instance::forceUnrequired();
        socketServer.listen(socketPath, [&socketClient, &socketPath, &testState](const net::un::SocketAddress&, core::socket::State state) {
            if (state == core::socket::State::OK) {
                ++testState.serverListenOkCount;
                socketClient.connect(socketPath, [&testState](const net::un::SocketAddress&, core::socket::State connectState) {
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
        testResult.expectEqual(0, startResult, "event loop stops successfully after framed Unix-domain request/reply exchange");
        testResult.expectEqual(1, testState.serverListenOkCount, "Unix-domain legacy server listen callback reports OK exactly once");
        testResult.expectEqual(1, testState.clientConnectOkCount, "Unix-domain legacy client connect callback reports OK exactly once");
        testResult.expectEqual(1, testState.serverFactoryCreateCount, "server socket context factory creates exactly one context");
        testResult.expectEqual(1, testState.clientFactoryCreateCount, "client socket context factory creates exactly one context");
        testResult.expectEqual(1, testState.serverConnectedCount, "server socket context reaches onConnected exactly once");
        testResult.expectEqual(1, testState.clientConnectedCount, "client socket context reaches onConnected exactly once");
        testResult.expectEqual(4, testState.clientRequestFragmentSentCount, "client sends request stream in four staged fragments");
        testResult.expectEqual(4, testState.serverReplyFragmentSentCount, "server sends reply stream in four staged fragments");
        testResult.expectEqual(1, testState.serverFirstFrameReceivedCount, "server reconstructs the first client frame exactly once");
        testResult.expectEqual(1, testState.serverSecondFrameReceivedCount, "server reconstructs the second client frame exactly once");
        testResult.expectEqual(1, testState.clientFirstReplyFrameReceivedCount, "client reconstructs the first server reply frame exactly once");
        testResult.expectEqual(1, testState.clientSecondReplyFrameReceivedCount, "client reconstructs the second server reply frame exactly once");
        testResult.expectEqual(0, testState.malformedFrameCount, "framed parser reports no malformed frames");
        testResult.expectEqual(0, testState.unexpectedStateCount, "listen and connect callbacks report no unexpected states");
        testResult.expectEqual(0, testState.unexpectedPayloadCount, "server and client receive no unexpected framed payloads");
        testResult.expectTrue(socketPathAbsentAfterCleanup, "Unix-domain socket path is absent after cleanup");
        result = testResult.processResult();
        core::SNodeC::free();
    }
    std::remove(socketPath.c_str());
    return result;
}
