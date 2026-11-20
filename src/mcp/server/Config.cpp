// SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
#include "mcp/server/Config.h"
namespace mcp::server {
    Config::Config(utils::SubCommand* parent)
        : utils::SubCommand(parent, this, "Applications") {
        addOption("--maximum-message-bytes", "Maximum MCP message bytes", "bytes", std::size_t{1048576}, CLI::Range(1, 16777216));
        addOption("--maximum-output-bytes", "Maximum queued MCP output bytes", "bytes", std::size_t{1048576}, CLI::Range(1024, 16777216));
        addOption("--deadline-ms", "Ordinary request deadline", "milliseconds", 30000, CLI::Range(1, 300000));
        addOption("--allowed-origin", "Exact allowed browser Origin (repeatable)", "origin", std::vector<std::string>{}, CLI::Validator());
    }
    Config::~Config() = default;
    std::size_t Config::messageBytes() const {
        return getOption("--maximum-message-bytes")->as<std::size_t>();
    }
    std::size_t Config::outputBytes() const {
        return getOption("--maximum-output-bytes")->as<std::size_t>();
    }
    std::chrono::milliseconds Config::deadline() const {
        return std::chrono::milliseconds(getOption("--deadline-ms")->as<int>());
    }
    std::vector<std::string> Config::origins() const {
        return getOption("--allowed-origin")->as<std::vector<std::string>>();
    }
} // namespace mcp::server
