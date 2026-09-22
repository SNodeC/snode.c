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
#include "net/in/SocketAddress.h"
#include "net/in/stream/legacy/SocketClient.h"
#include "net/in/stream/legacy/SocketServer.h"
#include "support/TestResult.h"
#include "utils/Timeval.h"

#include <memory>

namespace {
    struct State {
        int created = 0;
        int received = 0;
        int disconnected = 0;
        int clientsDisconnected = 0;
        int completed = 0;
    };

    class Context : public core::socket::stream::SocketContext {
    public:
        Context(core::socket::stream::SocketConnection* connection, State& state, bool client)
            : SocketContext(connection)
            , state(state)
            , client(client) {
            ++state.created;
        }

    private:
        void onConnected() override {
            if (client)
                sendToPeer("x", 1);
        }
        void onDisconnected() override {
            if (client)
                ++state.clientsDisconnected;
            if (++state.disconnected == 4)
                core::SNodeC::stop();
        }
        std::size_t onReceivedFromPeer() override {
            char data;
            const auto size = readFromPeer(&data, 1);
            if (size) {
                ++state.received;
                if (client)
                    close();
                else
                    sendToPeer(&data, size);
            }
            return size;
        }
        bool onSignal(int) override {
            return true;
        }
        State& state;
        bool client;
    };

    class Factory : public core::socket::stream::SocketContextFactory {
    public:
        Factory(State& state, bool client)
            : state(state)
            , client(client) {
        }
        core::socket::stream::SocketContext* create(core::socket::stream::SocketConnection* connection) override {
            return new Context(connection, state, client);
        }

    private:
        State& state;
        bool client;
    };
} // namespace

int main(int argc, char* argv[]) {
    if (tests::support::shouldSkipRootWithoutSNodeCGroup()) {
        tests::support::printRootWithoutSNodeCGroupSkipMessage("InetPerCallFlowTest");
        return tests::support::cTestSkipReturnCode;
    }
    tests::support::TestResult result;
    State state;
    core::SNodeC::init(argc, argv);
    using Server = net::in::stream::legacy::SocketServer<Factory, State&, bool>;
    using Client = net::in::stream::legacy::SocketClient<Factory, State&, bool>;
    int serverDestroyed = 0;
    {
        const Server owner("destroy-facade-server", state, false);
        owner.getConfig()->setPort(0);
        owner.setOnDestroy([&] {
            ++serverDestroyed;
            const Server replacement("destroy-facade-server", state, false);
            replacement.getConfig()->setPort(0);
        });
        {
            const auto copy = owner;
            copy.setOnDestroy([&] {
                ++serverDestroyed;
            });
        }
        result.expectEqual(0, serverDestroyed, "destroying a wrapper copy does not destroy the shared server instance");
    }
    result.expectEqual(2, serverDestroyed, "both facade callbacks run after the shared server name is released");
    int clientDestroyed = 0;
    Server server("per-call-server", state, false);
    server.getConfig()->Instance::forceUnrequired();
    server.getConfig()->setReusePort();
    server.getConfig()->setRetry(false);
    int listens = 0;
    int connects = 0;
    int failures = 0;
    Server::FlowHandle first, second, third;
    std::weak_ptr<core::socket::stream::ClientFlowController> ignoredClient;
    std::weak_ptr<core::socket::stream::ClientFlowController> cancelledClient;
    auto onComplete = [&](std::uint64_t, const std::string&) {
        ++state.completed;
    };
    first = server.listen(net::in::SocketAddress("127.0.0.1", 0), [&](const net::in::SocketAddress& address, core::socket::State status) {
        if (status != core::socket::State::OK) {
            ++failures;
            core::SNodeC::stop();
            return;
        }
        ++listens;
        // Bind a second socket to exactly the same effective address.
        second = server.listen(address, [&](const net::in::SocketAddress& address2, core::socket::State status2) {
            if (status2 != core::socket::State::OK) {
                ++failures;
                core::SNodeC::stop();
                return;
            }
            ++listens;
            first->terminateFlow();
            second->terminateFlow(); // Also exercise termination inside onStatus, before onInitState.
            third = server.listen(address2, [&](const net::in::SocketAddress& address3, core::socket::State status3) {
                if (status3 != core::socket::State::OK) {
                    ++failures;
                    core::SNodeC::stop();
                    return;
                }
                ++listens;
                Client client("per-call-client", state, true);
                client.setOnDestroy([&] {
                    ++clientDestroyed;
                    result.expectEqual(
                        2, state.clientsDisconnected, "client instance destruction follows both client connection lifecycles");
                    const Client replacement("per-call-client", state, true);
                    replacement.getConfig()->Remote::setHost("127.0.0.1")->setPort(0);
                });
                client.getConfig()->Instance::forceUnrequired();
                client.getConfig()->setRetry(false);
                client.getConfig()->setReconnect(false);
                auto onStatus = [&](const net::in::SocketAddress&, core::socket::State connectStatus) {
                    if (connectStatus == core::socket::State::OK)
                        ++connects;
                    else {
                        ++failures;
                        core::SNodeC::stop();
                    }
                };
                auto cancelled = client.connect(address3, onStatus);
                cancelledClient = cancelled;
                cancelled->setOnFlowCompleted(onComplete);
                cancelled->terminateFlow();
                auto active = client.connect(address3, onStatus);
                ignoredClient = active;
                active->setOnFlowCompleted(onComplete);
                client.connect(address3, onStatus);
                // Both the endpoint and returned handles leave scope before the attempts start.
            });
        });
    });
    result.expectEqual(0, core::SNodeC::start(utils::Timeval({1, 0})), "event loop stops successfully");
    result.expectEqual(0, failures, "same-endpoint repeated bind/connect has no errors");
    result.expectEqual(3, listens, "reused-port listeners and a fresh listen after termination succeed");
    result.expectTrue(first != second && second != third && first != third, "each listen has a distinct flow");
    result.expectTrue(first->isTerminated() && second->isTerminated() && !third->isTerminated(),
                      "termination never restarts old flows or terminates a sibling");
    result.expectEqual(2, connects, "only the cancelled connect is suppressed");
    result.expectEqual(4, state.created, "each accepted/connected socket gets its own SocketContext");
    result.expectEqual(4, state.received, "both connections exchange payloads");
    result.expectEqual(4, state.disconnected, "both connections finish their own lifecycle");
    std::weak_ptr<core::socket::stream::ServerFlowController> stoppedFirst = first;
    std::weak_ptr<core::socket::stream::ServerFlowController> stoppedSecond = second;
    first.reset();
    second.reset();
    result.expectTrue(stoppedFirst.expired() && stoppedSecond.expired(),
                      "terminated listeners release their flows before framework shutdown, including late registration");
    core::SNodeC::free();
    result.expectTrue(ignoredClient.expired() && cancelledClient.expired(), "callbacks release flows after runtime cleanup");
    result.expectEqual(2, state.completed, "completion notifications belong to individual flows");
    result.expectEqual(1, clientDestroyed, "runtime retains the shared client instance until its connections end");
    return result.processResult();
}
