/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#ifndef AI_OPENAI_CODEX_TYPED_TURNS_H
#define AI_OPENAI_CODEX_TYPED_TURNS_H

#include "ai/openai/codex/AppServerClient.h"
#include "ai/openai/codex/typed/CodexErrorInfo.h"
#include "ai/openai/codex/typed/Conversation.h"
#include "ai/openai/codex/typed/Items.h"
#include "ai/openai/codex/typed/Results.h"

#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace ai::openai::codex::typed {

    struct Turn {
        TurnId id;
        // The wire Turn aggregate is nested under a thread. Retain that
        // contextual identity for compatibility without treating it as a wire
        // property.
        ThreadId threadId;
        TurnStatus status;
        std::vector<ThreadItem> items;
        // The protocol omits this field for the legacy full-items view. Keep
        // the established semantic default while `raw` retains wire presence.
        TurnItemsView itemsView = TurnItemsView::full();
        OptionalNullable<TurnError> error;
        OptionalNullable<std::int64_t> startedAt;
        OptionalNullable<std::int64_t> completedAt;
        OptionalNullable<std::int64_t> durationMs;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;
    };

    struct TurnInterruptParams {
        ThreadId threadId;
        TurnId turnId;
    };

    struct TurnStartParams {
        ThreadId threadId;
        std::vector<UserInput> input;
        OptionalNullable<Personality> personality;
        OptionalNullable<AskForApproval> approvalPolicy;
        OptionalNullable<ApprovalsReviewer> approvalsReviewer;
        OptionalNullable<ClientUserMessageId> clientUserMessageId;
        OptionalNullable<std::string> serviceTier;
        OptionalNullable<std::string> cwd;
        OptionalNullable<ReasoningEffort> effort;
        OptionalNullable<ModelId> model;
        OptionalNullable<ReasoningSummary> summary;
        // ProtocolDefinedOpaqueJson: the pinned schema intentionally accepts
        // an arbitrary JSON Schema annotation here.
        OptionalNullable<Json> outputSchema;
        OptionalNullable<SandboxPolicy> sandboxPolicy;
    };

    struct TurnSteerParams {
        ThreadId threadId;
        TurnId expectedTurnId;
        std::vector<UserInput> input;
        OptionalNullable<ClientUserMessageId> clientUserMessageId;
    };

    struct TurnStartResponse {
        Turn turn;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;
    };

    struct TurnSteerResponse {
        TurnId turnId;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;
    };

    // Transitional compatibility for the formerly operation-specific empty
    // result type.
    using TurnInterruptResult = Unit;

    // Compatibility aggregate for the original convenience overload.
    struct TurnStartOptions {
        std::optional<std::string> cwd;
        std::optional<ModelId> model;
        std::optional<ReasoningEffort> reasoningEffort;
        std::optional<ApprovalPolicy> approvalPolicy;
        std::optional<SandboxPolicy> sandboxPolicy;
    };

    TurnStartParams toTurnStartParams(ThreadId threadId, std::vector<TurnInput> input, TurnStartOptions options);
    TurnInterruptParams toTurnInterruptParams(ThreadId threadId, TurnId turnId);

    class Turns {
    public:
        using Submission = AppServerClient::RawProtocol::Submission;
        using UnitResultHandler = std::function<void(const OperationResult<Unit>&)>;
        using TurnStartResultHandler = std::function<void(const OperationResult<TurnStartResponse>&)>;
        using TurnSteerResultHandler = std::function<void(const OperationResult<TurnSteerResponse>&)>;

        Submission interrupt(TurnInterruptParams params, UnitResultHandler handler);
        Submission start(TurnStartParams params, TurnStartResultHandler handler);
        Submission steer(TurnSteerParams params, TurnSteerResultHandler handler);

        using TurnResultHandler = std::function<void(const OperationResult<Turn>&)>;
        using InterruptResultHandler = UnitResultHandler;

        [[deprecated("use start(TurnStartParams, TurnStartResultHandler)")]]
        Submission start(ThreadId threadId, std::vector<TurnInput> input, TurnStartOptions options, TurnResultHandler handler);
        [[deprecated("use interrupt(TurnInterruptParams, UnitResultHandler)")]]
        Submission interrupt(ThreadId threadId, TurnId turnId, InterruptResultHandler handler);

    private:
        friend class ::ai::openai::codex::AppServerClient;

        explicit Turns(AppServerClient::RawProtocol& protocol) noexcept;

        AppServerClient::RawProtocol* protocol;
    };

} // namespace ai::openai::codex::typed

#endif // AI_OPENAI_CODEX_TYPED_TURNS_H
