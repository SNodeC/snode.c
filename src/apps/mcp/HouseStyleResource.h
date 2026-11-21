// SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
#pragma once
#include "mcp/server/Resource.h"
namespace apps::mcp {
    class HouseStyleResource : public ::mcp::server::Resource {
    public:
        HouseStyleResource();
        void read(const ::mcp::server::ResourceRead& request, const std::shared_ptr<::mcp::server::ResourceReply>& reply) override;
    };
} // namespace apps::mcp
