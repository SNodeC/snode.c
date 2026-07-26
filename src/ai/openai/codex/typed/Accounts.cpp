/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include "ai/openai/codex/typed/Accounts.h"

namespace ai::openai::codex::typed {

    ChatgptAuthTokensRefreshReason ChatgptAuthTokensRefreshReason::unauthorized() {
        return {"unauthorized"};
    }

    bool ChatgptAuthTokensRefreshReason::isKnown() const noexcept {
        return value == "unauthorized";
    }

    AddCreditsNudgeCreditType AddCreditsNudgeCreditType::credits() {
        return {"credits"};
    }

    AddCreditsNudgeCreditType AddCreditsNudgeCreditType::usageLimit() {
        return {"usage_limit"};
    }

    bool AddCreditsNudgeCreditType::isKnown() const noexcept {
        return value == "credits" || value == "usage_limit";
    }

    AddCreditsNudgeEmailStatus AddCreditsNudgeEmailStatus::sent() {
        return {"sent"};
    }

    AddCreditsNudgeEmailStatus AddCreditsNudgeEmailStatus::cooldownActive() {
        return {"cooldown_active"};
    }

    bool AddCreditsNudgeEmailStatus::isKnown() const noexcept {
        return value == "sent" || value == "cooldown_active";
    }

    AmazonBedrockCredentialSource AmazonBedrockCredentialSource::codexManaged() {
        return {"codexManaged"};
    }

    AmazonBedrockCredentialSource AmazonBedrockCredentialSource::awsManaged() {
        return {"awsManaged"};
    }

    bool AmazonBedrockCredentialSource::isKnown() const noexcept {
        return value == "codexManaged" || value == "awsManaged";
    }

    AuthMode AuthMode::apiKey() {
        return {"apikey"};
    }

    AuthMode AuthMode::chatgpt() {
        return {"chatgpt"};
    }

    AuthMode AuthMode::chatgptAuthTokens() {
        return {"chatgptAuthTokens"};
    }

    AuthMode AuthMode::headers() {
        return {"headers"};
    }

    AuthMode AuthMode::agentIdentity() {
        return {"agentIdentity"};
    }

    AuthMode AuthMode::personalAccessToken() {
        return {"personalAccessToken"};
    }

    AuthMode AuthMode::bedrockApiKey() {
        return {"bedrockApiKey"};
    }

    bool AuthMode::isKnown() const noexcept {
        return value == "apikey" || value == "chatgpt" || value == "chatgptAuthTokens" || value == "headers" ||
               value == "agentIdentity" || value == "personalAccessToken" || value == "bedrockApiKey";
    }

    CancelLoginAccountStatus CancelLoginAccountStatus::canceled() {
        return {"canceled"};
    }

    CancelLoginAccountStatus CancelLoginAccountStatus::notFound() {
        return {"notFound"};
    }

    bool CancelLoginAccountStatus::isKnown() const noexcept {
        return value == "canceled" || value == "notFound";
    }

    ConsumeAccountRateLimitResetCreditOutcome ConsumeAccountRateLimitResetCreditOutcome::reset() {
        return {"reset"};
    }

    ConsumeAccountRateLimitResetCreditOutcome ConsumeAccountRateLimitResetCreditOutcome::nothingToReset() {
        return {"nothingToReset"};
    }

    ConsumeAccountRateLimitResetCreditOutcome ConsumeAccountRateLimitResetCreditOutcome::noCredit() {
        return {"noCredit"};
    }

    ConsumeAccountRateLimitResetCreditOutcome ConsumeAccountRateLimitResetCreditOutcome::alreadyRedeemed() {
        return {"alreadyRedeemed"};
    }

    bool ConsumeAccountRateLimitResetCreditOutcome::isKnown() const noexcept {
        return value == "reset" || value == "nothingToReset" || value == "noCredit" || value == "alreadyRedeemed";
    }

    LoginAppBrand LoginAppBrand::codex() {
        return {"codex"};
    }

