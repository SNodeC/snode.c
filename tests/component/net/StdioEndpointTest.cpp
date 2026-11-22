// SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
#include "core/EventReceiver.h"
#include "core/SNodeC.h"
#include "core/socket/stream/SocketContext.h"
#include "core/socket/stream/SocketContextFactory.h"
#include "net/in/stream/legacy/SocketClient.h"
#include "net/in/stream/legacy/SocketServer.h"
#include "net/stdio/stream/SocketEndpoint.h"
#include "support/TestResult.h"

#include <fcntl.h>
#include <memory>
#include <stdexcept>
#include <string>
#include <unistd.h>

namespace {
    tests::support::TestResult test;
    std::string mode, order;
    int statuses = 0, disconnected = 0, factories = 0, protocols = 0;
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
    char file[] = "--config-file=/dev/null", quiet[] = "--quiet";
    char* arguments[] = {argv[0], file, quiet};
    if (mode == "network") {
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
            endpoint.setOnConnect([](auto*) {
                order += "c";
            });
            endpoint.setOnConnected([](auto* connection) {
                order += "C";
                if (mode == "close") {
                    connection->close();
                }
            });
            endpoint.setOnDisconnect([](auto*) {
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
        test.expectEqual(mode == "retry" || mode == "first" ? 2 : 1, statuses, "exactly one completion per attempt");
        if (mode == "success" || mode == "destroy" || mode == "stop" || mode == "retry" || mode == "first") {
            test.expectTrue(order == (mode == "retry" || mode == "first" ? "rcCfpsd" : "cCfpsd") && protocols == 1 && disconnected == 1,
                            "activation/callback/factory/status/release order");
        } else {
            test.expectEqual(0, disconnected, "no application disconnect before establishment");
        }
        if (mode == "disabled" || mode == "invalid" || (mode == "lifecycle" || mode == "stopping")) {
            test.expectTrue(::fcntl(handles[0], F_GETFL) == flags && factories == 0, "rejected attachment leaves caller binding untouched");
            ::close(handles[0]);
        }
        ::close(handles[1]);
    }
    return test.processResult();
}
