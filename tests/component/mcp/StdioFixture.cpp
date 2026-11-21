// SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
#include "Log.h"
#include "core/timer/Timer.h"
#include "mcp/server/Config.h"
#include "mcp/server/Server.h"
#include "mcp/server/stdio/Transport.h"
#include "utils/Config.h"

#include <stdexcept>
using mcp::server::ToolResult;
int main(int argc, char* argv[]) {
    const auto objectSchema = mcp::server::Json::parse(R"json(
{
    "type": "object"
}
)json");
    auto* config = utils::Config::configRoot.newSubCommand<mcp::server::Config>();
    config->addOption("--fixture-schema",
                      "Test registration failure",
                      "kind",
                      std::string("valid"),
                      CLI::IsMember({"valid", "invalid", "throw", "throw-int"}));
    mcp::server::Server server("stdio-fixture", "test-version");
    const int result = mcp::server::stdio::Transport::run(argc, argv, server, *config, [&] {
        server.tool({"echo", {}, objectSchema}, [](const auto& call, const auto& reply) {
            const auto& args = call.arguments;
            reply->result(ToolResult::text(args.dump()));
        });
        server.tool({"delay", {}, objectSchema}, [](const auto&, const auto& reply) {
            core::timer::Timer::singleshotTimer(
                [reply] {
                    reply->result(ToolResult::content({}));
                },
                utils::Timeval(0.2));
        });
        if (config->getOption("--fixture-schema")->as<std::string>() == "invalid") {
            server.tool({"bad", {}, mcp::server::Json::parse(R"json(
{
    "$ref": "unsupported",
    "type": "object"
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
    });
    return result;
}
