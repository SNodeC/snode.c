/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#ifndef AI_OPENAI_CODEX_TYPED_TURNS_H
#define AI_OPENAI_CODEX_TYPED_TURNS_H

#include "ai/openai/codex/AppServerClient.h"
#include "ai/openai/codex/typed/Items.h"
#include "ai/openai/codex/typed/Results.h"

#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace ai::openai::codex::typed {

    struct TextInput {
        std::string text;
    };

    struct ImageUrlInput {
        std::string url;
        std::optional<ImageDetail> detail;
    };

    struct LocalImageInput {
        std::string path;
        std::optional<ImageDetail> detail;
    };

    struct SkillInput {
        std::string name;
        std::string path;
    };

    struct MentionInput {
        std::string name;
        std::string path;
    };

    struct UnknownTurnInput {
        std::optional<std::string> type;
        Json raw;
    };

    using TurnInput = std::variant<TextInput, ImageUrlInput, LocalImageInput, SkillInput, MentionInput, UnknownTurnInput>;

    struct DangerFullAccessSandboxPolicy {};

    struct ReadOnlySandboxPolicy {
        std::optional<bool> networkAccess;
    };

    struct ExternalSandboxPolicy {
        std::optional<NetworkAccess> networkAccess;
    };

    struct WorkspaceWriteSandboxPolicy {
        std::vector<std::string> writableRoots;
        std::optional<bool> networkAccess;
        std::optional<bool> excludeTmpdirEnvVar;
        std::optional<bool> excludeSlashTmp;
    };

    using SandboxPolicy =
        std::variant<DangerFullAccessSandboxPolicy, ReadOnlySandboxPolicy, ExternalSandboxPolicy, WorkspaceWriteSandboxPolicy>;

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
