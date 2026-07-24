/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include "ai/openai/codex/detail/AccountCodec.h"
#include "ai/openai/codex/detail/ServerRequestDecoder.h"
#include "ai/openai/codex/typed/Accounts.h"
#include "support/TestResult.h"

#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace {
    namespace codex = ai::openai::codex;
    namespace detail = ai::openai::codex::detail;
    namespace typed = ai::openai::codex::typed;

    constexpr const char* SyntheticApiKey = "synthetic-api-key-for-account-codec";
    constexpr const char* SyntheticAccessToken = "synthetic-access-token-for-account-codec";
    constexpr const char* SyntheticAccountId = "synthetic-account-id";
    constexpr const char* SyntheticDeviceCode = "synthetic-device-code";

    bool hasDiagnostic(const std::vector<typed::DecodeDiagnostic>& diagnostics,
                       typed::DecodeIssueKind kind,
                       typed::DecodeIssueSeverity severity,
                       std::string_view path) {
        for (const typed::DecodeDiagnostic& diagnostic : diagnostics) {
            if (diagnostic.kind == kind && diagnostic.severity == severity && diagnostic.fieldPath == path) {
                return true;
            }
        }
        return false;
    }

    bool hasDiagnostic(const std::optional<typed::DecodeDiagnostic>& diagnostic,
                       typed::DecodeIssueKind kind,
                       typed::DecodeIssueSeverity severity,
                       std::string_view path) {
        return diagnostic && diagnostic->kind == kind && diagnostic->severity == severity && diagnostic->fieldPath == path;
    }

    bool diagnosticDoesNotContain(const typed::DecodeDiagnostic& diagnostic, std::string_view value) {
        return diagnostic.surface.find(value) == std::string::npos &&
               diagnostic.fieldPath.find(value) == std::string::npos &&
               diagnostic.message.find(value) == std::string::npos;
    }

    template <typename OpenEnum>
    void expectKnown(tests::support::TestResult& result,
                     const OpenEnum& value,
                     std::string_view expected,
                     std::string_view label) {
        result.expectTrue(value.value == expected && value.isKnown(), std::string(label) + " factory is exact and known");
    }

    template <typename OpenEnum>
    void expectFuture(tests::support::TestResult& result, std::string_view label) {
        const OpenEnum future{"future-value"};
        result.expectTrue(future.value == "future-value" && !future.isKnown(),
                          std::string(label) + " retains a future string value without closing the enum");
    }

    void testOpenStringEnums(tests::support::TestResult& result) {
        expectKnown(result, typed::ChatgptAuthTokensRefreshReason::unauthorized(), "unauthorized", "refresh reason unauthorized");

        expectKnown(result, typed::AddCreditsNudgeCreditType::credits(), "credits", "nudge credit type credits");
        expectKnown(result, typed::AddCreditsNudgeCreditType::usageLimit(), "usage_limit", "nudge credit type usage limit");

        expectKnown(result, typed::AddCreditsNudgeEmailStatus::sent(), "sent", "nudge status sent");
        expectKnown(result, typed::AddCreditsNudgeEmailStatus::cooldownActive(), "cooldown_active", "nudge status cooldown");

        expectKnown(result, typed::AmazonBedrockCredentialSource::codexManaged(), "codexManaged", "Bedrock Codex credential");
        expectKnown(result, typed::AmazonBedrockCredentialSource::awsManaged(), "awsManaged", "Bedrock AWS credential");

        expectKnown(result, typed::AuthMode::apiKey(), "apikey", "auth mode API key");
        expectKnown(result, typed::AuthMode::chatgpt(), "chatgpt", "auth mode ChatGPT");
        expectKnown(result, typed::AuthMode::chatgptAuthTokens(), "chatgptAuthTokens", "auth mode ChatGPT tokens");
        expectKnown(result, typed::AuthMode::headers(), "headers", "auth mode headers");
        expectKnown(result, typed::AuthMode::agentIdentity(), "agentIdentity", "auth mode agent identity");
        expectKnown(result, typed::AuthMode::personalAccessToken(), "personalAccessToken", "auth mode personal token");
        expectKnown(result, typed::AuthMode::bedrockApiKey(), "bedrockApiKey", "auth mode Bedrock API key");

        expectKnown(result, typed::CancelLoginAccountStatus::canceled(), "canceled", "cancel status canceled");
        expectKnown(result, typed::CancelLoginAccountStatus::notFound(), "notFound", "cancel status not found");

        expectKnown(result, typed::ConsumeAccountRateLimitResetCreditOutcome::reset(), "reset", "credit outcome reset");
        expectKnown(result,
                    typed::ConsumeAccountRateLimitResetCreditOutcome::nothingToReset(),
                    "nothingToReset",
                    "credit outcome nothing to reset");
        expectKnown(result, typed::ConsumeAccountRateLimitResetCreditOutcome::noCredit(), "noCredit", "credit outcome no credit");
        expectKnown(result,
                    typed::ConsumeAccountRateLimitResetCreditOutcome::alreadyRedeemed(),
                    "alreadyRedeemed",
                    "credit outcome redeemed");

        expectKnown(result, typed::LoginAppBrand::codex(), "codex", "login brand Codex");
        expectKnown(result, typed::LoginAppBrand::chatgpt(), "chatgpt", "login brand ChatGPT");

        expectKnown(result, typed::PlanType::free(), "free", "plan free");
        expectKnown(result, typed::PlanType::go(), "go", "plan go");
        expectKnown(result, typed::PlanType::plus(), "plus", "plan plus");
        expectKnown(result, typed::PlanType::pro(), "pro", "plan pro");
        expectKnown(result, typed::PlanType::proLite(), "prolite", "plan pro-lite");
        expectKnown(result, typed::PlanType::team(), "team", "plan team");
        expectKnown(result,
                    typed::PlanType::selfServeBusinessUsageBased(),
                    "self_serve_business_usage_based",
                    "plan self-serve business");
        expectKnown(result, typed::PlanType::business(), "business", "plan business");
        expectKnown(result,
                    typed::PlanType::enterpriseCbpUsageBased(),
                    "enterprise_cbp_usage_based",
                    "plan enterprise CBP");
        expectKnown(result, typed::PlanType::enterprise(), "enterprise", "plan enterprise");
        expectKnown(result, typed::PlanType::edu(), "edu", "plan education");
        expectKnown(result, typed::PlanType::unknown(), "unknown", "plan protocol-known unknown");

        expectKnown(result,
                    typed::RateLimitReachedType::rateLimitReached(),
                    "rate_limit_reached",
                    "rate-limit reached reason");
        expectKnown(result,
                    typed::RateLimitReachedType::workspaceOwnerCreditsDepleted(),
                    "workspace_owner_credits_depleted",
                    "owner credits reason");
        expectKnown(result,
                    typed::RateLimitReachedType::workspaceMemberCreditsDepleted(),
                    "workspace_member_credits_depleted",
                    "member credits reason");
        expectKnown(result,
                    typed::RateLimitReachedType::workspaceOwnerUsageLimitReached(),
                    "workspace_owner_usage_limit_reached",
                    "owner usage reason");
        expectKnown(result,
                    typed::RateLimitReachedType::workspaceMemberUsageLimitReached(),
                    "workspace_member_usage_limit_reached",
                    "member usage reason");

        expectKnown(result, typed::RateLimitResetCreditStatus::available(), "available", "credit status available");
        expectKnown(result, typed::RateLimitResetCreditStatus::redeeming(), "redeeming", "credit status redeeming");
        expectKnown(result, typed::RateLimitResetCreditStatus::redeemed(), "redeemed", "credit status redeemed");
        expectKnown(result, typed::RateLimitResetCreditStatus::unknown(), "unknown", "credit status protocol-known unknown");
        expectKnown(result, typed::RateLimitResetType::codexRateLimits(), "codexRateLimits", "reset type Codex");
        expectKnown(result, typed::RateLimitResetType::unknown(), "unknown", "reset type protocol-known unknown");

        expectKnown(result, typed::WorkspaceMessageType::headline(), "headline", "workspace message headline");
        expectKnown(result, typed::WorkspaceMessageType::announcement(), "announcement", "workspace message announcement");
        expectKnown(result, typed::WorkspaceMessageType::unknown(), "unknown", "workspace message protocol-known unknown");

        expectFuture<typed::ChatgptAuthTokensRefreshReason>(result, "refresh reason");
        expectFuture<typed::AddCreditsNudgeCreditType>(result, "nudge credit type");
        expectFuture<typed::AddCreditsNudgeEmailStatus>(result, "nudge status");
        expectFuture<typed::AmazonBedrockCredentialSource>(result, "Bedrock credential source");
        expectFuture<typed::AuthMode>(result, "auth mode");
        expectFuture<typed::CancelLoginAccountStatus>(result, "cancel status");
        expectFuture<typed::ConsumeAccountRateLimitResetCreditOutcome>(result, "credit outcome");
        expectFuture<typed::LoginAppBrand>(result, "login app brand");
        expectFuture<typed::PlanType>(result, "plan type");
        expectFuture<typed::RateLimitReachedType>(result, "rate-limit reached type");
        expectFuture<typed::RateLimitResetCreditStatus>(result, "credit status");
        expectFuture<typed::RateLimitResetType>(result, "reset type");
        expectFuture<typed::WorkspaceMessageType>(result, "workspace message type");
    }

    void testRequestEncoding(tests::support::TestResult& result) {
        std::string error = "stale";

        const auto cancel = detail::encodeCancelLoginAccountParams({typed::LoginId{"login-1"}}, error);
        result.expectTrue(cancel == codex::Json{{"loginId", "login-1"}} && error.empty(),
                          "cancel-login parameters encode the exact strong login ID");

        const typed::LoginAccountParams apiKey = typed::ApiKeyLoginAccountParams{SyntheticApiKey};
        const auto encodedApiKey = detail::encodeLoginAccountParams(apiKey, error);
        result.expectTrue(encodedApiKey == codex::Json{{"type", "apiKey"}, {"apiKey", SyntheticApiKey}} && error.empty(),
                          "API-key login encodes its distinct exact branch");

        typed::ChatgptLoginAccountParams chatgpt;
        chatgpt.appBrand = typed::OptionalNullable<typed::LoginAppBrand>::explicitNull();
        chatgpt.codexStreamlinedLogin = false;
        chatgpt.useHostedLoginSuccessPage = true;
        const auto encodedChatgpt = detail::encodeLoginAccountParams(typed::LoginAccountParams{chatgpt}, error);
        result.expectTrue(encodedChatgpt ==
                                  codex::Json{{"type", "chatgpt"},
                                              {"appBrand", nullptr},
                                              {"codexStreamlinedLogin", false},
                                              {"useHostedLoginSuccessPage", true}},
                          "ChatGPT login preserves explicit null and present false Boolean values");

        chatgpt.appBrand = typed::OptionalNullable<typed::LoginAppBrand>::omitted();
        chatgpt.codexStreamlinedLogin.reset();
        chatgpt.useHostedLoginSuccessPage.reset();
        const auto encodedChatgptOmitted = detail::encodeLoginAccountParams(typed::LoginAccountParams{chatgpt}, error);
        result.expectTrue(encodedChatgptOmitted == codex::Json{{"type", "chatgpt"}},
                          "ChatGPT login omits all omitted optional fields");

        chatgpt.appBrand = typed::LoginAppBrand::codex();
        const auto encodedChatgptValue = detail::encodeLoginAccountParams(typed::LoginAccountParams{chatgpt}, error);
        result.expectTrue(encodedChatgptValue == codex::Json{{"type", "chatgpt"}, {"appBrand", "codex"}},
                          "ChatGPT login encodes the OptionalNullable value state");

        const auto encodedDevice =
            detail::encodeLoginAccountParams(typed::LoginAccountParams{typed::ChatgptDeviceCodeLoginAccountParams{}}, error);
        result.expectTrue(encodedDevice == codex::Json{{"type", "chatgptDeviceCode"}},
                          "device-code login encodes its exact empty branch");

        typed::ChatgptAuthTokensLoginAccountParams tokens{
            SyntheticAccessToken,
            typed::AccountId{SyntheticAccountId},
            typed::OptionalNullable<typed::PlanType>::explicitNull(),
        };
        const auto encodedTokens = detail::encodeLoginAccountParams(typed::LoginAccountParams{tokens}, error);
        result.expectTrue(encodedTokens ==
                                  codex::Json{{"type", "chatgptAuthTokens"},
                                              {"accessToken", SyntheticAccessToken},
                                              {"chatgptAccountId", SyntheticAccountId},
                                              {"chatgptPlanType", nullptr}},
                          "host-token login encodes required credentials and explicit-null plan exactly");

        tokens.chatgptPlanType = typed::OptionalNullable<typed::PlanType>::omitted();
        const auto encodedTokensOmitted = detail::encodeLoginAccountParams(typed::LoginAccountParams{tokens}, error);
        result.expectTrue(encodedTokensOmitted && !encodedTokensOmitted->contains("chatgptPlanType"),
                          "host-token login distinguishes an omitted plan from explicit null");
        tokens.chatgptPlanType = typed::PlanType::plus();
        const auto encodedTokensValue = detail::encodeLoginAccountParams(typed::LoginAccountParams{tokens}, error);
        result.expectTrue(encodedTokensValue && encodedTokensValue->value("chatgptPlanType", "") == "plus",
                          "host-token login encodes a plan value without collapsing it into null");

        error.clear();
        const auto future = detail::encodeLoginAccountParams(
            typed::LoginAccountParams{typed::UnknownLoginAccountParams{std::string("future-login")}}, error);
        result.expectTrue(!future && error == "LoginAccountParams future discriminator cannot be encoded" &&
                              error.find(SyntheticApiKey) == std::string::npos &&
                              error.find(SyntheticAccessToken) == std::string::npos,
                          "future outgoing login alternatives reject synchronously without credential disclosure");

        typed::ConsumeAccountRateLimitResetCreditParams consume{
            typed::OptionalNullable<typed::RateLimitResetCreditId>::explicitNull(),
            typed::IdempotencyKey{"idempotency-1"},
        };
        const auto encodedConsume = detail::encodeConsumeAccountRateLimitResetCreditParams(consume, error);
        result.expectTrue(encodedConsume == codex::Json{{"creditId", nullptr}, {"idempotencyKey", "idempotency-1"}},
                          "reset-credit consume preserves explicit null");
        consume.creditId = typed::OptionalNullable<typed::RateLimitResetCreditId>::omitted();
        const auto encodedConsumeOmitted = detail::encodeConsumeAccountRateLimitResetCreditParams(consume, error);
        result.expectTrue(encodedConsumeOmitted == codex::Json{{"idempotencyKey", "idempotency-1"}},
                          "reset-credit consume omits an omitted credit ID");
        consume.creditId = typed::RateLimitResetCreditId{"credit-1"};
        const auto encodedConsumeValue = detail::encodeConsumeAccountRateLimitResetCreditParams(consume, error);
        result.expectTrue(encodedConsumeValue ==
                                  codex::Json{{"creditId", "credit-1"}, {"idempotencyKey", "idempotency-1"}},
                          "reset-credit consume encodes its credit-ID value");

        const auto readOmitted = detail::encodeGetAccountParams({}, error);
        const auto readFalse = detail::encodeGetAccountParams({false}, error);
        result.expectTrue(readOmitted == codex::Json::object() && readFalse == codex::Json{{"refreshToken", false}},
                          "account/read distinguishes an omitted refresh flag from present false");

        const auto nudge =
            detail::encodeSendAddCreditsNudgeEmailParams({typed::AddCreditsNudgeCreditType::usageLimit()}, error);
        result.expectTrue(nudge == codex::Json{{"creditType", "usage_limit"}},
                          "add-credits nudge encodes its exact open string value");
    }

    void testAccountUnion(tests::support::TestResult& result) {
        const codex::Json apiKeyRaw{{"type", "apiKey"}, {"future", true}};
        const auto apiKey = detail::decodeAccount(apiKeyRaw);
        result.expectTrue(apiKey.value && std::holds_alternative<typed::ApiKeyAccount>(*apiKey.value) &&
                              typed::accountRaw(*apiKey.value) == apiKeyRaw && !apiKey.diagnostic,
                          "Account apiKey decodes as a distinct typed branch with exact raw retention");

        const codex::Json chatgptRaw{{"type", "chatgpt"}, {"email", nullptr}, {"planType", "future-plan"}};
        const auto chatgpt = detail::decodeAccount(chatgptRaw);
        const auto* chatgptValue = chatgpt.value ? std::get_if<typed::ChatgptAccount>(&*chatgpt.value) : nullptr;
        result.expectTrue(chatgptValue && !chatgptValue->email && chatgptValue->planType.value == "future-plan" &&
                              chatgptValue->raw == chatgptRaw &&
                              hasDiagnostic(chatgptValue->diagnostics,
                                            typed::DecodeIssueKind::UnknownEnumValue,
                                            typed::DecodeIssueSeverity::ForwardCompatibility,
                                            "$.planType") &&
                              hasDiagnostic(chatgpt.diagnostic,
                                            typed::DecodeIssueKind::UnknownEnumValue,
                                            typed::DecodeIssueSeverity::ForwardCompatibility,
                                            "$.planType"),
                          "a future nested plan retains the known ChatGPT account branch and emits ForwardCompatibility");

        const codex::Json bedrockRaw{{"type", "amazonBedrock"}};
        const auto bedrock = detail::decodeAccount(bedrockRaw);
        const auto* bedrockValue = bedrock.value ? std::get_if<typed::AmazonBedrockAccount>(&*bedrock.value) : nullptr;
        result.expectTrue(bedrockValue && bedrockValue->credentialSource == typed::AmazonBedrockCredentialSource::awsManaged() &&
                              bedrockValue->raw == bedrockRaw,
                          "Amazon Bedrock account applies the pinned default while retaining the exact source JSON");

        const codex::Json futureRaw{{"type", "futureAccount"}, {"credential", "opaque-future"}};
        const auto future = detail::decodeAccount(futureRaw);
        const auto* unknown = future.value ? std::get_if<typed::UnknownAccount>(&*future.value) : nullptr;
        result.expectTrue(unknown && unknown->type == "futureAccount" && unknown->raw == futureRaw &&
                              hasDiagnostic(unknown->diagnostic,
                                            typed::DecodeIssueKind::UnknownDiscriminator,
                                            typed::DecodeIssueSeverity::ForwardCompatibility,
                                            "$.type") &&
                              hasDiagnostic(future.diagnostic,
                                            typed::DecodeIssueKind::UnknownDiscriminator,
                                            typed::DecodeIssueSeverity::ForwardCompatibility,
                                            "$.type"),
                          "future Account discriminator becomes an explicit raw-retaining unknown alternative");

        const std::vector<std::pair<codex::Json, std::string>> malformedAccounts{
            {codex::Json{{"type", "chatgpt"}, {"email", "person@example.invalid"}}, "$.planType"},
            {codex::Json{{"type", 1}}, "$.type"},
            {codex::Json{{"email", nullptr}, {"planType", "plus"}}, "$.type"},
            {codex::Json::array({1}), "$"},
        };
        for (const auto& [malformed, expectedPath] : malformedAccounts) {
            const auto decoded = detail::decodeAccount(malformed);
            result.expectTrue(!decoded.value &&
                                  hasDiagnostic(decoded.diagnostic,
                                                typed::DecodeIssueKind::MalformedKnownPayload,
                                                typed::DecodeIssueSeverity::ProtocolWarning,
                                                expectedPath),
                              "malformed or discriminator-less Account is never misclassified as a future branch");
        }
    }

    void testLoginResponseUnion(tests::support::TestResult& result) {
        const std::vector<std::pair<codex::Json, std::size_t>> known{
            {codex::Json{{"type", "apiKey"}, {"future", 1}}, 0},
            {codex::Json{{"type", "chatgpt"}, {"authUrl", "https://example.invalid/auth"}, {"loginId", "login-chatgpt"}}, 1},
            {codex::Json{{"type", "chatgptDeviceCode"},
                         {"loginId", "login-device"},
                         {"userCode", SyntheticDeviceCode},
                         {"verificationUrl", "https://example.invalid/device"}},
             2},
            {codex::Json{{"type", "chatgptAuthTokens"}, {"future", true}}, 3},
        };
        for (const auto& [wire, expectedIndex] : known) {
            const auto decoded = detail::decodeLoginAccountResponseUnion(wire);
            result.expectTrue(decoded.value && decoded.value->index() == expectedIndex &&
                                  typed::loginAccountResponseRaw(*decoded.value) == wire && !decoded.diagnostic,
                              "each known LoginAccountResponse branch decodes distinctly with exact raw retention");
        }

        const codex::Json futureRaw{{"type", "futureLoginResult"}, {"opaque", codex::Json::array({1, 2})}};
        const auto future = detail::decodeLoginAccountResponseUnion(futureRaw);
        const auto* unknown = future.value ? std::get_if<typed::UnknownLoginAccountResponse>(&*future.value) : nullptr;
        result.expectTrue(unknown && unknown->type == "futureLoginResult" && unknown->raw == futureRaw &&
                              hasDiagnostic(unknown->diagnostic,
                                            typed::DecodeIssueKind::UnknownDiscriminator,
                                            typed::DecodeIssueSeverity::ForwardCompatibility,
                                            "$.type"),
                          "future LoginAccountResponse discriminator becomes its direction-specific unknown alternative");

        const codex::Json secretBearingMalformed{
            {"type", "chatgptDeviceCode"},
            {"loginId", "login-device"},
            {"userCode", SyntheticDeviceCode},
        };
        const auto malformed = detail::decodeLoginAccountResponseUnion(secretBearingMalformed);
        result.expectTrue(!malformed.value &&
                              hasDiagnostic(malformed.diagnostic,
                                            typed::DecodeIssueKind::MalformedKnownPayload,
                                            typed::DecodeIssueSeverity::ProtocolWarning,
                                            "$.verificationUrl") &&
                              malformed.diagnostic && diagnosticDoesNotContain(*malformed.diagnostic, SyntheticDeviceCode),
                          "malformed known device login reports a stable code/path without the ephemeral code value");

        for (const codex::Json& wrongDiscriminator :
             {codex::Json{{"type", false}}, codex::Json{{"authUrl", "https://example.invalid"}}}) {
            const auto decoded = detail::decodeLoginAccountResponseUnion(wrongDiscriminator);
            result.expectTrue(!decoded.value &&
                                  hasDiagnostic(decoded.diagnostic,
                                                typed::DecodeIssueKind::MalformedKnownPayload,
                                                typed::DecodeIssueSeverity::ProtocolWarning,
                                                "$.type"),
                              "missing and wrong-type login response discriminators are malformed known payloads");
        }
    }

    codex::Json completeRateLimitSnapshot() {
        return {
            {"credits", {{"balance", "10.50"}, {"hasCredits", true}, {"unlimited", false}}},
            {"individualLimit", {{"limit", "20.00"}, {"remainingPercent", 50}, {"resetsAt", 2'000}, {"used", "10.00"}}},
            {"limitId", "primary"},
            {"limitName", nullptr},
            {"planType", "future-plan"},
            {"primary", {{"resetsAt", nullptr}, {"usedPercent", 25}, {"windowDurationMins", 60}}},
            {"rateLimitReachedType", "future-reached-reason"},
            {"secondary", nullptr},
        };
    }

    void testSuccessfulResultDecoding(tests::support::TestResult& result) {
        std::string error;

        const codex::Json cancelRaw{{"status", "future-cancel-status"}};
        const auto cancel = detail::decodeCancelLoginAccountResponse(cancelRaw, error);
        result.expectTrue(cancel && cancel->raw == cancelRaw && cancel->status.value == "future-cancel-status" &&
                              hasDiagnostic(cancel->diagnostics,
                                            typed::DecodeIssueKind::UnknownEnumValue,
                                            typed::DecodeIssueSeverity::ForwardCompatibility,
                                            "$.status"),
                          "cancel-login retains a future open status and exact raw result");

        const codex::Json loginRaw{
            {"type", "chatgpt"},
            {"authUrl", "https://example.invalid/login"},
            {"loginId", "login-result"},
            {"future", true},
        };
        const auto login = detail::decodeLoginAccountResponse(loginRaw, error);
        result.expectTrue(login && std::holds_alternative<typed::ChatgptLoginAccountResponse>(*login) &&
                              typed::loginAccountResponseRaw(*login) == loginRaw,
                          "login/start successful result uses the full union decoder and retains raw JSON");

        const codex::Json consumeRaw{{"outcome", "future-outcome"}};
        const auto consume = detail::decodeConsumeAccountRateLimitResetCreditResponse(consumeRaw, error);
        result.expectTrue(consume && consume->raw == consumeRaw && consume->outcome.value == "future-outcome" &&
                              hasDiagnostic(consume->diagnostics,
                                            typed::DecodeIssueKind::UnknownEnumValue,
                                            typed::DecodeIssueSeverity::ForwardCompatibility,
                                            "$.outcome"),
                          "credit consumption retains a future outcome within the known response");

        const codex::Json rateLimitRaw{
            {"rateLimitResetCredits",
             {{"availableCount", 1},
              {"credits",
               codex::Json::array({codex::Json{{"description", nullptr},
                                               {"expiresAt", std::numeric_limits<std::int64_t>::max()},
                                               {"grantedAt", std::numeric_limits<std::int64_t>::min()},
                                               {"id", "credit-1"},
                                               {"resetType", "future-reset"},
                                               {"status", "future-status"},
                                               {"title", "Synthetic credit"}}})}}},
            {"rateLimits", completeRateLimitSnapshot()},
            {"rateLimitsByLimitId", {{"primary", completeRateLimitSnapshot()}}},
        };
        const auto rateLimits = detail::decodeGetAccountRateLimitsResponse(rateLimitRaw, error);
        result.expectTrue(rateLimits && rateLimits->raw == rateLimitRaw && rateLimits->rateLimits.planType.hasValue() &&
                              rateLimits->rateLimits.planType->value == "future-plan" &&
                              rateLimits->rateLimits.limitName.isNull() &&
                              rateLimits->rateLimitResetCredits.hasValue() &&
                              rateLimits->rateLimitsByLimitId.hasValue() &&
                              rateLimits->rateLimitsByLimitId->contains(typed::RateLimitId{"primary"}) &&
                              hasDiagnostic(rateLimits->diagnostics,
                                            typed::DecodeIssueKind::UnknownEnumValue,
                                            typed::DecodeIssueSeverity::ForwardCompatibility,
                                            "$.rateLimits.planType"),
                          "rate-limit result decodes nested maps, monetary strings, bounds, tri-state values, and future enums");

        const codex::Json accountRaw{
            {"account", {{"type", "futureAccount"}, {"futureField", true}}},
            {"requiresOpenaiAuth", false},
        };
        const auto account = detail::decodeGetAccountResponse(accountRaw, error);
        const auto* unknownAccount =
            account && account->account.hasValue() ? std::get_if<typed::UnknownAccount>(&*account->account) : nullptr;
        result.expectTrue(account && account->raw == accountRaw && !account->requiresOpenaiAuth && unknownAccount &&
                              unknownAccount->raw == accountRaw.at("account") &&
                              hasDiagnostic(account->diagnostics,
                                            typed::DecodeIssueKind::UnknownDiscriminator,
                                            typed::DecodeIssueSeverity::ForwardCompatibility,
                                            "$.account.type"),
                          "account/read keeps a future Account alternative inside the known response");

        const auto accountWithFuturePlan = detail::decodeGetAccountResponse(
            {{"account", {{"type", "chatgpt"}, {"email", nullptr}, {"planType", "future-plan"}}},
             {"requiresOpenaiAuth", false}},
            error);
        result.expectTrue(accountWithFuturePlan &&
                              hasDiagnostic(accountWithFuturePlan->diagnostics,
                                            typed::DecodeIssueKind::UnknownEnumValue,
                                            typed::DecodeIssueSeverity::ForwardCompatibility,
                                            "$.account.planType"),
                          "account/read prefixes a known nested Account branch's future-enum diagnostic path");

        const auto accountOmitted = detail::decodeGetAccountResponse({{"requiresOpenaiAuth", true}}, error);
        const auto accountNull =
            detail::decodeGetAccountResponse({{"account", nullptr}, {"requiresOpenaiAuth", true}}, error);
        result.expectTrue(accountOmitted && accountOmitted->account.isOmitted() && accountNull && accountNull->account.isNull(),
                          "account/read preserves omitted and explicit-null Account states");

        const codex::Json nudgeRaw{{"status", "future-nudge-status"}};
        const auto nudge = detail::decodeSendAddCreditsNudgeEmailResponse(nudgeRaw, error);
        result.expectTrue(nudge && nudge->raw == nudgeRaw && nudge->status.value == "future-nudge-status" &&
                              hasDiagnostic(nudge->diagnostics,
                                            typed::DecodeIssueKind::UnknownEnumValue,
                                            typed::DecodeIssueSeverity::ForwardCompatibility,
                                            "$.status"),
                          "nudge result retains an upstream-added status as an open enum");

        const codex::Json usageRaw{
            {"dailyUsageBuckets",
             codex::Json::array(
                 {codex::Json{{"startDate", "2026-07-24"}, {"tokens", std::numeric_limits<std::int64_t>::max()}}})},
            {"summary",
             {{"currentStreakDays", nullptr},
              {"lifetimeTokens", std::numeric_limits<std::int64_t>::max()},
              {"longestRunningTurnSec", 10},
              {"longestStreakDays", 4},
              {"peakDailyTokens", std::numeric_limits<std::int64_t>::min()}}},
        };
        const auto usage = detail::decodeGetAccountTokenUsageResponse(usageRaw, error);
        result.expectTrue(usage && usage->raw == usageRaw && usage->dailyUsageBuckets.hasValue() &&
                              usage->dailyUsageBuckets->size() == 1 && usage->summary.currentStreakDays.isNull() &&
                              usage->summary.lifetimeTokens.hasValue() &&
                              *usage->summary.lifetimeTokens == std::numeric_limits<std::int64_t>::max(),
                          "usage result preserves signed 64-bit boundaries and all nullable states");

        const codex::Json workspaceRaw{
            {"featureEnabled", true},
            {"messages",
             codex::Json::array({codex::Json{{"archivedAt", nullptr},
                                             {"createdAt", 1},
                                             {"messageBody", "Synthetic message"},
                                             {"messageId", "message-1"},
                                             {"messageType", "future-message-type"}}})},
        };
        const auto workspace = detail::decodeGetWorkspaceMessagesResponse(workspaceRaw, error);
        result.expectTrue(workspace && workspace->raw == workspaceRaw && workspace->messages.size() == 1 &&
                              workspace->messages.front().archivedAt.isNull() &&
                              workspace->messages.front().messageType.value == "future-message-type" &&
                              hasDiagnostic(workspace->diagnostics,
                                            typed::DecodeIssueKind::UnknownEnumValue,
                                            typed::DecodeIssueSeverity::ForwardCompatibility,
                                            "$.messages[0].messageType"),
                          "workspace messages preserve ordered objects, tri-state timestamps, raw JSON, and future types");

        error.clear();
        const auto fractionalUsage = detail::decodeGetAccountTokenUsageResponse(
            {{"dailyUsageBuckets",
              codex::Json::array({codex::Json{{"startDate", "2026-07-24"}, {"tokens", 1.5}}})},
             {"summary", codex::Json::object()}},
            error);
        result.expectTrue(!fractionalUsage && error.find("$.") != std::string::npos,
                          "fractional usage integers are rejected with a stable field path");
    }

    codex::Notification makeNotification(std::string method, codex::Json params, codex::Json futureEnvelopeValue) {
        codex::Json raw{
            {"jsonrpc", "2.0"},
            {"method", method},
            {"params", params},
            {"futureEnvelopeField", std::move(futureEnvelopeValue)},
        };
        return {std::move(method), std::move(params), std::move(raw)};
    }

    void testNotificationDecoding(tests::support::TestResult& result) {
        std::string error;
        const codex::Notification login =
            makeNotification("account/login/completed",
                             {{"error", nullptr}, {"loginId", "login-completed"}, {"success", true}},
                             "login-envelope");
        const auto decodedLogin = detail::decodeAccountLoginCompletedNotification(login, error);
        result.expectTrue(decodedLogin && decodedLogin->success && decodedLogin->error.isNull() &&
                              decodedLogin->loginId.hasValue() && decodedLogin->loginId->value == "login-completed" &&
                              decodedLogin->raw == login.raw,
                          "login completion preserves sparse tri-state fields and the complete envelope");

        const codex::Notification rateLimits =
            makeNotification("account/rateLimits/updated",
                             {{"rateLimits", completeRateLimitSnapshot()}},
                             "rate-envelope");
        const auto decodedRateLimits = detail::decodeAccountRateLimitsUpdatedNotification(rateLimits, error);
        result.expectTrue(decodedRateLimits && decodedRateLimits->raw == rateLimits.raw &&
                              decodedRateLimits->rateLimits.secondary.isNull() &&
                              decodedRateLimits->rateLimits.planType.hasValue() &&
                              hasDiagnostic(decodedRateLimits->diagnostics,
                                            typed::DecodeIssueKind::UnknownEnumValue,
                                            typed::DecodeIssueSeverity::ForwardCompatibility,
                                            "$.params.rateLimits.planType"),
                          "rate-limit update retains its exact sparse shape, full envelope, and future nested enum");

        const codex::Notification updated =
            makeNotification("account/updated",
                             {{"authMode", "future-auth-mode"}, {"planType", nullptr}},
                             "updated-envelope");
        const auto decodedUpdated = detail::decodeAccountUpdatedNotification(updated, error);
        result.expectTrue(decodedUpdated && decodedUpdated->raw == updated.raw &&
                              decodedUpdated->authMode.hasValue() && decodedUpdated->authMode->value == "future-auth-mode" &&
                              decodedUpdated->planType.isNull() &&
                              hasDiagnostic(decodedUpdated->diagnostics,
                                            typed::DecodeIssueKind::UnknownEnumValue,
                                            typed::DecodeIssueSeverity::ForwardCompatibility,
                                            "$.params.authMode"),
                          "account update keeps the known notification with future auth mode and explicit-null plan");

        const auto malformed = detail::decodeAccountLoginCompletedNotification(
            makeNotification("account/login/completed",
                             {{"error", "synthetic login failure"}, {"loginId", SyntheticDeviceCode}, {"success", "yes"}},
                             "malformed-envelope"),
            error);
        result.expectTrue(!malformed && error.find("$.params.success") != std::string::npos &&
                              error.find(SyntheticDeviceCode) == std::string::npos,
                          "malformed account notification reports only its stable field path, never a sensitive value");
    }

    void testAuthRefreshCodec(tests::support::TestResult& result) {
        std::string error;

        const auto omitted = detail::decodeChatgptAuthTokensRefreshParams({{"reason", "unauthorized"}}, error);
        const auto explicitNull =
            detail::decodeChatgptAuthTokensRefreshParams({{"previousAccountId", nullptr}, {"reason", "unauthorized"}}, error);
        const codex::Json valueRaw{{"previousAccountId", SyntheticAccountId}, {"reason", "future-reason"}, {"future", true}};
        const auto value = detail::decodeChatgptAuthTokensRefreshParams(valueRaw, error);
        result.expectTrue(omitted && omitted->previousAccountId.isOmitted() &&
                              explicitNull && explicitNull->previousAccountId.isNull() &&
                              value && value->previousAccountId.hasValue() &&
                              value->previousAccountId->value == SyntheticAccountId && value->reason.value == "future-reason" &&
                              value->raw == valueRaw &&
                              hasDiagnostic(value->diagnostics,
                                            typed::DecodeIssueKind::UnknownEnumValue,
                                            typed::DecodeIssueSeverity::ForwardCompatibility,
                                            "$.params.reason"),
                          "auth refresh request preserves omitted/null/value account identity and a future open reason");

        const auto missing = detail::decodeChatgptAuthTokensRefreshParams(
            {{"previousAccountId", SyntheticAccountId}, {"futureSecret", SyntheticAccessToken}}, error);
        result.expectTrue(!missing && error.find("$.params.reason") != std::string::npos &&
                              error.find(SyntheticAccountId) == std::string::npos &&
                              error.find(SyntheticAccessToken) == std::string::npos,
                          "malformed auth refresh diagnostics contain a path but no identity or credential value");

        const codex::Json malformedParams{
            {"previousAccountId", SyntheticAccountId},
            {"futureSecret", SyntheticAccessToken},
        };
        const codex::Json malformedEnvelope{
            {"id", "malformed-auth"},
            {"method", "account/chatgptAuthTokens/refresh"},
            {"params", malformedParams},
            {"futureEnvelope", true},
        };
        const typed::TypedServerRequest malformedRequest = detail::decodeServerRequest(
            codex::ServerRequest{
                codex::ServerRequestId{std::string("malformed-auth")},
                "account/chatgptAuthTokens/refresh",
                malformedParams,
                malformedEnvelope,
                codex::ServerRequestToken{71},
            });
        const auto* unknown = std::get_if<typed::UnknownServerRequest>(&malformedRequest);
        result.expectTrue(unknown && unknown->params == malformedParams && unknown->raw == malformedEnvelope &&
                              unknown->decodingError &&
                              unknown->decodingError->find("$.params.reason") != std::string::npos &&
                              unknown->decodingError->find(SyntheticAccountId) == std::string::npos &&
                              unknown->decodingError->find(SyntheticAccessToken) == std::string::npos &&
                              unknown->diagnostic &&
                              unknown->diagnostic->kind == typed::DecodeIssueKind::MalformedKnownPayload &&
                              unknown->diagnostic->severity == typed::DecodeIssueSeverity::ProtocolWarning &&
                              unknown->diagnostic->fieldPath == "$.params.reason" &&
                              diagnosticDoesNotContain(*unknown->diagnostic, SyntheticAccountId) &&
                              diagnosticDoesNotContain(*unknown->diagnostic, SyntheticAccessToken),
                          "malformed auth server request remains answerable and raw-retained with value-free structured diagnostics");

        typed::ChatgptAuthTokensRefreshResponse response{
            SyntheticAccessToken,
            typed::AccountId{SyntheticAccountId},
            typed::OptionalNullable<typed::PlanType>::omitted(),
        };
        const auto encodedOmitted = detail::encodeChatgptAuthTokensRefreshResponse(response, error);
        result.expectTrue(encodedOmitted && encodedOmitted->size() == 2 && !encodedOmitted->contains("chatgptPlanType"),
                          "auth refresh response omits an omitted plan while retaining both required fields");
        response.chatgptPlanType = typed::OptionalNullable<typed::PlanType>::explicitNull();
        const auto encodedNull = detail::encodeChatgptAuthTokensRefreshResponse(response, error);
        result.expectTrue(encodedNull && encodedNull->contains("chatgptPlanType") &&
                              encodedNull->at("chatgptPlanType").is_null(),
                          "auth refresh response preserves an explicit-null plan");
        response.chatgptPlanType = typed::PlanType::plus();
        const auto encodedValue = detail::encodeChatgptAuthTokensRefreshResponse(response, error);
        result.expectTrue(encodedValue && encodedValue->value("chatgptPlanType", "") == "plus" && error.empty(),
                          "auth refresh response encodes an open plan value");
    }
} // namespace

int main() {
    static_assert(!std::is_same_v<typed::UnknownLoginAccountParams, typed::UnknownLoginAccountResponse>);
    static_assert(!std::is_same_v<typed::LoginAccountParams, typed::LoginAccountResponse>);
    static_assert(std::is_same_v<decltype(typed::ChatgptAuthTokensRefreshParams::previousAccountId),
                                 typed::OptionalNullable<typed::AccountId>>);

    tests::support::TestResult result;
    testOpenStringEnums(result);
    testRequestEncoding(result);
    testAccountUnion(result);
    testLoginResponseUnion(result);
    testSuccessfulResultDecoding(result);
    testNotificationDecoding(result);
    testAuthRefreshCodec(result);
    return result.processResult();
}
