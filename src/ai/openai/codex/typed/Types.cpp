/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include "ai/openai/codex/typed/Types.h"

#include "ai/openai/codex/typed/CodexErrorInfo.h"
#include "ai/openai/codex/typed/ServerRequests.h"

namespace ai::openai::codex::typed {

    NonSteerableTurnKind NonSteerableTurnKind::review() {
        return {"review"};
    }

    NonSteerableTurnKind NonSteerableTurnKind::compact() {
        return {"compact"};
    }

    ReasoningEffort ReasoningEffort::minimal() {
        return {"minimal"};
    }

    ReasoningEffort ReasoningEffort::low() {
        return {"low"};
    }

    ReasoningEffort ReasoningEffort::medium() {
        return {"medium"};
    }

    ReasoningEffort ReasoningEffort::high() {
        return {"high"};
    }

    ReasoningEffort ReasoningEffort::xhigh() {
        return {"xhigh"};
    }

    bool ReasoningEffort::isKnown() const noexcept {
        return value == "minimal" || value == "low" || value == "medium" || value == "high" || value == "xhigh";
    }

    ApprovalPolicy ApprovalPolicy::untrusted() {
        return {"untrusted"};
    }

    ApprovalPolicy ApprovalPolicy::onRequest() {
        return {"on-request"};
    }

    ApprovalPolicy ApprovalPolicy::never() {
        return {"never"};
    }

    bool ApprovalPolicy::isKnown() const noexcept {
        return value == "untrusted" || value == "on-request" || value == "never";
    }

    SandboxMode SandboxMode::readOnly() {
        return {"read-only"};
    }

    SandboxMode SandboxMode::workspaceWrite() {
        return {"workspace-write"};
    }

    SandboxMode SandboxMode::dangerFullAccess() {
        return {"danger-full-access"};
    }

    bool SandboxMode::isKnown() const noexcept {
        return value == "read-only" || value == "workspace-write" || value == "danger-full-access";
    }

    ApprovalsReviewer ApprovalsReviewer::user() {
        return {"user"};
    }

    ApprovalsReviewer ApprovalsReviewer::autoReview() {
        return {"auto_review"};
    }

    ApprovalsReviewer ApprovalsReviewer::guardianSubagent() {
        return {"guardian_subagent"};
    }

    bool ApprovalsReviewer::isKnown() const noexcept {
        return value == "user" || value == "auto_review" || value == "guardian_subagent";
    }

    Personality Personality::none() {
        return {"none"};
    }

    Personality Personality::friendly() {
        return {"friendly"};
    }

    Personality Personality::pragmatic() {
        return {"pragmatic"};
    }

    bool Personality::isKnown() const noexcept {
        return value == "none" || value == "friendly" || value == "pragmatic";
    }

    ReasoningSummary ReasoningSummary::automatic() {
        return {"auto"};
    }

    ReasoningSummary ReasoningSummary::concise() {
        return {"concise"};
    }

    ReasoningSummary ReasoningSummary::detailed() {
        return {"detailed"};
    }

    ReasoningSummary ReasoningSummary::none() {
        return {"none"};
    }

    bool ReasoningSummary::isKnown() const noexcept {
        return value == "auto" || value == "concise" || value == "detailed" || value == "none";
    }

    SortDirection SortDirection::ascending() {
        return {"asc"};
    }

    SortDirection SortDirection::descending() {
        return {"desc"};
    }

    bool SortDirection::isKnown() const noexcept {
        return value == "asc" || value == "desc";
    }

    ThreadActiveFlag ThreadActiveFlag::waitingOnApproval() {
        return {"waitingOnApproval"};
    }

    ThreadActiveFlag ThreadActiveFlag::waitingOnUserInput() {
        return {"waitingOnUserInput"};
    }

    bool ThreadActiveFlag::isKnown() const noexcept {
        return value == "waitingOnApproval" || value == "waitingOnUserInput";
    }

    ThreadGoalStatus ThreadGoalStatus::active() {
        return {"active"};
    }

    ThreadGoalStatus ThreadGoalStatus::paused() {
        return {"paused"};
    }

    ThreadGoalStatus ThreadGoalStatus::blocked() {
        return {"blocked"};
    }

    ThreadGoalStatus ThreadGoalStatus::usageLimited() {
        return {"usageLimited"};
    }

    ThreadGoalStatus ThreadGoalStatus::budgetLimited() {
        return {"budgetLimited"};
    }

    ThreadGoalStatus ThreadGoalStatus::complete() {
        return {"complete"};
    }

    bool ThreadGoalStatus::isKnown() const noexcept {
        return value == "active" || value == "paused" || value == "blocked" || value == "usageLimited" ||
               value == "budgetLimited" || value == "complete";
    }

