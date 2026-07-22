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
#include "support/TestResult.h"
#include "utils/Timeval.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace {

    struct TestState {
        int clientConnectFailureCount = 0;
        int unexpectedConnectOkCount = 0;
        int clientFactoryCreateCount = 0;
        int clientConnectedCount = 0;
        int unexpectedStateCount = 0;
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
        }

        void onDisconnected() override {
        }

        std::size_t onReceivedFromPeer() override {
            const std::size_t processed = 0;

            return processed;
        }

        bool onSignal([[maybe_unused]] int signum) override {
            return true;
        }

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
        tests::support::printRootWithoutSNodeCGroupSkipMessage("InetLegacyClientConnectFailureTest");
    } else {
        TestState testState;

        const auto logPath = std::filesystem::temp_directory_path() / "snodec-phase2-ipv4-attempt-failure.jsonl";
        std::error_code removeError;
        std::filesystem::remove(logPath, removeError);
        logger::Logger::init();
        logger::LogManager::init();
        logger::Logger::setQuiet(true);
        logger::Logger::setDisableColor(true);
        logger::Logger::logToFile(logPath.string());

        char arg0[] = "InetLegacyClientConnectFailureTest";
        char arg1[] = "--log-level=6";
        char arg2[] = "--log-format=json";
        char arg3[] = "--quiet";
        char* logArgs[] = {arg0, arg1, arg2, arg3, nullptr};
        core::SNodeC::init(4, logArgs);

        net::in::stream::legacy::SocketClient<TestClientSocketContextFactory, TestState&> socketClient("ipv4-connect-failure-client", testState);

        socketClient.getConfig()->Instance::forceUnrequired();

        const net::in::SocketAddress failingAddress("127.0.0.1", static_cast<std::uint16_t>(0));

        socketClient.connect(failingAddress, [&testState](const net::in::SocketAddress&, core::socket::State connectState) {
            if (connectState != core::socket::State::OK) {
                ++testState.clientConnectFailureCount;
            } else {
                ++testState.unexpectedConnectOkCount;
                ++testState.unexpectedStateCount;
            }

            core::SNodeC::stop();
        });

        const int startResult = core::SNodeC::start(utils::Timeval({1, 0}));

        testResult.expectEqual(0, startResult, "event loop stops successfully after IPv4 connect failure");
        testResult.expectEqual(1, testState.clientConnectFailureCount, "IPv4 legacy client connect callback reports one failure");
        testResult.expectEqual(0, testState.unexpectedConnectOkCount, "IPv4 legacy client connect callback never reports OK");
        testResult.expectEqual(0, testState.clientFactoryCreateCount, "client socket context factory is not called for connect failure");
        testResult.expectEqual(0, testState.clientConnectedCount, "client socket context never reaches onConnected");
        testResult.expectEqual(0, testState.unexpectedStateCount, "connect callback reports no unexpected states");

        core::SNodeC::free();
        logger::Logger::disableLogToFile();

        int attemptStarted = 0;
        int attemptFailed = 0;
        int attemptTimedOut = 0;
        int transportDisconnected = 0;
        std::ifstream input(logPath);
        std::string line;
        while (std::getline(input, line)) {
            if (line.empty()) {
                continue;
            }
            const auto record = nlohmann::json::parse(line);
            const std::string message = record.at("message").get<std::string>();
            if (message == "connection attempt started" || message == "connection attempt failed") {
                testResult.expectTrue(record.at("level") == "debug" && record.at("origin") == "framework" &&
                                          record.at("boundary") == "instance" && record.at("component") == "core.socket.stream" &&
                                          record.at("instance") == "ipv4-connect-failure-client" && record.at("role") == "client" &&
                                          !record.contains("connection"),
                                      message + " carries client endpoint identity at Debug");
            }
            attemptStarted += message == "connection attempt started" ? 1 : 0;
            attemptFailed += message == "connection attempt failed" ? 1 : 0;
            attemptTimedOut += message == "connection attempt timed out" ? 1 : 0;
            transportDisconnected += message == "transport disconnected" ? 1 : 0;
        }

        testResult.expectEqual(1, attemptStarted, "failed connection emits one attempt start");
        testResult.expectEqual(1, attemptFailed, "failed connection emits one terminal failure");
        testResult.expectEqual(0, attemptTimedOut, "failed connection does not also time out");
        testResult.expectEqual(0, transportDisconnected, "failed connection attempt emits no fictional transport disconnect");

        logger::Logger::init();
        logger::LogManager::init();
        result = testResult.processResult();
    }

    return result;
}
