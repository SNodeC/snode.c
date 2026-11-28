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
#include "core/socket/stream/SocketContext.h"
#include "core/socket/stream/SocketContextFactory.h"
#include "core/socket/stream/stdio/DescriptorOwnership.h"
#include "core/socket/stream/stdio/SocketEndpoint.h"
#include "net/in/stream/legacy/SocketClient.h"
#include "net/in/stream/legacy/SocketServer.h"
#include "net/stdio/SocketAddress.h"
#include "net/stdio/config/ConfigAddress.h"
#include "net/stdio/phy/stream/PhysicalSocket.h"
#include "net/stdio/stream/SocketEndpoint.h"
#include "net/stdio/stream/config/ConfigSocketEndpoint.h"
#include "support/TestResult.h"

#include <cstddef>
#include <optional>

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <fcntl.h>
#include <map>
#include <stdexcept>
#include <unistd.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace {

    tests::support::TestResult test;
    std::string mode;
    std::string order;
    std::map<core::socket::stream::SocketConnection*, std::unique_ptr<int>> connectionStates;
    int statuses = 0;
    int disconnected = 0;
    int factories = 0;
    int protocols = 0;

    class Context final : public core::socket::stream::SocketContext {
    public:
        explicit Context(core::socket::stream::SocketConnection* connection)
            : SocketContext(connection) {
        }

    private:
        void onConnected() override {
            ++protocols;
            order += "p";
            close();
        }

        void onDisconnected() override {
        }

        std::size_t onReceivedFromPeer() override {
            char bytes[16];

            return readFromPeer(bytes, sizeof(bytes));
        }

        bool onSignal([[maybe_unused]] int signal) override {
            return true;
        }
    };

    class Factory final : public core::socket::stream::SocketContextFactory {
    public:
        ~Factory() override;

        core::socket::stream::SocketContext* create(core::socket::stream::SocketConnection* connection) override {
            ++factories;
            order += "f";
            Context* context = nullptr;
            if (mode == "throw") {
                throw std::runtime_error("test factory");
            } else if (mode != "null") {
                context = new Context(connection);
            }

            return context;
        }
    };

    Factory::~Factory() = default;

    class FailingPhysical : public net::stdio::phy::stream::PhysicalSocket {
    public:
        using PhysicalSocket::PhysicalSocket;

        int getReadFd() const {
            return mode == "first" ? -1 : PhysicalSocket::getReadFd();
        }

        int getWriteFd() const {
            return -1;
        }
    };

    using Endpoint = net::stdio::stream::SocketEndpoint<Factory>;

} // namespace

