// SPDX-License-Identifier: LGPL-3.0-or-later OR MIT

#ifndef APPS_MCP_HOUSESTYLERESOURCE_H
#define APPS_MCP_HOUSESTYLERESOURCE_H

#include "mcp/server/Resource.h"
namespace apps::mcp {
    class HouseStyleResource : public ::mcp::server::Resource {
    public:
        HouseStyleResource();
        void read(const ::mcp::server::ResourceRead& request, const std::shared_ptr<::mcp::server::ResourceReply>& reply) override;
    };
} // namespace apps::mcp

#endif // APPS_MCP_HOUSESTYLERESOURCE_H
