/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#ifndef AI_OPENAI_CODEX_TYPED_ITEMS_H
#define AI_OPENAI_CODEX_TYPED_ITEMS_H

#include "ai/openai/codex/typed/Types.h"

#include <cstdint>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace ai::openai::codex::typed {

    struct ItemMetadata {
        ItemId id;
        std::optional<ThreadId> threadId;
        std::optional<TurnId> turnId;
        Json raw;
    };

    struct AgentMessageItem {
        ItemMetadata metadata;
        std::string text;
        std::optional<std::string> phase;
    };

    struct UserMessageItem {
        ItemMetadata metadata;
        std::optional<std::string> clientId;
        Json content = Json::array();
    };

    struct ReasoningItem {
        ItemMetadata metadata;
        std::vector<std::string> summary;
        std::vector<std::string> content;
    };

    struct CommandExecutionItem {
        ItemMetadata metadata;
        std::string command;
        std::string cwd;
        std::string status;
        Json commandActions = Json::array();
        std::optional<std::string> processId;
        std::optional<std::int32_t> exitCode;
        std::optional<std::int64_t> durationMs;
        std::optional<std::string> output;
    };

    struct FileChangeItem {
        ItemMetadata metadata;
        Json changes = Json::array();
        std::string status;
    };

    struct ToolCallItem {
        ItemMetadata metadata;
        std::string type;
        std::optional<std::string> server;
        std::optional<std::string> nameSpace;
        std::string tool;
        std::string status;
        Json arguments = Json::object();
        Json result = nullptr;
        Json error = nullptr;
    };

    struct WebSearchItem {
        ItemMetadata metadata;
        std::string query;
        Json action = nullptr;
    };

    struct UnknownItemMetadata {
        std::optional<ItemId> id;
        std::optional<ThreadId> threadId;
        std::optional<TurnId> turnId;
    };

    struct UnknownItem {
        std::optional<std::string> type;
        Json raw;
        std::optional<std::string> decodingError;
        UnknownItemMetadata metadata = {};
        std::optional<DecodeDiagnostic> diagnostic;
    };

    using ThreadItem = std::variant<AgentMessageItem,
                                    ReasoningItem,
                                    CommandExecutionItem,
                                    FileChangeItem,
                                    ToolCallItem,
                                    WebSearchItem,
                                    UnknownItem,
                                    UserMessageItem>;

    // The pinned protocol has a separate ResponseItem union. Its domain
    // alternatives belong to A1.1, so A1.0 establishes only the distinct type
    // direction and does not merge those alternatives into ThreadItem.
    struct ResponseItem;

    // Transitional source compatibility for the A0 public name.
    using Item = ThreadItem;

} // namespace ai::openai::codex::typed

#endif // AI_OPENAI_CODEX_TYPED_ITEMS_H
