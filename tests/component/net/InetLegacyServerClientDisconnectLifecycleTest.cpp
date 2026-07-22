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
#include "log/Logger.h"
#include "net/in/SocketAddress.h"
#include "net/in/stream/legacy/SocketClient.h"
#include "net/in/stream/legacy/SocketServer.h"
#include "support/TestResult.h"
#include "utils/Timeval.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace {

    struct CapturedRecord {
        std::string level;
        std::string origin;
        std::string boundary;
        std::string component;
        std::optional<std::string> instance;
        std::optional<std::string> role;
        std::optional<std::string> connection;
        std::string message;
    };

    std::vector<CapturedRecord> readLogRecords(const std::filesystem::path& path) {
        std::vector<CapturedRecord> records;
        std::ifstream input(path);
        std::string line;
        while (std::getline(input, line)) {
            if (!line.empty()) {
                const auto json = nlohmann::json::parse(line);
                records.push_back(
                    {.level = json.at("level").get<std::string>(),
                     .origin = json.at("origin").get<std::string>(),
                     .boundary = json.at("boundary").get<std::string>(),
                     .component = json.at("component").get<std::string>(),
                     .instance =
                         json.contains("instance") ? std::optional<std::string>(json.at("instance").get<std::string>()) : std::nullopt,
                     .role = json.contains("role") ? std::optional<std::string>(json.at("role").get<std::string>()) : std::nullopt,
                     .connection =
                         json.contains("connection") ? std::optional<std::string>(json.at("connection").get<std::string>()) : std::nullopt,
                     .message = json.at("message").get<std::string>()});
            }
        }
        return records;
    }

    std::vector<CapturedRecord> recordsFor(const std::vector<CapturedRecord>& records, const std::string& message) {
        std::vector<CapturedRecord> matches;
        std::copy_if(records.begin(), records.end(), std::back_inserter(matches), [&message](const CapturedRecord& record) {
            return record.message == message;
        });
        return matches;
    }

    std::vector<CapturedRecord>
    recordsFor(const std::vector<CapturedRecord>& records, const std::string& message, const std::string& instance) {
        std::vector<CapturedRecord> matches;
        std::copy_if(records.begin(), records.end(), std::back_inserter(matches), [&message, &instance](const CapturedRecord& record) {
            return record.message == message && record.instance == std::optional<std::string>(instance);
        });
        return matches;
    }

    constexpr std::string_view clientPayload = "snodec-ipv4-disconnect-request";
    constexpr std::string_view serverPayload = "snodec-ipv4-disconnect-ack";

    struct TestState {
        int serverListenOkCount = 0;
        int conflictingListenFailureCount = 0;
        int effectiveListenPortOkCount = 0;
        int zeroEffectiveListenPortCount = 0;
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

    if (tests::support::shouldSkipRootWithoutSNodeCGroup()) {
        tests::support::printRootWithoutSNodeCGroupSkipMessage("InetLegacyServerClientDisconnectLifecycleTest");
    } else {
        TestState testState;

        const auto logPath = std::filesystem::temp_directory_path() / "snodec-phase2-ipv4-transport.jsonl";
        std::error_code removeError;
        std::filesystem::remove(logPath, removeError);
        logger::Logger::init();
        logger::LogManager::init();
        logger::Logger::setQuiet(true);
        logger::Logger::setDisableColor(true);
        logger::Logger::logToFile(logPath.string());

        char arg0[] = "InetLegacyServerClientDisconnectLifecycleTest";
        char arg1[] = "--log-level=6";
        char arg2[] = "--log-format=json";
        char arg3[] = "--quiet";
        char* logArgs[] = {arg0, arg1, arg2, arg3, nullptr};
        core::SNodeC::init(4, logArgs);

        net::in::stream::legacy::SocketClient<TestClientSocketContextFactory, TestState&> socketClient("ipv4-disconnect-client", testState);
        const net::in::stream::legacy::SocketServer<TestServerSocketContextFactory, TestState&> socketServer("ipv4-disconnect-server",
                                                                                                             testState);
        const net::in::stream::legacy::SocketServer<TestServerSocketContextFactory, TestState&> conflictingSocketServer(
            "ipv4-conflicting-server", testState);

        socketClient.getConfig()->Instance::forceUnrequired();
        socketServer.getConfig()->Instance::forceUnrequired();
        conflictingSocketServer.getConfig()->Instance::forceUnrequired();

        socketServer.listen(
            net::in::SocketAddress("127.0.0.1", 0),
            [&socketClient, &conflictingSocketServer, &testState](const net::in::SocketAddress& socketAddress, core::socket::State state) {
                if (state == core::socket::State::OK) {
                    ++testState.serverListenOkCount;

                    const std::uint16_t effectivePort = socketAddress.getPort();
                    if (effectivePort != 0) {
                        ++testState.effectiveListenPortOkCount;
                        conflictingSocketServer.listen(
                            net::in::SocketAddress("127.0.0.1", effectivePort),
                            [&socketClient, &testState, effectivePort](const net::in::SocketAddress&,
                                                                       core::socket::State conflictingState) {
                                if (conflictingState != core::socket::State::OK) {
                                    ++testState.conflictingListenFailureCount;
                                    socketClient.connect(net::in::SocketAddress("127.0.0.1", effectivePort),
                                                         [&testState](const net::in::SocketAddress&, core::socket::State connectState) {
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

        testResult.expectEqual(0, startResult, "event loop stops successfully after controlled IPv4 disconnect lifecycle");
        testResult.expectEqual(1, testState.serverListenOkCount, "IPv4 legacy server listen callback reports OK exactly once");
        testResult.expectEqual(1, testState.conflictingListenFailureCount, "conflicting IPv4 listener reports one startup failure");
        testResult.expectEqual(1, testState.effectiveListenPortOkCount, "IPv4 legacy listen callback reports one non-zero effective port");
        testResult.expectEqual(0, testState.zeroEffectiveListenPortCount, "IPv4 legacy listen callback never reports port zero");
        testResult.expectEqual(1, testState.clientConnectOkCount, "IPv4 legacy client connect callback reports OK exactly once");
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

        core::SNodeC::free();
        logger::Logger::disableLogToFile();

        const auto records = readLogRecords(logPath);
        const auto listenerStarted = recordsFor(records, "listener started", "ipv4-disconnect-server");
        const auto listenerStopped = recordsFor(records, "listener stopped", "ipv4-disconnect-server");
        const auto successfulListenerStartFailed = recordsFor(records, "listener start failed", "ipv4-disconnect-server");
        const auto listenerStartFailed = recordsFor(records, "listener start failed", "ipv4-conflicting-server");
        const auto attemptStarted = recordsFor(records, "connection attempt started");
        const auto attemptSucceeded = recordsFor(records, "connection attempt succeeded");
        const auto connected = recordsFor(records, "transport connected");
        const auto disconnected = recordsFor(records, "transport disconnected");

        testResult.expectEqual(1, static_cast<int>(listenerStarted.size()), "listener start is emitted once");
        testResult.expectEqual(1, static_cast<int>(listenerStopped.size()), "listener stop is emitted once");
        testResult.expectEqual(
            0, static_cast<int>(successfulListenerStartFailed.size()), "successful listener emits no startup failure record");
        testResult.expectEqual(1, static_cast<int>(listenerStartFailed.size()), "failed listener emits one startup terminal record");
        testResult.expectEqual(1, static_cast<int>(attemptStarted.size()), "client attempt start is emitted once");
        testResult.expectEqual(1, static_cast<int>(attemptSucceeded.size()), "client attempt success is emitted once");
        testResult.expectEqual(2, static_cast<int>(connected.size()), "client and accepted server transports each connect once");
        testResult.expectEqual(2, static_cast<int>(disconnected.size()), "client and accepted server transports each disconnect once");

        for (const auto& record : listenerStarted) {
            testResult.expectTrue(record.level == "info" && record.origin == "framework" && record.boundary == "instance" &&
                                      record.component == "core.socket.stream" &&
                                      record.instance == std::optional<std::string>("ipv4-disconnect-server") &&
                                      record.role == std::optional<std::string>("server") && !record.connection,
                                  "listener start carries server endpoint identity at Info");
        }
        for (const auto& record : listenerStopped) {
            testResult.expectTrue(record.level == "info" && record.origin == "framework" && record.boundary == "instance" &&
                                      record.component == "core.socket.stream" &&
                                      record.instance == std::optional<std::string>("ipv4-disconnect-server") &&
                                      record.role == std::optional<std::string>("server") && !record.connection,
                                  "listener stop carries matching server endpoint identity at Info");
        }
        for (const auto& record : listenerStartFailed) {
            testResult.expectTrue(record.level == "debug" && record.boundary == "instance" &&
                                      record.instance == std::optional<std::string>("ipv4-conflicting-server") &&
                                      record.role == std::optional<std::string>("server"),
                                  "listener startup failure carries server endpoint identity at Debug");
        }
        for (const auto& record : attemptStarted) {
            testResult.expectTrue(record.level == "debug" && record.boundary == "instance" &&
                                      record.instance == std::optional<std::string>("ipv4-disconnect-client") &&
                                      record.role == std::optional<std::string>("client") && !record.connection,
                                  "attempt start carries client endpoint identity at Debug");
        }
        for (const auto& record : attemptSucceeded) {
            testResult.expectTrue(record.level == "debug" && record.instance == std::optional<std::string>("ipv4-disconnect-client") &&
                                      record.role == std::optional<std::string>("client"),
                                  "attempt success carries matching client endpoint identity at Debug");
        }
        for (const auto& record : connected) {
            testResult.expectTrue(record.level == "info" && record.boundary == "connection" && record.component == "core.socket.stream" &&
                                      record.connection == std::optional<std::string>("1") && record.role.has_value(),
                                  "transport start carries role-aware connection identity at Info");
        }
        for (const auto& record : disconnected) {
            testResult.expectTrue(record.level == "info" && record.boundary == "connection" && record.component == "core.socket.stream" &&
                                      record.connection == std::optional<std::string>("1") && record.role.has_value(),
                                  "transport end carries role-aware connection identity at Info");
        }

        logger::Logger::init();
        logger::LogManager::init();
        result = testResult.processResult();
    }

    return result;
}
