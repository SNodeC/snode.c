/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#ifndef AI_OPENAI_CODEX_TYPED_TURNS_H
#define AI_OPENAI_CODEX_TYPED_TURNS_H

#include "ai/openai/codex/AppServerClient.h"
#include "ai/openai/codex/typed/Conversation.h"
#include "ai/openai/codex/typed/Items.h"
#include "ai/openai/codex/typed/Results.h"

#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace ai::openai::codex::typed {

    struct Turn {
        TurnId id;
        ThreadId threadId;
        TurnStatus status;
        TurnItemsView itemsView = TurnItemsView::full();
        std::vector<Item> items;
        std::optional<Json> error;
        std::optional<std::int64_t> startedAt;
        std::optional<std::int64_t> completedAt;
        std::optional<std::int64_t> durationMs;
        Json raw;
    };

    struct TurnStartOptions {
        std::optional<std::string> cwd;
        std::optional<ModelId> model;
        std::optional<ReasoningEffort> reasoningEffort;
        std::optional<ApprovalPolicy> approvalPolicy;
        std::optional<SandboxPolicy> sandboxPolicy;
    };

    struct TurnInterruptResult {};

    class Turns {
    public:
        using Submission = AppServerClient::RawProtocol::Submission;
        using TurnResultHandler = std::function<void(const OperationResult<Turn>&)>;
        using InterruptResultHandler = std::function<void(const OperationResult<TurnInterruptResult>&)>;

        Submission start(ThreadId threadId, std::vector<TurnInput> input, TurnStartOptions options, TurnResultHandler handler);
        Submission interrupt(ThreadId threadId, TurnId turnId, InterruptResultHandler handler);

    private:
        friend class ::ai::openai::codex::AppServerClient;

        explicit Turns(AppServerClient::RawProtocol& protocol) noexcept;

        AppServerClient::RawProtocol* protocol;
    };

} // namespace ai::openai::codex::typed

#endif // AI_OPENAI_CODEX_TYPED_TURNS_H
