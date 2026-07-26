/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#ifndef AI_OPENAI_CODEX_TYPED_REVIEWS_H
#define AI_OPENAI_CODEX_TYPED_REVIEWS_H

#include "ai/openai/codex/AppServerClient.h"
#include "ai/openai/codex/typed/Results.h"
#include "ai/openai/codex/typed/ServerRequests.h"
#include "ai/openai/codex/typed/Turns.h"
#include "ai/openai/codex/typed/Types.h"

#include <compare>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace ai::openai::codex::typed {

    struct ReviewDelivery {
        std::string value;

        static ReviewDelivery inlineReview() {
            return {"inline"};
        }

        static ReviewDelivery detached() {
            return {"detached"};
        }

        [[nodiscard]] bool isKnown() const noexcept {
            return value == "inline" || value == "detached";
        }

        auto operator<=>(const ReviewDelivery&) const = default;
    };

    struct UncommittedChangesReviewTarget {
        bool operator==(const UncommittedChangesReviewTarget&) const = default;
    };

    struct BaseBranchReviewTarget {
        std::string branch;

        bool operator==(const BaseBranchReviewTarget&) const = default;
    };

    struct CommitReviewTarget {
        std::string sha;
        OptionalNullable<std::string> title;

        bool operator==(const CommitReviewTarget&) const = default;
    };

    struct CustomReviewTarget {
        std::string instructions;

        bool operator==(const CustomReviewTarget&) const = default;
    };

    struct UnknownReviewTarget {
        std::optional<std::string> type;
        Json raw = Json::object();
        DecodeDiagnostic diagnostic;

        bool operator==(const UnknownReviewTarget&) const = default;
    };

    using ReviewTarget =
        std::variant<UncommittedChangesReviewTarget, BaseBranchReviewTarget, CommitReviewTarget, CustomReviewTarget, UnknownReviewTarget>;

    struct ReviewStartParams {
        ThreadId threadId;
        ReviewTarget target;
        OptionalNullable<ReviewDelivery> delivery;
    };

    struct ReviewStartResponse {
        ThreadId reviewThreadId;
        Turn turn;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;
    };

    struct GuardianCommandSource {
        std::string value;

        static GuardianCommandSource shell() {
            return {"shell"};
        }

        static GuardianCommandSource unifiedExec() {
            return {"unifiedExec"};
        }

        [[nodiscard]] bool isKnown() const noexcept {
            return value == "shell" || value == "unifiedExec";
        }

        auto operator<=>(const GuardianCommandSource&) const = default;
    };

    struct GuardianApprovalReviewStatus {
        std::string value;

        static GuardianApprovalReviewStatus inProgress() {
            return {"inProgress"};
        }

        static GuardianApprovalReviewStatus approved() {
            return {"approved"};
        }

        static GuardianApprovalReviewStatus denied() {
            return {"denied"};
        }

        static GuardianApprovalReviewStatus timedOut() {
            return {"timedOut"};
        }

        static GuardianApprovalReviewStatus aborted() {
            return {"aborted"};
        }

        [[nodiscard]] bool isKnown() const noexcept {
            return value == "inProgress" || value == "approved" || value == "denied" || value == "timedOut" || value == "aborted";
        }

        auto operator<=>(const GuardianApprovalReviewStatus&) const = default;
    };

    struct GuardianRiskLevel {
        std::string value;

        static GuardianRiskLevel low() {
            return {"low"};
        }

        static GuardianRiskLevel medium() {
            return {"medium"};
        }

        static GuardianRiskLevel high() {
            return {"high"};
        }

        static GuardianRiskLevel critical() {
            return {"critical"};
        }

        [[nodiscard]] bool isKnown() const noexcept {
            return value == "low" || value == "medium" || value == "high" || value == "critical";
        }

        auto operator<=>(const GuardianRiskLevel&) const = default;
    };

    struct GuardianUserAuthorization {
        std::string value;

        static GuardianUserAuthorization unknown() {
            return {"unknown"};
        }

        static GuardianUserAuthorization low() {
            return {"low"};
        }

        static GuardianUserAuthorization medium() {
            return {"medium"};
        }

        static GuardianUserAuthorization high() {
            return {"high"};
        }

        [[nodiscard]] bool isKnown() const noexcept {
            return value == "unknown" || value == "low" || value == "medium" || value == "high";
        }

        auto operator<=>(const GuardianUserAuthorization&) const = default;
    };

    struct AutoReviewDecisionSource {
        std::string value;

        static AutoReviewDecisionSource agent() {
            return {"agent"};
        }

        [[nodiscard]] bool isKnown() const noexcept {
            return value == "agent";
        }

        auto operator<=>(const AutoReviewDecisionSource&) const = default;
    };

    struct GuardianApprovalReview {
        GuardianApprovalReviewStatus status;
        OptionalNullable<std::string> rationale;
        OptionalNullable<GuardianRiskLevel> riskLevel;
        OptionalNullable<GuardianUserAuthorization> userAuthorization;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;
    };

    struct CommandGuardianApprovalReviewAction {
        std::string command;
        AbsolutePathBuf cwd;
        GuardianCommandSource source;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;
    };

    struct ExecveGuardianApprovalReviewAction {
        std::vector<std::string> argv;
        AbsolutePathBuf cwd;
        std::string program;
        GuardianCommandSource source;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;
    };

    struct ApplyPatchGuardianApprovalReviewAction {
        AbsolutePathBuf cwd;
        std::vector<AbsolutePathBuf> files;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;
    };

    struct NetworkAccessGuardianApprovalReviewAction {
        std::string host;
        std::uint16_t port = 0;
        NetworkApprovalProtocol protocol;
        std::string target;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;
    };

    struct McpToolCallGuardianApprovalReviewAction {
        OptionalNullable<std::string> connectorId;
        OptionalNullable<std::string> connectorName;
        std::string server;
        std::string toolName;
        OptionalNullable<std::string> toolTitle;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;
    };

    struct RequestPermissionsGuardianApprovalReviewAction {
        RequestPermissionProfile permissions;
        OptionalNullable<std::string> reason;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;
    };

    struct UnknownGuardianApprovalReviewAction {
        std::optional<std::string> type;
        Json raw = Json::object();
        DecodeDiagnostic diagnostic;

        bool operator==(const UnknownGuardianApprovalReviewAction&) const = default;
    };

    using GuardianApprovalReviewAction = std::variant<CommandGuardianApprovalReviewAction,
                                                      ExecveGuardianApprovalReviewAction,
                                                      ApplyPatchGuardianApprovalReviewAction,
                                                      NetworkAccessGuardianApprovalReviewAction,
                                                      McpToolCallGuardianApprovalReviewAction,
                                                      RequestPermissionsGuardianApprovalReviewAction,
                                                      UnknownGuardianApprovalReviewAction>;

    struct GuardianWarningNotification {
        std::string message;
        ThreadId threadId;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;
    };

    struct ItemGuardianApprovalReviewStartedNotification {
        GuardianApprovalReviewAction action;
        GuardianApprovalReview review;
        std::string reviewId;
        std::int64_t startedAtMs = 0;
        OptionalNullable<ItemId> targetItemId;
        ThreadId threadId;
        TurnId turnId;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;
    };

    struct ItemGuardianApprovalReviewCompletedNotification {
        GuardianApprovalReviewAction action;
        std::int64_t completedAtMs = 0;
        AutoReviewDecisionSource decisionSource;
        GuardianApprovalReview review;
        std::string reviewId;
        std::int64_t startedAtMs = 0;
        OptionalNullable<ItemId> targetItemId;
        ThreadId threadId;
        TurnId turnId;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;
    };

    struct ThreadApproveGuardianDeniedActionParams {
        ThreadId threadId;
        // ProtocolDefinedOpaqueJson: the pinned schema intentionally accepts
        // the serialized GuardianAssessmentEvent as any JSON value.
        Json event = nullptr;
    };

    class Reviews {
    public:
        using Submission = AppServerClient::RawProtocol::Submission;
        using ReviewStartResultHandler = std::function<void(const OperationResult<ReviewStartResponse>&)>;

        Submission start(ReviewStartParams params, ReviewStartResultHandler handler);

    private:
        friend class ::ai::openai::codex::AppServerClient;

        explicit Reviews(AppServerClient::RawProtocol& protocol) noexcept;

        AppServerClient::RawProtocol* protocol;
    };

} // namespace ai::openai::codex::typed

#endif // AI_OPENAI_CODEX_TYPED_REVIEWS_H
