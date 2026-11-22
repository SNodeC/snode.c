// SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
#include "Log.h"
#include "core/SNodeC.h"
#include "core/timer/Timer.h"
#include "mcp/server/Config.h"
#include "mcp/server/Server.h"
#include "mcp/server/stream/SocketContextFactory.h"
#include "net/stdio/stream/SocketEndpoint.h"
#include "utils/Config.h"

#include <exception>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>
#include <stdexcept>
#include <sys/stat.h>
#include <unistd.h>
using mcp::server::ToolResult;
int main(int argc, char* argv[]) {
    const auto objectSchema = nlohmann::ordered_json::parse(R"json(
{
    "type": "object"
}
)json");
    auto* config = utils::Config::configRoot.newSubCommand<mcp::server::Config>();
    config->addOption("--fixture-schema",
                      "Test registration failure",
                      "kind",
                      std::string("valid"),
                      CLI::IsMember({"valid", "invalid", "throw", "throw-int", "stop"}));
    config->addOption("--fixture-reparse-config", "Bootstrap reparse fixture", "path", std::string(""), CLI::Validator());
    mcp::server::Server server("stdio-fixture", "test-version");
    int result = 1;
    int protocolFd = ::fcntl(STDOUT_FILENO, F_DUPFD_CLOEXEC, 3);
    struct stat protocol{}, diagnostics{};
    const bool shared = protocolFd >= 0 && ::fstat(protocolFd, &protocol) == 0 && ::fstat(STDERR_FILENO, &diagnostics) == 0 &&
                        protocol.st_dev == diagnostics.st_dev && protocol.st_ino == diagnostics.st_ino;
    if (protocolFd >= 0 && ::dup2(STDERR_FILENO, STDOUT_FILENO) >= 0) {
        using Endpoint =
            net::stdio::stream::SocketEndpoint<mcp::server::stream::SocketContextFactory, mcp::server::Server&, mcp::server::Config&>;
        const Endpoint endpoint("mcp-stdio", server, *config);
        endpoint.getConfig()->Instance::forceUnrequired();
        endpoint.getConfig()->Connection::setReadTimeout(0)->setWriteTimeout(0)->setMaximumWriteQueueBytes(1);
        try {
            core::SNodeC::init(argc, argv);
            if (!utils::Config::configRoot.getOption("--daemonize")->as<bool>() &&
                (!shared || utils::Config::configRoot.getOption("--quiet")->as<bool>())) {
                utils::Config::configRoot.getOption("--daemonize")->check(CLI::IsMember({"false"}));
                server.tool({"echo", {}, objectSchema}, [](const auto& call, const auto& reply) {
                    const auto& args = call.arguments;
                    reply->result(ToolResult::text(args.dump()));
                });
                server.tool({"delay", {}, objectSchema}, [config](const auto&, const auto& reply) {
                    core::timer::Timer::singleshotTimer(
                        [reply, config] {
                            if (config->getOption("--fixture-schema")->as<std::string>() == "stop") {
                                core::SNodeC::stop();
                            } else {
                                reply->result(ToolResult::content({}));
                            }
                        },
                        utils::Timeval(0.2));
                });
                if (config->getOption("--fixture-schema")->as<std::string>() == "invalid") {
                    server.tool({"bad", {}, nlohmann::ordered_json::parse(R"json(
{
    "type": "object",
    "$ref": "unsupported"
}
)json")},
                                [](const auto&, const auto&) {
                                });
                }
                if (config->getOption("--fixture-schema")->as<std::string>() == "throw") {
                    throw std::runtime_error("fixture ready failed");
                }
                if (config->getOption("--fixture-schema")->as<std::string>() == "throw-int") {
                    throw 42;
                }
                snode::log::application("mcp.fixture").info() << "MCP fixture ready";
                std::cout << "fixture direct stdout diagnostic" << std::endl;
                const auto path = config->getOption("--fixture-reparse-config")->as<std::string>();
                if (!path.empty()) {
                    std::ofstream changed(path);
                    changed << "daemonize=true\n";
                }
                endpoint.setOnConnect([&protocolFd](auto*) {
                    protocolFd = -1;
                });
                int failure = 0;
                endpoint.attach(net::stdio::SocketAddress(STDIN_FILENO, protocolFd), [&failure](const auto&, auto state) {
                    if (state != core::socket::State::OK) {
                        failure = 1;
                        core::SNodeC::stop();
                    }
                });
                const int loop = core::SNodeC::start();
                result = loop != 0 ? loop : failure;
            }
        } catch (const std::exception& error) {
            snode::log::application("mcp.fixture").error() << error.what();
            core::SNodeC::free();
        } catch (...) {
            snode::log::application("mcp.fixture").error() << "MCP fixture failed";
            core::SNodeC::free();
        }
    }
    if (protocolFd >= 0) {
        ::close(protocolFd);
    }
    return result;
}
