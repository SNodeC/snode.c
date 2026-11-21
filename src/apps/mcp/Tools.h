// SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
#pragma once
#include "mcp/server/Config.h"
#include <cstddef>
#include <string>
namespace CLI {
    class Option;
}
namespace utils {
    class SubCommand;
}
namespace mcp::server {
    class Server;
} // namespace mcp::server
namespace apps::mcp {
    class DevelopmentConfig : public ::mcp::server::Config {
    public:
        explicit DevelopmentConfig(utils::SubCommand* parent);
        std::string getHeaderRoot() const;
        std::size_t getSearchFiles() const;
        std::size_t getSearchBytes() const;

    private:
        CLI::Option *headerRoot, *searchFiles, *searchBytes;
    };
    std::string instructions();
    void registerTools(::mcp::server::Server& server, const DevelopmentConfig& config);
} // namespace apps::mcp
