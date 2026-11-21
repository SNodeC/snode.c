// SPDX-License-Identifier: LGPL-3.0-or-later OR MIT

#ifndef APPS_MCP_FINDTOOL_H
#define APPS_MCP_FINDTOOL_H

#include "mcp/server/Reply.h"
#include "mcp/server/Tool.h"

#include <cstddef>
#include <memory>
namespace apps::mcp {
    class Headers;
    class DevelopmentConfig;
    class FindTool : public ::mcp::server::Tool {
    public:
        explicit FindTool(const DevelopmentConfig& config);
        void call(const ::mcp::server::ToolCall& request, const std::shared_ptr<::mcp::server::ToolReply>& reply) override;

    private:
        std::shared_ptr<Headers> headers;
        std::size_t files;
        std::size_t bytes;
    };
} // namespace apps::mcp

#endif // APPS_MCP_FINDTOOL_H
