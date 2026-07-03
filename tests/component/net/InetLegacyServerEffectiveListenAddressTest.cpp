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

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef>
#include <cstdint>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace {

    struct TestState {
        int listenOkCount = 0;
        int unexpectedStateCount = 0;
        std::uint16_t listenPort = 0;
    };

    class TestSocketContext : public core::socket::stream::SocketContext {
    public:
        explicit TestSocketContext(core::socket::stream::SocketConnection* socketConnection)
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

    class TestSocketContextFactory : public core::socket::stream::SocketContextFactory {
    public:
        explicit TestSocketContextFactory(TestState&) {
        }

        core::socket::stream::SocketContext* create(core::socket::stream::SocketConnection* socketConnection) override {
            return new TestSocketContext(socketConnection);
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

        const net::in::stream::legacy::SocketServer<TestSocketContextFactory, TestState&> socketServer("effective-listen-server",
                                                                                                       testState);
        socketServer.getConfig()->Instance::forceUnrequired();

        socketServer.listen(net::in::SocketAddress("127.0.0.1", 0),
                            [&testState](const net::in::SocketAddress& socketAddress, core::socket::State state) {
                                if (state == core::socket::State::OK) {
                                    ++testState.listenOkCount;
                                    testState.listenPort = socketAddress.getPort();
                                } else {
                                    ++testState.unexpectedStateCount;
                                }

                                core::SNodeC::stop();
                            });

        const int startResult = core::SNodeC::start(utils::Timeval({1, 0}));

        testResult.expectEqual(0, startResult, "event loop stops after listen status callback");
        testResult.expectEqual(1, testState.listenOkCount, "IPv4 legacy server listen callback reports OK exactly once");
        testResult.expectTrue(testState.listenPort != 0, "IPv4 legacy server listen callback reports kernel-selected port");
        testResult.expectEqual(0, testState.unexpectedStateCount, "listen callback reports no unexpected states");

        result = testResult.processResult();

        core::SNodeC::free();
    }

    return result;
}
