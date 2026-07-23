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

    ApprovalPolicy ApprovalPolicy::untrusted() {
        return {"untrusted"};
    }

    ApprovalPolicy ApprovalPolicy::onRequest() {
        return {"on-request"};
    }

    ApprovalPolicy ApprovalPolicy::never() {
        return {"never"};
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

    TurnItemsView TurnItemsView::notLoaded() {
        return {"notLoaded"};
    }

    TurnItemsView TurnItemsView::summary() {
        return {"summary"};
    }

    TurnItemsView TurnItemsView::full() {
        return {"full"};
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
