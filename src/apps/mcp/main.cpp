// SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
#include "FindTool.h"
#include "HouseStyleResource.h"
#include "Log.h"
#include "ReadTool.h"
#include "ReviewPrompt.h"
#include "SkeletonPreviewTool.h"
#include "Tools.h"
#include "mcp-config.h"
#include "mcp/server/Server.h"

#ifdef MCP_STDIO
#include "mcp/server/stdio/Transport.h"
#include "utils/Config.h"

#include <memory>
#include <string>

#else
#include "core/SNodeC.h"
#include <exception>
#include "express/legacy/in/WebApp.h"
#include "mcp/server/http/Router.h"
#include "web/http/ConfigHttpParser.h"
#include "web/http/server/ConfigHttpServer.h"
#endif


int main(int argc, char* argv[]) {
    int result = 1;
    mcp::server::Server server("snodec-dev", SNODEC_DEV_VERSION, apps::mcp::instructions());
#ifdef MCP_STDIO
    auto* config = utils::Config::configRoot.newSubCommand<apps::mcp::DevelopmentConfig>();
    result = mcp::server::stdio::Transport::run(argc, argv, server, *config, [&] {
        server.tool(std::make_shared<apps::mcp::FindTool>(*config));
        server.tool(std::make_shared<apps::mcp::ReadTool>(*config));
        server.tool(std::make_shared<apps::mcp::SkeletonPreviewTool>());
        server.resource(std::make_shared<apps::mcp::HouseStyleResource>());
        server.prompt(std::make_shared<apps::mcp::ReviewPrompt>());
        snode::log::application("mcp.development").info() << "SNode.C development stdio ready";
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
            server.tool(std::make_shared<apps::mcp::FindTool>(*config));
            server.tool(std::make_shared<apps::mcp::ReadTool>(*config));
            server.tool(std::make_shared<apps::mcp::SkeletonPreviewTool>());
            server.resource(std::make_shared<apps::mcp::HouseStyleResource>());
            server.prompt(std::make_shared<apps::mcp::ReviewPrompt>());
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
