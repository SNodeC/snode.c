// SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
#pragma once
namespace mcp::server {
    class Server;
    class Config;
} // namespace mcp::server
namespace apps::mcp {
    void addOptions(::mcp::server::Config& config);
    void registerTools(::mcp::server::Server& server, const ::mcp::server::Config& config);
} // namespace apps::mcp
