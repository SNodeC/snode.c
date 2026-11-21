// SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
#include "Log.h"
#include "Tools.h"
#include "core/SNodeC.h"
#include "mcp/server/Config.h"
#include "mcp/server/Server.h"
#ifdef MCP_STDIO
#include "mcp/server/stdio/Transport.h"
#include "utils/Config.h"

#include <unistd.h>
#include <utility>
#else
#include "express/legacy/in/WebApp.h"
#include "mcp/server/http/Router.h"
#include "web/http/ConfigHttpParser.h"
#include "web/http/server/ConfigHttpServer.h"
#endif

int main(int argc, char* argv[]) {
    int result = 1;
#ifdef MCP_STDIO
    int protocolFd = mcp::server::stdio::Transport::retainProtocol();
    if (protocolFd >= 0) {
        try {
            auto* config = utils::Config::configRoot.newSubCommand<mcp::server::Config>();
            apps::mcp::addOptions(*config);
            std::vector<char*> arguments(argv, argv + argc);
            char foreground[] = "--daemonize=false";
            arguments.push_back(foreground);
            arguments.push_back(nullptr);
            core::SNodeC::init(argc + 1, arguments.data());
            utils::Config::configRoot.setDefaultValue(utils::Config::configRoot.getOption("--log-console"), "stderr", false);
            mcp::server::Server server("snodec-dev");
            apps::mcp::registerTools(server, *config);
            mcp::server::stdio::Transport transport(server, *config, std::exchange(protocolFd, -1));
            snode::log::application("mcp.development").info() << "SNode.C development stdio ready";
            result = core::SNodeC::start();
        } catch (const std::exception& error) {
            snode::log::application("mcp.development").error() << error.what();
            core::SNodeC::free();
        }
        if (protocolFd >= 0) {
            ::close(protocolFd);
        }
    }
#else
    using WebApp = express::legacy::in::WebApp;
    const WebApp app("mcp-http");
    auto* config = app.getConfig()->Instance::newSubCommand<mcp::server::Config>();
    apps::mcp::addOptions(*config);
    auto* http = app.getConfig()->Instance::getSubCommand<web::http::server::ConfigHttpServer>();
    http->setAllowPipelining(false)->setMaximumPendingRequests(1);
    http->getParserConfig()->setMaximumBodyBytes(1048576)->setMaximumHeaderBytes(65536)->setMaximumHeaderFields(100);
    app.getConfig()->Connection::setMaximumWriteQueueBytes(1048576);
    app.getConfig()->Local::setHost("127.0.0.1")->setPort(3337);
    try {
        WebApp::init(argc, argv);
        if (app.getConfig()->Instance::getDisabled()) {
            core::SNodeC::free();
            result = 0;
        } else {
            mcp::server::Server server("snodec-dev");
            apps::mcp::registerTools(server, *config);
            app.use("/mcp", mcp::server::http::Router(server, *config));
            int listenFailure = 0;
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
                result = WebApp::start();
                if (listenFailure != 0) {
                    result = listenFailure;
                }
            }
        }
    } catch (const std::exception& error) {
        snode::log::application("mcp.development").error() << error.what();
        core::SNodeC::free();
    }
#endif
    return result;
}
