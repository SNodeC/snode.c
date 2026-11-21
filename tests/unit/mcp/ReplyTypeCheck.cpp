// SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
#include "mcp/server/Reply.h"
#include <utility>
void check(mcp::server::MCP_REPLY& reply, mcp::server::MCP_RESULT result) {
    reply.result(std::move(result));
}
