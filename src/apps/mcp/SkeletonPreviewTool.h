// SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
#pragma once
#include "mcp/server/Tool.h"
namespace apps::mcp {
    class SkeletonPreviewTool : public ::mcp::server::Tool {
    public:
        SkeletonPreviewTool();
        void call(const ::mcp::server::ToolCall& call, const std::shared_ptr<::mcp::server::ToolReply>& reply) override;
    };
} // namespace apps::mcp