    ThreadSortKey ThreadSortKey::createdAt() {
        return {"created_at"};
    }

    ThreadSortKey ThreadSortKey::updatedAt() {
        return {"updated_at"};
    }

    ThreadSortKey ThreadSortKey::recencyAt() {
        return {"recency_at"};
    }

    bool ThreadSortKey::isKnown() const noexcept {
        return value == "created_at" || value == "updated_at" || value == "recency_at";
    }

    ThreadSourceKind ThreadSourceKind::cli() {
        return {"cli"};
    }

    ThreadSourceKind ThreadSourceKind::vscode() {
        return {"vscode"};
    }

    ThreadSourceKind ThreadSourceKind::exec() {
        return {"exec"};
    }

    ThreadSourceKind ThreadSourceKind::appServer() {
        return {"appServer"};
    }

    ThreadSourceKind ThreadSourceKind::subAgent() {
        return {"subAgent"};
    }

    ThreadSourceKind ThreadSourceKind::subAgentReview() {
        return {"subAgentReview"};
    }

    ThreadSourceKind ThreadSourceKind::subAgentCompact() {
        return {"subAgentCompact"};
    }

    ThreadSourceKind ThreadSourceKind::subAgentThreadSpawn() {
        return {"subAgentThreadSpawn"};
    }

    ThreadSourceKind ThreadSourceKind::subAgentOther() {
        return {"subAgentOther"};
    }

    ThreadSourceKind ThreadSourceKind::unknown() {
        return {"unknown"};
    }

    bool ThreadSourceKind::isKnown() const noexcept {
        return value == "cli" || value == "vscode" || value == "exec" || value == "appServer" || value == "subAgent" ||
               value == "subAgentReview" || value == "subAgentCompact" || value == "subAgentThreadSpawn" ||
               value == "subAgentOther" || value == "unknown";
    }

    ThreadStartSource ThreadStartSource::startup() {
        return {"startup"};
    }

    ThreadStartSource ThreadStartSource::clear() {
        return {"clear"};
    }

    bool ThreadStartSource::isKnown() const noexcept {
        return value == "startup" || value == "clear";
    }

    ThreadUnsubscribeStatus ThreadUnsubscribeStatus::notLoaded() {
        return {"notLoaded"};
    }

    ThreadUnsubscribeStatus ThreadUnsubscribeStatus::notSubscribed() {
        return {"notSubscribed"};
    }

    ThreadUnsubscribeStatus ThreadUnsubscribeStatus::unsubscribed() {
        return {"unsubscribed"};
    }

    bool ThreadUnsubscribeStatus::isKnown() const noexcept {
        return value == "notLoaded" || value == "notSubscribed" || value == "unsubscribed";
    }

    ImageDetail ImageDetail::automatic() {
        return {"auto"};
    }

    ImageDetail ImageDetail::low() {
        return {"low"};
    }

    ImageDetail ImageDetail::high() {
        return {"high"};
    }

    ImageDetail ImageDetail::original() {
        return {"original"};
    }

    bool ImageDetail::isKnown() const noexcept {
        return value == "auto" || value == "low" || value == "high" || value == "original";
    }

    NetworkAccess NetworkAccess::restricted() {
        return {"restricted"};
    }

    NetworkAccess NetworkAccess::enabled() {
        return {"enabled"};
    }

    bool NetworkAccess::isKnown() const noexcept {
        return value == "restricted" || value == "enabled";
    }

    TurnStatus TurnStatus::completed() {
        return {"completed"};
    }

    TurnStatus TurnStatus::interrupted() {
        return {"interrupted"};
    }

    TurnStatus TurnStatus::failed() {
        return {"failed"};
    }

    TurnStatus TurnStatus::inProgress() {
        return {"inProgress"};
    }

    bool TurnStatus::isKnown() const noexcept {
        return value == "completed" || value == "interrupted" || value == "failed" || value == "inProgress";
    }

    TurnItemsView TurnItemsView::notLoaded() {
        return {"notLoaded"};
    }

    TurnItemsView TurnItemsView::summary() {
        return {"summary"};
    }

    TurnItemsView TurnItemsView::full() {
        return {"full"};
    }

    bool TurnItemsView::isKnown() const noexcept {
        return value == "notLoaded" || value == "summary" || value == "full";
    }

    ApprovalDecision ApprovalDecision::accept() {
        return {"accept"};
    }

    ApprovalDecision ApprovalDecision::acceptForSession() {
        return {"acceptForSession"};
    }

    ApprovalDecision ApprovalDecision::decline() {
        return {"decline"};
    }

    ApprovalDecision ApprovalDecision::cancel() {
        return {"cancel"};
    }

} // namespace ai::openai::codex::typed
