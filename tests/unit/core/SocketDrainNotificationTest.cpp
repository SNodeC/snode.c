/* SPDX-License-Identifier: LGPL-3.0-or-later OR MIT */

#include "core/socket/stream/SocketConnection.h"
#include "core/socket/stream/SocketContext.h"
#include "core/socket/stream/SocketContextFactory.h"
#include "net/in/stream/legacy/SocketServer.h"
#include "tests/support/NetworkTest.h"

#include <array>
#include <cstddef>
#include <string>
#include <vector>

namespace {

    struct State {
        int initialDrains = 0;
        int activeDrains = 0;
        int activeDisconnected = 0;
        bool firstWritable = true;
        bool secondWritable = true;
        std::vector<std::size_t> sentAtDrain;
    };

    class Context final : public core::socket::stream::SocketContext {
    public:
        Context(core::socket::stream::SocketConnection* connection, State& state, bool replacement = false)
            : SocketContext(connection)
            , state(state)
            , replacement(replacement) {
        }

    private:
        void onConnected() override {
        }

        void onDisconnected() override {
            if (replacement) {
                ++state.activeDisconnected;
            }
        }

        std::size_t onReceivedFromPeer() override {
            std::array<char, 64> input{};
            const std::size_t received = readFromPeer(input.data(), input.size());
            if (!replacement && received != 0) {
                state.firstWritable = sendToPeer("12345678");
                getSocketConnection()->setSocketContext(new Context(getSocketConnection(), state, true));
            }
            return received;
        }

        void onDrain() override {
            if (replacement) {
                ++state.activeDrains;
                state.sentAtDrain.push_back(getTotalSent());
                if (state.activeDrains == 1) {
                    state.secondWritable = sendToPeer("abcdefgh");
                } else {
                    close();
                }
            } else {
                ++state.initialDrains;
            }
        }

        bool onSignal(int) override {
            return true;
        }

        State& state;
        bool replacement;
    };

    class Factory final : public core::socket::stream::SocketContextFactory {
    public:
        explicit Factory(State& state)
            : state(state) {
        }

        core::socket::stream::SocketContext* create(core::socket::stream::SocketConnection* connection) override {
            return new Context(connection, state);
        }

    private:
        State& state;
    };

} // namespace

int main() {
    tests::support::TestResult result;
    tests::support::Runtime runtime;
    State state;
    tests::support::Peer peer;
    std::string wire;
    bool eof = false;
    net::in::stream::legacy::SocketServer<Factory, State&> server("socket-drain", state);
    server.getConfig()->Instance::forceUnrequired();
    server.getConfig()->Connection::setWriteBlockSize(4)->setWriteQueueHighWatermark(6)->setWriteQueueLowWatermark(2);
    server.listen(net::in::SocketAddress("127.0.0.1", 0), [&](const auto& address, auto status) {
        tests::support::require(status == core::socket::State::OK, "listen");
        peer.connect(address.getPort());
        tests::support::require(peer.write("switch") == 6, "request context replacement");
    });
    result.expectTrue(tests::support::runUntil(
                          [&]() {
                              return eof && state.activeDisconnected == 1;
                          },
                          [&]() {
                              if (peer.fd >= 0) {
                                  std::array<char, 64> bytes{};
                                  const int received = peer.read(bytes.data(), bytes.size());
                                  if (received > 0) {
                                      wire.append(bytes.data(), static_cast<std::size_t>(received));
                                  } else if (received == 0) {
                                      eof = true;
                                      peer.close();
                                  }
                              }
                          }),
                      "drain callback sends subsequent data and closes through a real socket");
    result.expectEqual(0, state.initialDrains, "the replaced context receives no drain notification");
    result.expectEqual(2, state.activeDrains, "the active context receives one notification per pressure cycle");
    result.expectTrue(!state.firstWritable && !state.secondWritable, "both complete chunks report queue pressure");
    result.expectTrue(state.sentAtDrain == std::vector<std::size_t>{8, 16}, "notifications follow actual socket draining");
    result.expectTrue(wire == "12345678abcdefgh", "the peer receives both complete chunks once and in order");
    return result.processResult();
}
