// SPDX-License-Identifier: LGPL-3.0-or-later OR MIT

#ifndef APPS_MCP_SKELETONPREVIEWTOOL_H
#define APPS_MCP_SKELETONPREVIEWTOOL_H

#include "mcp/server/Reply.h"
#include "mcp/server/Tool.h"
namespace apps::mcp {
    class SkeletonPreviewTool : public ::mcp::server::Tool {
    public:
        SkeletonPreviewTool();
        void call(const ::mcp::server::ToolCall& request, const std::shared_ptr<::mcp::server::ToolReply>& reply) override;
    };
} // namespace apps::mcp

#endif // APPS_MCP_SKELETONPREVIEWTOOL_H
