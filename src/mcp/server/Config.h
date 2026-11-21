// SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
#ifndef MCP_SERVER_CONFIG_H
#define MCP_SERVER_CONFIG_H
#include "utils/SubCommand.h"

#include <chrono>
#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace mcp::server {
    class Config : public utils::SubCommand {
    public:
        static constexpr std::string_view NAME = "mcp";
        static constexpr std::string_view DESCRIPTION = "MCP limits and Origin policy";
        explicit Config(utils::SubCommand* parent);
        ~Config() override;
        std::size_t messageBytes() const;
        std::size_t outputBytes() const;
        std::chrono::milliseconds deadline() const;
        std::vector<std::string> origins() const;
    };
} // namespace mcp::server
#endif