    LoginAppBrand LoginAppBrand::chatgpt() {
        return {"chatgpt"};
    }

    bool LoginAppBrand::isKnown() const noexcept {
        return value == "codex" || value == "chatgpt";
    }

    PlanType PlanType::free() {
        return {"free"};
    }

    PlanType PlanType::go() {
        return {"go"};
    }

    PlanType PlanType::plus() {
        return {"plus"};
    }

    PlanType PlanType::pro() {
        return {"pro"};
    }

    PlanType PlanType::proLite() {
        return {"prolite"};
    }

    PlanType PlanType::team() {
        return {"team"};
    }

    PlanType PlanType::selfServeBusinessUsageBased() {
        return {"self_serve_business_usage_based"};
    }

    PlanType PlanType::business() {
        return {"business"};
    }

    PlanType PlanType::enterpriseCbpUsageBased() {
        return {"enterprise_cbp_usage_based"};
    }

    PlanType PlanType::enterprise() {
        return {"enterprise"};
    }

    PlanType PlanType::edu() {
        return {"edu"};
    }

    PlanType PlanType::unknown() {
        return {"unknown"};
    }

    bool PlanType::isKnown() const noexcept {
        return value == "free" || value == "go" || value == "plus" || value == "pro" || value == "prolite" ||
               value == "team" || value == "self_serve_business_usage_based" || value == "business" ||
               value == "enterprise_cbp_usage_based" || value == "enterprise" || value == "edu" || value == "unknown";
    }

    RateLimitReachedType RateLimitReachedType::rateLimitReached() {
        return {"rate_limit_reached"};
    }

    RateLimitReachedType RateLimitReachedType::workspaceOwnerCreditsDepleted() {
        return {"workspace_owner_credits_depleted"};
    }

    RateLimitReachedType RateLimitReachedType::workspaceMemberCreditsDepleted() {
        return {"workspace_member_credits_depleted"};
    }

    RateLimitReachedType RateLimitReachedType::workspaceOwnerUsageLimitReached() {
        return {"workspace_owner_usage_limit_reached"};
    }

    RateLimitReachedType RateLimitReachedType::workspaceMemberUsageLimitReached() {
        return {"workspace_member_usage_limit_reached"};
    }

    bool RateLimitReachedType::isKnown() const noexcept {
        return value == "rate_limit_reached" || value == "workspace_owner_credits_depleted" ||
               value == "workspace_member_credits_depleted" || value == "workspace_owner_usage_limit_reached" ||
               value == "workspace_member_usage_limit_reached";
    }

    RateLimitResetCreditStatus RateLimitResetCreditStatus::available() {
        return {"available"};
    }

    RateLimitResetCreditStatus RateLimitResetCreditStatus::redeeming() {
        return {"redeeming"};
    }

    RateLimitResetCreditStatus RateLimitResetCreditStatus::redeemed() {
        return {"redeemed"};
    }

    RateLimitResetCreditStatus RateLimitResetCreditStatus::unknown() {
        return {"unknown"};
    }

    bool RateLimitResetCreditStatus::isKnown() const noexcept {
        return value == "available" || value == "redeeming" || value == "redeemed" || value == "unknown";
    }

    RateLimitResetType RateLimitResetType::codexRateLimits() {
        return {"codexRateLimits"};
    }

    RateLimitResetType RateLimitResetType::unknown() {
        return {"unknown"};
    }

    bool RateLimitResetType::isKnown() const noexcept {
        return value == "codexRateLimits" || value == "unknown";
    }

    WorkspaceMessageType WorkspaceMessageType::headline() {
        return {"headline"};
    }

    WorkspaceMessageType WorkspaceMessageType::announcement() {
        return {"announcement"};
    }

    WorkspaceMessageType WorkspaceMessageType::unknown() {
        return {"unknown"};
    }

    bool WorkspaceMessageType::isKnown() const noexcept {
        return value == "headline" || value == "announcement" || value == "unknown";
    }

} // namespace ai::openai::codex::typed
