// SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
#pragma once
#include "mcp/server/Prompt.h"
namespace apps::mcp {
    class ReviewPrompt : public ::mcp::server::Prompt {
    public:
        ReviewPrompt();
        void get(const ::mcp::server::PromptGet& call, const std::shared_ptr<::mcp::server::PromptReply>& reply) override;
    };
} // namespace apps::mcp
