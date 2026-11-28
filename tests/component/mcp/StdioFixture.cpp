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

// IWYU pragma: no_include "core/socket/Socket.hpp"
// IWYU pragma: no_include "core/socket/stream/SocketConnection.hpp"

#include "Features.h"
#include "Log.h"
#include "core/SNodeC.h"
#include "core/socket/State.h"
#include "core/timer/Timer.h"
#include "mcp/server/Config.h"
#include "mcp/server/Reply.h"
#include "mcp/server/Results.h"
#include "mcp/server/Server.h"
#include "mcp/server/stream/SocketContextFactory.h"
#include "net/config/ConfigConnection.h"
#include "net/config/ConfigSection.h"
#include "net/stdio/SocketAddress.h"
#include "net/stdio/stream/SocketEndpoint.h"
#include "net/stdio/stream/config/ConfigSocketEndpoint.h"
#include "utils/Config.h"

#include <map>

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <exception>
#include <fcntl.h>
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>
#include <stdexcept>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

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
    struct stat protocol{};
    struct stat diagnostics{};
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

                tests::mcp::registerFeatures(server);
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
            } else {
                core::SNodeC::free();
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
