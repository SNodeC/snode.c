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

#include "Features.h"
#include "core/timer/Timer.h"
#include "express/Request.h"
#include "mcp/server/Config.h"
#include "mcp/server/Server.h"
#include "mcp/server/http/Router.h"
#include "web/http/ConfigHttpParser.h"
#include "web/http/server/ConfigHttpServer.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>
#include <stdexcept>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#ifdef MCP_TLS
#include "express/tls/in/WebApp.h"
using WebApp = express::tls::in::WebApp;
#else
#include "express/legacy/in/WebApp.h"
using WebApp = express::legacy::in::WebApp;
#endif
using mcp::server::ToolResult;

int main(int argc, char* argv[]) {
    const auto objectSchema = nlohmann::ordered_json::parse(R"json(
{
    "type": "object"
}
)json");
    const WebApp app("mcp-http");
    auto* config = app.getConfig()->Instance::newSubCommand<mcp::server::Config>();
    auto* http = app.getConfig()->Instance::getSubCommand<web::http::server::ConfigHttpServer>();
    http->setAllowPipelining(false)->setMaximumPendingRequests(1);
    http->getParserConfig()->setMaximumBodyBytes(1048576)->setMaximumHeaderBytes(65536)->setMaximumHeaderFields(100);
    app.getConfig()->Connection::setMaximumWriteQueueBytes(1048576);
    app.getConfig()->Local::setHost("127.0.0.1")->setPort(0);
    WebApp::init(argc, argv);
    mcp::server::Server server("http-fixture", "test-version");
    for (const std::string name : {"echo", "écho", "=?base64?literal?="}) {
        server.tool({name, {}, objectSchema}, [http](const auto& call, const auto& reply) {
            const auto& args = call.arguments;
            static_cast<void>(args);
            reply->result(ToolResult::json({{"pipelining", http->getAllowPipelining()}, {"pending", http->getMaximumPendingRequests()}}));
        });
    }

    server.tool({"delay", {}, objectSchema}, [](const auto&, const auto& reply) {
        core::timer::Timer::singleshotTimer(
            [reply] {
                reply->result(ToolResult::content({}));
            },
            utils::Timeval(0.1));
    });
    server.tool({"large", {}, objectSchema}, [](const auto&, const auto& reply) {
        reply->result(ToolResult::text(std::string(200000, 'x')));
    });
    server.tool({"overflow", {}, objectSchema}, [](const auto&, const auto& reply) {
        reply->result(ToolResult::text(std::string(1048576, 'x')));
    });
    server.tool({"identity", {}, objectSchema}, [](const auto& call, const auto& reply) {
        const auto& principal = call.context.getPrincipal();
        reply->result(ToolResult::json(principal ? mcp::server::Json{{"subject", principal->subject}, {"claims", principal->claims}}
                                                 : mcp::server::Json{{"anonymous", true}}));
    });
    tests::mcp::registerFeatures(server);
    mcp::server::http::RouterOptions routerOptions;
    routerOptions.protectedResource =
        mcp::server::http::ProtectedResource{"https://example.com/mcp", {"https://auth.example.com"}, {"test"}};
    const mcp::server::http::Router router(
        server,
        *config,
        [](const auto& request) -> mcp::server::http::Authorization {
            using namespace mcp::server::http;
            const std::string credential = request->get("Authorization");
            Authorization result = Unauthorized{"Bearer realm=\"fixture\""};
            if (credential == "Bearer fixture") {
                result = Authorized{mcp::server::Principal{"fixture-user", {{"scope", "test"}}}};
            } else if (credential == "Bearer anonymous") {
                result = Authorized{};
            } else if (credential == "Bearer forbidden") {
                result = Forbidden{};
            } else if (credential == "Bearer insufficient") {
                result = InsufficientScope{"test"};
            } else if (credential == "Bearer malformed") {
                result = Unauthorized{"invalid\r\nchallenge"};
            } else if (credential == "Bearer throw") {
                throw std::runtime_error("authorization failed");
            }

            return result;
        },
        routerOptions);
    router.mountMetadata(app);
    app.use("/mcp", router);
    app.listen([](const auto&, const auto&) {
    });

    return WebApp::start();
}
