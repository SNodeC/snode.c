// SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
#include "HouseStyleResource.h"

#include "Tools.h"
#include "mcp/server/Request.h"
#include "mcp/server/Results.h"

#include <memory>
#include <string>
#include <variant>
#include <vector>
namespace apps::mcp {
    HouseStyleResource::HouseStyleResource()
        : Resource({"snodec://development/house-style", "SNode.C house style", {}, "text/plain"}) {
    }
    void HouseStyleResource::read(const ::mcp::server::ResourceRead& request, const std::shared_ptr<::mcp::server::ResourceReply>& reply) {
        reply->result(::mcp::server::ResourceResult{{::mcp::server::TextResource{request.uri, "text/plain", instructions()}}});
    }
} // namespace apps::mcp
