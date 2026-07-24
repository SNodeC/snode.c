/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#ifndef AI_OPENAI_CODEX_DETAIL_CONVERSATIONCODEC_H
#define AI_OPENAI_CODEX_DETAIL_CONVERSATIONCODEC_H

#include "ai/openai/codex/Protocol.h"
#include "ai/openai/codex/typed/Conversation.h"
#include "ai/openai/codex/typed/Items.h"
#include "ai/openai/codex/typed/Types.h"

#include <optional>
#include <string>

namespace ai::openai::codex::detail {

    template <typename T>
    struct ConversationDecodeResult {
        std::optional<T> value;
        std::optional<typed::DecodeDiagnostic> diagnostic;

        [[nodiscard]] explicit operator bool() const noexcept {
            return value.has_value();
        }
    };

    ConversationDecodeResult<typed::AskForApproval> decodeAskForApproval(const Json& value) noexcept;
    ConversationDecodeResult<typed::AgentMessageInputContent> decodeAgentMessageInputContent(const Json& value) noexcept;
    ConversationDecodeResult<typed::CommandAction> decodeCommandAction(const Json& value) noexcept;
    ConversationDecodeResult<typed::ContentItem> decodeContentItem(const Json& value) noexcept;
    ConversationDecodeResult<typed::DynamicToolCallOutputContentItem> decodeDynamicToolCallOutputContentItem(const Json& value) noexcept;
    ConversationDecodeResult<typed::FunctionCallOutputContentItem> decodeFunctionCallOutputContentItem(const Json& value) noexcept;
    ConversationDecodeResult<typed::LocalShellAction> decodeLocalShellAction(const Json& value) noexcept;
    ConversationDecodeResult<typed::PatchChangeKind> decodePatchChangeKind(const Json& value) noexcept;
    ConversationDecodeResult<typed::ReasoningItemContent> decodeReasoningItemContent(const Json& value) noexcept;
    ConversationDecodeResult<typed::ReasoningItemReasoningSummary> decodeReasoningItemReasoningSummary(const Json& value) noexcept;
    ConversationDecodeResult<typed::ResponsesApiWebSearchAction> decodeResponsesApiWebSearchAction(const Json& value) noexcept;
    ConversationDecodeResult<typed::SandboxPolicy> decodeSandboxPolicy(const Json& value) noexcept;
    ConversationDecodeResult<typed::UserInput> decodeUserInput(const Json& value) noexcept;
    ConversationDecodeResult<typed::WebSearchAction> decodeWebSearchAction(const Json& value) noexcept;

    std::optional<Json> encodeAskForApproval(const typed::AskForApproval& value, std::string& error) noexcept;
    std::optional<Json> encodeSandboxPolicy(const typed::SandboxPolicy& value, std::string& error) noexcept;
    std::optional<Json> encodeUserInput(const typed::UserInput& value, std::string& error) noexcept;

} // namespace ai::openai::codex::detail

#endif // AI_OPENAI_CODEX_DETAIL_CONVERSATIONCODEC_H
