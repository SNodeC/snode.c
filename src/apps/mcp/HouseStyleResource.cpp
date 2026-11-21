// SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
#include "HouseStyleResource.h"

#include "Tools.h"
namespace apps::mcp {
    HouseStyleResource::HouseStyleResource()
        : Resource({"snodec://development/house-style", "SNode.C house style", {}, "text/plain"}) {
    }
    void HouseStyleResource::read(const ::mcp::server::ResourceRead&, const std::shared_ptr<::mcp::server::ResourceReply>& reply) {
        reply->result(
            ::mcp::server::ResourceResult{{::mcp::server::TextResource{"snodec://development/house-style", "text/plain", instructions()}}});
    }
} // namespace apps::mcp
