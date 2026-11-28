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

#include "Stdio.h"

#include "Log.h"
#include "Tools.h"
#include "core/SNodeC.h"
#include "core/socket/State.h"
#include "mcp/server/Config.h"
#include "mcp/server/Server.h"
#include "mcp/server/stream/SocketContextFactory.h"
#include "net/config/ConfigConnection.h" // IWYU pragma: keep - directly used configuration base
#include "net/stdio/SocketAddress.h"
#include "net/stdio/stream/SocketEndpoint.h"
#include "net/stdio/stream/config/ConfigSocketEndpoint.h"
#include "utils/CLI11.hpp"
#include "utils/Config.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <exception>
#include <fcntl.h>
#include <functional>
#include <memory>
#include <string>
#include <unistd.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace apps::mcp {

    int runStdio(int argc,
                 char* argv[],
                 ::mcp::server::Server& server,
                 DevelopmentConfig& config,
                 const std::function<void()>& registerApplication) {
        int result = 1;
        int protocolFd = ::fcntl(STDOUT_FILENO, F_DUPFD_CLOEXEC, 3);
        if (protocolFd >= 0 && ::dup2(STDERR_FILENO, STDOUT_FILENO) >= 0) {
            using Endpoint = net::stdio::stream::
                SocketEndpoint<::mcp::server::stream::SocketContextFactory, ::mcp::server::Server&, ::mcp::server::Config&>;
            const Endpoint endpoint("mcp-stdio", server, config);
            endpoint.getConfig()->Instance::forceUnrequired();
            endpoint.getConfig()->Connection::setReadTimeout(0)->setWriteTimeout(0);
            try {
                core::SNodeC::init(argc, argv);
                if (!utils::Config::configRoot.getOption("--daemonize")->as<bool>()) {
                    utils::Config::configRoot.getOption("--daemonize")->check(CLI::IsMember({"false"}));
                    registerApplication();
                    snode::log::application("mcp.development").info() << "SNode.C development stdio ready";
                    endpoint.setOnConnect([&protocolFd](auto*) {
                        protocolFd = -1;
                    });
                    int failure = 0;
                    endpoint.attach(net::stdio::SocketAddress(STDIN_FILENO, protocolFd), [&failure](const auto&, auto state) {
                        if (state != core::socket::State::OK) {
                            failure = state == core::socket::State::DISABLED ? 0 : 1;
                            core::SNodeC::stop();
                        }
                    });
                    const int loop = core::SNodeC::start();
                    result = loop != 0 ? loop : failure;
                } else {
                    core::SNodeC::free();
                }
            } catch (const std::exception& error) {
                snode::log::application("mcp.development").error() << error.what();
                core::SNodeC::free();
            } catch (...) {
                snode::log::application("mcp.development").error() << "MCP stdio startup failed";
                core::SNodeC::free();
            }
        }

        if (protocolFd >= 0) {
            ::close(protocolFd);
        }

        return result;
    }

} // namespace apps::mcp