int main(int argc, char* argv[]) {
    mode = argc > 1 ? argv[1] : "success";
    char file[] = "--config-file=/dev/null";
    char quiet[] = "--quiet";
    char* arguments[] = {argv[0], file, quiet};
    if (mode == "borrow") {
        const int saved = ::fcntl(STDIN_FILENO, F_DUPFD_CLOEXEC, 3);
        int descriptors[4] = {-1, -1, -1, -1};
        const bool pipes = ::pipe(descriptors) == 0 && ::pipe(descriptors + 2) == 0;
        test.expectTrue(saved >= 0 && pipes, "create borrowed stdin and output pipes");
        if (saved >= 0 && pipes && ::dup2(descriptors[0], STDIN_FILENO) >= 0) {
            const int inputFlags = ::fcntl(STDIN_FILENO, F_GETFL);
            const int outputFlags = ::fcntl(descriptors[3], F_GETFL);
            test.expectEqual(0, ::fcntl(STDIN_FILENO, F_SETFL, inputFlags | O_NONBLOCK | O_APPEND), "prepare borrowed input flags");
            test.expectEqual(0, ::fcntl(descriptors[3], F_SETFL, outputFlags | O_NONBLOCK), "prepare borrowed output flags");
            const int expectedFlags = ::fcntl(STDIN_FILENO, F_GETFL);
            const std::string payload = "borrowed stdin survives cleanup";
            test.expectEqual(static_cast<ssize_t>(payload.size()),
                             ::write(descriptors[1], payload.data(), payload.size()),
                             "queue data owned by the stdin borrower");
            const Endpoint endpoint("borrowed-stdin");
            endpoint.getConfig()->Instance::forceUnrequired();
            endpoint.getConfig()->Local::setDescriptorOwnership(net::stdio::config::DescriptorOwnership::BORROW);
            core::SNodeC::init(3, arguments);
            endpoint.attach(net::stdio::SocketAddress(STDIN_FILENO, descriptors[3]), [](const auto&, auto state) {
                ++statuses;
                test.expectTrue(state == core::socket::State::OK, "borrowed stdin attaches successfully");
            });
            test.expectEqual(0, core::SNodeC::start(), "borrowed connection closes without consuming input");
            test.expectEqual(1, statuses, "borrowed connection reports one attachment");
            test.expectEqual(1, protocols, "borrowed context was attached and closed");
            test.expectEqual(expectedFlags, ::fcntl(STDIN_FILENO, F_GETFL), "global cleanup preserves borrowed stdin flags");
            char received[64]{};
            const auto count = ::read(STDIN_FILENO, received, sizeof(received));
            test.expectEqual(static_cast<ssize_t>(payload.size()), count, "global cleanup preserves unread pipe bytes");
            test.expectTrue(payload == std::string(received, count > 0 ? static_cast<std::size_t>(count) : 0),
                            "stdin borrower receives its exact unread payload after teardown");
        }

        if (saved >= 0) {
            test.expectTrue(::dup2(saved, STDIN_FILENO) >= 0, "restore the test process stdin");
            ::close(saved);
        }

        for (int descriptor : descriptors) {
            if (descriptor >= 0) {
                ::close(descriptor);
            }
        }
    } else if (mode == "network") {
        net::in::stream::legacy::SocketServer<Factory> server("server");
        const net::in::stream::legacy::SocketClient<Factory> client("client");
        server.getConfig()->Instance::forceUnrequired();
        client.getConfig()->Instance::forceUnrequired();
        server.setOnDisconnect([](auto*) {
            if (++disconnected == 2) {
                core::SNodeC::stop();
            }
        });
        client.setOnDisconnect([](auto*) {
            if (++disconnected == 2) {
                core::SNodeC::stop();
            }
        });
        core::SNodeC::init(3, arguments);
        server.listen(net::in::SocketAddress("127.0.0.1", 0), [&](const auto& address, auto state) {
            test.expectTrue(state == core::socket::State::OK, "network listener");
            client.connect(address, [](const auto&, auto connected) {
                test.expectTrue(connected == core::socket::State::OK, "network connection");
            });
        });
        test.expectEqual(0, core::SNodeC::start(), "network loop");
        test.expectTrue(factories == 2 && protocols == 2 && disconnected == 2, "same original context/factory over network");
    } else {
        int handles[2]{};
        test.expectEqual(0, ::socketpair(AF_UNIX, SOCK_STREAM, 0, handles), "duplex binding");
        const int flags = ::fcntl(handles[0], F_GETFL);
        {
            const Endpoint endpoint("endpoint");
            endpoint.getConfig()->Instance::forceUnrequired();
            test.expectTrue(!endpoint.getConfig()->getLogRole(), "endpoint has no logging role");
            endpoint.getConfig()->Local::setSocketAddress(net::stdio::SocketAddress(handles[0], handles[0]));
            endpoint.getConfig()->Instance::setDisabled(mode == "disabled");
            endpoint.setOnConnect([](auto* connection) {
                connectionStates.emplace(connection, std::make_unique<int>(1));
                order += "c";
                if (mode == "halt") {
                    core::SNodeC::stop();
                }
            });
            endpoint.setOnConnected([](auto* connection) {
                order += "C";
                if (mode == "close") {
                    connection->close();
                }
            });
            endpoint.setOnDisconnect([](auto* connection) {
                test.expectTrue(connectionStates.erase(connection) == 1, "disconnect releases exactly the state allocated by onConnect");
                ++disconnected;
                order += "d";
                core::SNodeC::stop();
            });
            core::SNodeC::init(3, arguments);
            const auto status = [&](const auto&, auto state) {
                ++statuses;
                order += "s";
                const bool success = mode == "success" || mode == "destroy" || mode == "stop" || mode == "retry" || mode == "first";
                test.expectTrue(state == (success              ? core::socket::State::OK
                                          : mode == "disabled" ? core::socket::State::DISABLED
                                                               : core::socket::State::ERROR),
                                "one appropriate attachment status");
                if (!success || mode == "stop") {
                    core::SNodeC::stop();
                }
            };
            if (mode == "stopping") {
                core::EventReceiver::atNextTick([] {
                    core::SNodeC::stop();
                });
            }

            if (mode == "retry" || mode == "first") {
                using FailingEndpoint = core::socket::stream::stdio::SocketEndpoint<FailingPhysical, Endpoint::Config, Factory>;
                const FailingEndpoint failure("failure");
                failure.getConfig()->Instance::forceUnrequired();
                failure.attach(net::stdio::SocketAddress(handles[0], handles[0]), [&](const auto&, auto state) {
                    ++statuses;
                    order += "r";
                    test.expectTrue(state == core::socket::State::ERROR && disconnected == 0,
                                    "activation failure has no disconnect callback");
                    test.expectTrue(((::fcntl(handles[0], F_GETFL) & O_NONBLOCK) != 0) == (mode == "retry"),
                                    "first failure rolls back before reporting; second reports before deferred rollback");
                    endpoint.attach(status);
                });
            } else if (mode == "invalid") {
                endpoint.attach(net::stdio::SocketAddress(-1, -1), status);
            } else {
                endpoint.attach(status);
            }

            test.expectTrue(statuses == 0 && ::fcntl(handles[0], F_GETFL) == flags, "attachment is queued without touching flags");
            if (mode == "lifecycle") {
                static_cast<void>(core::SNodeC::tick());
                core::SNodeC::free();
            } else if (mode != "destroy") {
                test.expectEqual(0, core::SNodeC::start(), "endpoint loop");
            }
        }
        if (mode == "destroy") {
            test.expectEqual(0, core::SNodeC::start(), "queued state outlives facade");
        }

        if (mode == "halt") {
            test.expectEqual(0, factories, "shutdown from onConnect prevents protocol construction");
        }

        test.expectEqual(mode == "retry" || mode == "first" ? 2 : 1, statuses, "exactly one completion per attempt");
        if (mode == "success" || mode == "destroy" || mode == "stop" || mode == "retry" || mode == "first") {
            test.expectTrue(order == (mode == "retry" || mode == "first" ? "rcCfpsd" : "cCfpsd") && protocols == 1 && disconnected == 1,
                            "activation/callback/factory/status/release order");
        } else if (mode == "null" || mode == "throw" || mode == "close" || mode == "halt") {
            test.expectEqual(1, disconnected, "every onConnect is paired with onDisconnect, including establishment failures");
        } else {
            test.expectEqual(0, disconnected, "no application disconnect without onConnect");
        }

        test.expectTrue(connectionStates.empty(), "all per-connection state is released after teardown");
        if (mode == "disabled" || mode == "invalid" || (mode == "lifecycle" || mode == "stopping")) {
            test.expectTrue(::fcntl(handles[0], F_GETFL) == flags && factories == 0, "rejected attachment leaves caller binding untouched");
            ::close(handles[0]);
        }

        ::close(handles[1]);
    }

    return test.processResult();
}
