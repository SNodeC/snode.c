// SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
#include "core/timer/Timer.h"
#include "express/Request.h"
#include "mcp/server/Config.h"
#include "mcp/server/Server.h"
#include "mcp/server/http/Router.h"
#include <stdexcept>
#include "web/http/ConfigHttpParser.h"
#include "web/http/server/ConfigHttpServer.h"
#ifdef MCP_TLS
#include "express/tls/in/WebApp.h"
using WebApp = express::tls::in::WebApp;
#else
#include "express/legacy/in/WebApp.h"
using WebApp = express::legacy::in::WebApp;
#endif
using mcp::server::ToolResult;
int main(int argc, char* argv[]) {
    const auto objectSchema = mcp::server::Json::parse(R"json(
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
    app.getConfig()->Local::setHost("127.0.0.1")->setPort(3337);
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
    app.use("/mcp", mcp::server::http::Router(server, *config, [](const auto& request) -> mcp::server::http::Authorization {
                using namespace mcp::server::http;
                const std::string credential = request->get("Authorization");
                Authorization result = Unauthorized{"Bearer realm=\"fixture\""};
                if (credential == "Bearer fixture") {
                    result = Authorized{mcp::server::Principal{"fixture-user", {{"scope", "test"}}}};
                } else if (credential == "Bearer anonymous") {
                    result = Authorized{};
                } else if (credential == "Bearer forbidden") {
                    result = Forbidden{};
                } else if (credential == "Bearer malformed") {
                    result = Unauthorized{"invalid\r\nchallenge"};
                } else if (credential == "Bearer throw") {
                    throw std::runtime_error("authorization failed");
                }
                return result;
            }));
    app.listen([](const auto&, const auto&) {
    });
    return WebApp::start();
}
