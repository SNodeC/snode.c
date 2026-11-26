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

#include "Tools.h"
#include "mcp-config.h"
#include "mcp/server/Server.h"

#ifdef MCP_STDIO
#include "Stdio.h"
#include "utils/Config.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#else
#include "Log.h"
#include "core/SNodeC.h"
#include "express/legacy/in/WebApp.h"
#include "mcp/server/http/Router.h"
#include "web/http/ConfigHttpParser.h"
#include "web/http/server/ConfigHttpServer.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <exception>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#endif

int main(int argc, char* argv[]) {
    int result = 1;
    mcp::server::Server server("snodec-dev", SNODEC_DEV_VERSION, apps::mcp::instructions());
#ifdef MCP_STDIO
    auto* config = utils::Config::configRoot.newSubCommand<apps::mcp::DevelopmentConfig>();
    result = apps::mcp::runStdio(argc, argv, server, *config, [&server, config] {
        apps::mcp::registerDevelopment(server, *config);
    });

#else
    using WebApp = express::legacy::in::WebApp;
    const WebApp app("mcp-http");
    auto* config = app.getConfig()->Instance::newSubCommand<apps::mcp::DevelopmentConfig>();
    auto* http = app.getConfig()->Instance::getSubCommand<web::http::server::ConfigHttpServer>();
    http->setAllowPipelining(false)->setMaximumPendingRequests(1);
    http->getParserConfig()->setMaximumBodyBytes(1048576)->setMaximumHeaderBytes(65536)->setMaximumHeaderFields(100);
    app.getConfig()->Connection::setMaximumWriteQueueBytes(1048576);
    app.getConfig()->Local::setHost("127.0.0.1")->setPort(3337);
    int listenFailure = 0;
    try {
        WebApp::init(argc, argv);
        if (app.getConfig()->Instance::getDisabled()) {
            core::SNodeC::free();
            result = 0;
        } else {
            apps::mcp::registerDevelopment(server, *config);
            app.use("/mcp", mcp::server::http::Router(server, *config));
            app.listen([&listenFailure](const auto& address, const auto& state) {
                if (state == core::socket::State::OK) {
                    snode::log::application("mcp.development").info() << "SNode.C development HTTP listening on " << address.toString();
                } else {
                    listenFailure = state == core::socket::State::DISABLED ? 0 : 1;
                    snode::log::application("mcp.development").error() << "HTTP listener unavailable: " << state.what();
                    core::SNodeC::stop();
                }
            });
            if (core::SNodeC::state() == core::State::STOPPING) {
                core::SNodeC::free();
                result = listenFailure;
            } else {
                const int loopResult = WebApp::start();
                result = listenFailure != 0 ? listenFailure : loopResult;
            }
        }
    } catch (const std::exception& error) {
        snode::log::application("mcp.development").error() << error.what();
        core::SNodeC::free();
    }
#endif

    return result;
}
