/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
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

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef>
#include <cstdint>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace {

    struct TestState {
        int serverListenOkCount = 0;
        int unexpectedStateCount = 0;
        std::uint16_t effectivePort = 0;
    };

    class TestServerSocketContext : public core::socket::stream::SocketContext {
    public:
        explicit TestServerSocketContext(core::socket::stream::SocketConnection* socketConnection)
            : core::socket::stream::SocketContext(socketConnection) {
        }

    private:
        void onConnected() override {
        }

        void onDisconnected() override {
        }

        std::size_t onReceivedFromPeer() override {
            return 0;
        }

        bool onSignal([[maybe_unused]] int signum) override {
            return true;
        }
    };

    class TestServerSocketContextFactory : public core::socket::stream::SocketContextFactory {
    public:
        core::socket::stream::SocketContext* create(core::socket::stream::SocketConnection* socketConnection) override {
            return new TestServerSocketContext(socketConnection);
        }
    };

} // namespace

int main(int argc, char* argv[]) {
    tests::support::TestResult testResult;
    int result = tests::support::cTestSkipReturnCode;

    if (tests::support::shouldSkipRootWithoutSNodeCGroup()) {
        tests::support::printRootWithoutSNodeCGroupSkipMessage("InetLegacyServerEffectiveListenAddressTest");
    } else {
        TestState testState;

        core::SNodeC::init(argc, argv);

        const net::in::stream::legacy::SocketServer<TestServerSocketContextFactory> socketServer("effective-listen-address-server");
        socketServer.getConfig()->Instance::forceUnrequired();

        socketServer.listen(net::in::SocketAddress("127.0.0.1", 0), [&testState](const net::in::SocketAddress& socketAddress,
                                                                                  core::socket::State state) {
            if (state == core::socket::State::OK) {
                ++testState.serverListenOkCount;
                testState.effectivePort = socketAddress.getPort();
            } else {
                ++testState.unexpectedStateCount;
            }

            core::SNodeC::stop();
        });

        const int startResult = core::SNodeC::start(utils::Timeval({1, 0}));

        testResult.expectEqual(0, startResult, "event loop stops successfully after listen callback");
        testResult.expectEqual(1, testState.serverListenOkCount, "IPv4 legacy server listen callback reports OK exactly once");
        testResult.expectTrue(testState.effectivePort != 0, "IPv4 legacy server listen callback reports an effective non-zero port");
        testResult.expectEqual(0, testState.unexpectedStateCount, "listen callback reports no unexpected states");

        result = testResult.processResult();

        core::SNodeC::free();
    }

    return result;
}
