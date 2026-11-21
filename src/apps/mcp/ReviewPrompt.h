// SPDX-License-Identifier: LGPL-3.0-or-later OR MIT

#ifndef APPS_MCP_REVIEWPROMPT_H
#define APPS_MCP_REVIEWPROMPT_H

#include "mcp/server/Prompt.h"
#include "mcp/server/Reply.h"
namespace apps::mcp {
    class ReviewPrompt : public ::mcp::server::Prompt {
    public:
        ReviewPrompt();
        void get(const ::mcp::server::PromptGet& request, const std::shared_ptr<::mcp::server::PromptReply>& reply) override;
    };
} // namespace apps::mcp

#endif // APPS_MCP_REVIEWPROMPT_H
