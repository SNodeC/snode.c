// SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
#pragma once
#include "Tools.h"
#include "mcp/server/Tool.h"
namespace apps::mcp {
    class ReadTool : public ::mcp::server::Tool {
    public:
        ReadTool(const DevelopmentConfig& config);
        void call(const ::mcp::server::ToolCall& call, const std::shared_ptr<::mcp::server::ToolReply>& reply) override;

    private:
        std::shared_ptr<Headers> headers;
    };
} // namespace apps::mcp
