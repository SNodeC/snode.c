// SPDX-License-Identifier: MIT OR LGPL-3.0-or-later
#include "HttpServerClientRoundTripTest.h"
#include "core/timer/Timer.h"
#include "net/in/SocketAddress.h"
#include "web/http/legacy/in/Client.h"
#include "web/http/legacy/in/Server.h"

#include <array>
#include <barrier>
#include <memory>
#include <mutex>
#include <thread>

int main(int argc, char** argv) {
    if (tests::support::shouldSkipRootWithoutSNodeCGroup()) {
        tests::support::printRootWithoutSNodeCGroupSkipMessage("ThreadedHttpEventLoopsTest");
        return tests::support::cTestSkipReturnCode;
    }
    tests::support::TestResult result;
    core::SNodeC::init(argc, argv);
    std::mutex configuration;
    std::barrier prepared(3);
    std::array<tests::component::http::RoundTripState, 2> states;
    std::array<int, 2> exits{};
    std::array<bool, 2> local{true, true};
    std::array<std::thread, 2> workers;
    for (int i = 0; i < 2; ++i) {
        workers[i] = std::thread([&, i] {
            core::SNodeC::init(argc, argv);
            const auto owner = std::this_thread::get_id();
            auto& state = states[i];
            using Server = web::http::legacy::in::Server;
            using Client = web::http::legacy::in::Client;
            std::unique_ptr<Server> server;
            std::unique_ptr<Client> client;
            {
                // Configuration is process-wide; event receivers are constructed in their owning thread.
                const std::lock_guard lock(configuration);
                server =
                    std::make_unique<Server>("threaded-http-server-" + std::to_string(i), [&](const auto& request, const auto& response) {
                        local[i] = local[i] && std::this_thread::get_id() == owner;
                        tests::component::http::handleRequest(request, response, state);
                    });
                client = std::make_unique<Client>(
                    "threaded-http-client-" + std::to_string(i),
                    [&](const auto& request) {
                        local[i] = local[i] && std::this_thread::get_id() == owner;
                        tests::component::http::sendRequest(request, state);
                    },
                    [&](const auto&) {
                        ++state.httpDisconnectedCount;
                    });
                tests::component::http::configureHttpRoundTrip(*server, *client, state);
                server->listen(net::in::SocketAddress("127.0.0.1", 0), [&](const auto& address, core::socket::State listenState) {
                    if (listenState == core::socket::State::OK && address.getPort() != 0) {
                        ++state.listenOkCount;
                        ++state.effectiveListenEndpointOkCount;
                        client->connect(net::in::SocketAddress("127.0.0.1", address.getPort()),
                                        [&](const auto&, core::socket::State connectState) {
                                            if (connectState == core::socket::State::OK) {
                                                ++state.clientConnectOkCount;
                                            } else {
                                                ++state.unexpectedStateCount;
                                                core::SNodeC::stop();
                                            }
                                        });
                    } else {
                        ++state.unexpectedStateCount;
                        core::SNodeC::stop();
                    }
                });
            }
            prepared.arrive_and_wait();
            auto deadline = core::timer::Timer::singleshotTimer(
                [&] {
                    ++state.unexpectedStateCount;
                    core::SNodeC::stop();
                },
                utils::Timeval(2));
            exits[i] = core::SNodeC::start();
            core::SNodeC::free();
            const std::lock_guard lock(configuration);
            client.reset();
            server.reset();
        });
    }
    prepared.arrive_and_wait();
    for (auto& worker : workers) {
        worker.join();
    }
    // Keep process configuration alive until both workers and their wrappers are gone.
    core::SNodeC::free();
    for (int i = 0; i < 2; ++i) {
        tests::component::http::expectRoundTrip(result, states[i], "threaded IPv4 HTTP " + std::to_string(i), exits[i]);
        result.expectTrue(local[i], "HTTP callbacks execute in their owning loop");
    }
    return result.processResult();
}
