// SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
#include "Log.h"
#include "core/SNodeC.h"
#include "core/timer/Timer.h"
#include "mcp/server/Config.h"
#include "mcp/server/Server.h"
#include "mcp/server/stdio/Transport.h"
#include "utils/Config.h"

#include <unistd.h>
using mcp::server::ToolResult;
int main(int argc, char* argv[]) {
    const int fd = mcp::server::stdio::Transport::retainProtocol();
    int result = 1;
    if (fd >= 0) {
        auto* config = utils::Config::configRoot.newSubCommand<mcp::server::Config>();
        config->addOption(
            "--fixture-schema", "Test registration failure", "kind", std::string("valid"), CLI::IsMember({"valid", "invalid"}));
        std::vector<char*> arguments(argv, argv + argc);
        char foreground[] = "--daemonize=false";
        arguments.push_back(foreground);
        arguments.push_back(nullptr);
        core::SNodeC::init(argc + 1, arguments.data());
        utils::Config::configRoot.setDefaultValue(utils::Config::configRoot.getOption("--log-console"), "stderr", false);
        try {
            mcp::server::Server server("stdio-fixture", "test-version");
            server.tool({"echo", {}, {{"type", "object"}}}, [](const auto& call, const auto& reply) {
                const auto& args = call.arguments;
                reply->result(ToolResult::text(args.dump()));
            });
            server.tool({"delay", {}, {{"type", "object"}}}, [](const auto&, const auto& reply) {
                core::timer::Timer::singleshotTimer(
                    [reply] {
                        reply->result(ToolResult::content({}));
                    },
                    utils::Timeval(0.2));
            });
            if (config->getOption("--fixture-schema")->as<std::string>() == "invalid") {
                server.tool({"bad", {}, {{"type", "object"}, {"$ref", "unsupported"}}}, [](const auto&, const auto&) {
                });
            }
            mcp::server::stdio::Transport transport(server, *config, fd);
            snode::log::application("mcp.fixture").info() << "MCP fixture ready";
            result = core::SNodeC::start();
        } catch (const std::exception& error) {
            snode::log::application().error() << error.what();
            core::SNodeC::free();
        }
    }
    return result;
}
