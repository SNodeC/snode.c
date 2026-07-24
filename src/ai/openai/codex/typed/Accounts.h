/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#ifndef AI_OPENAI_CODEX_TYPED_ACCOUNTS_H
#define AI_OPENAI_CODEX_TYPED_ACCOUNTS_H

#include "ai/openai/codex/AppServerClient.h"
#include "ai/openai/codex/Protocol.h"
#include "ai/openai/codex/typed/Results.h"
#include "ai/openai/codex/typed/Types.h"

#include <compare>
#include <cstdint>
#include <functional>
#include <map>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>

namespace ai::openai::codex::typed {

    struct AccountId {
        std::string value;

        auto operator<=>(const AccountId&) const = default;
    };

    struct LoginId {
        std::string value;

        auto operator<=>(const LoginId&) const = default;
    };

    struct RateLimitResetCreditId {
        std::string value;

        auto operator<=>(const RateLimitResetCreditId&) const = default;
    };

    struct IdempotencyKey {
        std::string value;

        auto operator<=>(const IdempotencyKey&) const = default;
    };

    struct RateLimitId {
        std::string value;

        auto operator<=>(const RateLimitId&) const = default;
    };

    struct WorkspaceMessageId {
        std::string value;

        auto operator<=>(const WorkspaceMessageId&) const = default;
    };

    struct ChatgptAuthTokensRefreshReason {
        std::string value;

        static ChatgptAuthTokensRefreshReason unauthorized();
        [[nodiscard]] bool isKnown() const noexcept;

        auto operator<=>(const ChatgptAuthTokensRefreshReason&) const = default;
    };

    struct AddCreditsNudgeCreditType {
        std::string value;

        static AddCreditsNudgeCreditType credits();
        static AddCreditsNudgeCreditType usageLimit();
        [[nodiscard]] bool isKnown() const noexcept;

        auto operator<=>(const AddCreditsNudgeCreditType&) const = default;
    };

    struct AddCreditsNudgeEmailStatus {
        std::string value;

        static AddCreditsNudgeEmailStatus sent();
        static AddCreditsNudgeEmailStatus cooldownActive();
        [[nodiscard]] bool isKnown() const noexcept;

        auto operator<=>(const AddCreditsNudgeEmailStatus&) const = default;
    };

    struct AmazonBedrockCredentialSource {
        std::string value;

        static AmazonBedrockCredentialSource codexManaged();
        static AmazonBedrockCredentialSource awsManaged();
        [[nodiscard]] bool isKnown() const noexcept;

        auto operator<=>(const AmazonBedrockCredentialSource&) const = default;
    };

    struct AuthMode {
        std::string value;

        static AuthMode apiKey();
        static AuthMode chatgpt();
        static AuthMode chatgptAuthTokens();
        static AuthMode headers();
        static AuthMode agentIdentity();
        static AuthMode personalAccessToken();
        static AuthMode bedrockApiKey();
        [[nodiscard]] bool isKnown() const noexcept;

        auto operator<=>(const AuthMode&) const = default;
    };

    struct CancelLoginAccountStatus {
        std::string value;

        static CancelLoginAccountStatus canceled();
        static CancelLoginAccountStatus notFound();
        [[nodiscard]] bool isKnown() const noexcept;

        auto operator<=>(const CancelLoginAccountStatus&) const = default;
    };

    struct ConsumeAccountRateLimitResetCreditOutcome {
        std::string value;

        static ConsumeAccountRateLimitResetCreditOutcome reset();
        static ConsumeAccountRateLimitResetCreditOutcome nothingToReset();
        static ConsumeAccountRateLimitResetCreditOutcome noCredit();
        static ConsumeAccountRateLimitResetCreditOutcome alreadyRedeemed();
        [[nodiscard]] bool isKnown() const noexcept;

        auto operator<=>(const ConsumeAccountRateLimitResetCreditOutcome&) const = default;
    };

    struct LoginAppBrand {
        std::string value;

        static LoginAppBrand codex();
        static LoginAppBrand chatgpt();
        [[nodiscard]] bool isKnown() const noexcept;

        auto operator<=>(const LoginAppBrand&) const = default;
    };

    struct PlanType {
        std::string value;

        static PlanType free();
        static PlanType go();
        static PlanType plus();
        static PlanType pro();
        static PlanType proLite();
        static PlanType team();
        static PlanType selfServeBusinessUsageBased();
        static PlanType business();
        static PlanType enterpriseCbpUsageBased();
        static PlanType enterprise();
        static PlanType edu();
        static PlanType unknown();
        [[nodiscard]] bool isKnown() const noexcept;

        auto operator<=>(const PlanType&) const = default;
    };

    struct RateLimitReachedType {
        std::string value;

        static RateLimitReachedType rateLimitReached();
        static RateLimitReachedType workspaceOwnerCreditsDepleted();
        static RateLimitReachedType workspaceMemberCreditsDepleted();
        static RateLimitReachedType workspaceOwnerUsageLimitReached();
        static RateLimitReachedType workspaceMemberUsageLimitReached();
        [[nodiscard]] bool isKnown() const noexcept;

        auto operator<=>(const RateLimitReachedType&) const = default;
    };

    struct RateLimitResetCreditStatus {
        std::string value;

        static RateLimitResetCreditStatus available();
        static RateLimitResetCreditStatus redeeming();
        static RateLimitResetCreditStatus redeemed();
        static RateLimitResetCreditStatus unknown();
        [[nodiscard]] bool isKnown() const noexcept;

        auto operator<=>(const RateLimitResetCreditStatus&) const = default;
    };

    struct RateLimitResetType {
        std::string value;

        static RateLimitResetType codexRateLimits();
        static RateLimitResetType unknown();
        [[nodiscard]] bool isKnown() const noexcept;

        auto operator<=>(const RateLimitResetType&) const = default;
    };

    struct WorkspaceMessageType {
        std::string value;

        static WorkspaceMessageType headline();
        static WorkspaceMessageType announcement();
        static WorkspaceMessageType unknown();
        [[nodiscard]] bool isKnown() const noexcept;

        auto operator<=>(const WorkspaceMessageType&) const = default;
    };

    struct ChatgptAuthTokensRefreshParams {
        OptionalNullable<AccountId> previousAccountId;
        ChatgptAuthTokensRefreshReason reason;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const ChatgptAuthTokensRefreshParams&) const = default;
    };

    struct ChatgptAuthTokensRefreshResponse {
        std::string accessToken;
        AccountId chatgptAccountId;
        OptionalNullable<PlanType> chatgptPlanType;

        bool operator==(const ChatgptAuthTokensRefreshResponse&) const = default;
    };

    struct ApiKeyAccount {
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const ApiKeyAccount&) const = default;
    };

    struct ChatgptAccount {
        std::optional<std::string> email;
        PlanType planType;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const ChatgptAccount&) const = default;
    };

    struct AmazonBedrockAccount {
        AmazonBedrockCredentialSource credentialSource = AmazonBedrockCredentialSource::awsManaged();
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const AmazonBedrockAccount&) const = default;
    };

    struct UnknownAccount {
        std::optional<std::string> type;
        Json raw = Json::object();
        std::optional<DecodeDiagnostic> diagnostic;

        bool operator==(const UnknownAccount&) const = default;
    };

    using Account = std::variant<ApiKeyAccount, ChatgptAccount, AmazonBedrockAccount, UnknownAccount>;

    [[nodiscard]] inline std::string accountDiscriminator(const Account& account) {
        return std::visit(
            [](const auto& alternative) -> std::string {
                using Alternative = std::decay_t<decltype(alternative)>;
                if constexpr (std::is_same_v<Alternative, ApiKeyAccount>) {
                    return "apiKey";
                } else if constexpr (std::is_same_v<Alternative, ChatgptAccount>) {
                    return "chatgpt";
                } else if constexpr (std::is_same_v<Alternative, AmazonBedrockAccount>) {
                    return "amazonBedrock";
                } else {
                    return alternative.type.value_or(std::string{});
                }
            },
            account);
    }

    [[nodiscard]] inline const Json& accountRaw(const Account& account) noexcept {
        return std::visit(
            [](const auto& alternative) -> const Json& {
                return alternative.raw;
            },
            account);
    }

    struct ApiKeyLoginAccountParams {
        std::string apiKey;

        bool operator==(const ApiKeyLoginAccountParams&) const = default;
    };

    struct ChatgptLoginAccountParams {
        OptionalNullable<LoginAppBrand> appBrand;
        std::optional<bool> codexStreamlinedLogin;
        std::optional<bool> useHostedLoginSuccessPage;

        bool operator==(const ChatgptLoginAccountParams&) const = default;
    };

    struct ChatgptDeviceCodeLoginAccountParams {
        bool operator==(const ChatgptDeviceCodeLoginAccountParams&) const = default;
    };

    struct ChatgptAuthTokensLoginAccountParams {
        std::string accessToken;
        AccountId chatgptAccountId;
        OptionalNullable<PlanType> chatgptPlanType;

        bool operator==(const ChatgptAuthTokensLoginAccountParams&) const = default;
    };

    // A future outgoing discriminator cannot be safely encoded without a
    // reviewed schema. The encoder rejects this alternative synchronously.
    struct UnknownLoginAccountParams {
        std::optional<std::string> type;

        bool operator==(const UnknownLoginAccountParams&) const = default;
    };

    using LoginAccountParams = std::variant<ApiKeyLoginAccountParams,
                                            ChatgptLoginAccountParams,
                                            ChatgptDeviceCodeLoginAccountParams,
                                            ChatgptAuthTokensLoginAccountParams,
                                            UnknownLoginAccountParams>;

    [[nodiscard]] inline std::string loginAccountParamsDiscriminator(const LoginAccountParams& params) {
        return std::visit(
            [](const auto& alternative) -> std::string {
                using Alternative = std::decay_t<decltype(alternative)>;
                if constexpr (std::is_same_v<Alternative, ApiKeyLoginAccountParams>) {
                    return "apiKey";
                } else if constexpr (std::is_same_v<Alternative, ChatgptLoginAccountParams>) {
                    return "chatgpt";
                } else if constexpr (std::is_same_v<Alternative, ChatgptDeviceCodeLoginAccountParams>) {
                    return "chatgptDeviceCode";
                } else if constexpr (std::is_same_v<Alternative, ChatgptAuthTokensLoginAccountParams>) {
                    return "chatgptAuthTokens";
                } else {
                    return alternative.type.value_or(std::string{});
                }
            },
            params);
    }

    struct ApiKeyLoginAccountResponse {
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const ApiKeyLoginAccountResponse&) const = default;
    };

    struct ChatgptLoginAccountResponse {
        std::string authUrl;
        LoginId loginId;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const ChatgptLoginAccountResponse&) const = default;
    };

    struct ChatgptDeviceCodeLoginAccountResponse {
        LoginId loginId;
        std::string userCode;
        std::string verificationUrl;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const ChatgptDeviceCodeLoginAccountResponse&) const = default;
    };

    struct ChatgptAuthTokensLoginAccountResponse {
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const ChatgptAuthTokensLoginAccountResponse&) const = default;
    };

    struct UnknownLoginAccountResponse {
        std::optional<std::string> type;
        Json raw = Json::object();
        std::optional<DecodeDiagnostic> diagnostic;

        bool operator==(const UnknownLoginAccountResponse&) const = default;
    };

    using LoginAccountResponse = std::variant<ApiKeyLoginAccountResponse,
                                              ChatgptLoginAccountResponse,
                                              ChatgptDeviceCodeLoginAccountResponse,
                                              ChatgptAuthTokensLoginAccountResponse,
                                              UnknownLoginAccountResponse>;

    [[nodiscard]] inline std::string loginAccountResponseDiscriminator(const LoginAccountResponse& response) {
        return std::visit(
            [](const auto& alternative) -> std::string {
                using Alternative = std::decay_t<decltype(alternative)>;
                if constexpr (std::is_same_v<Alternative, ApiKeyLoginAccountResponse>) {
                    return "apiKey";
                } else if constexpr (std::is_same_v<Alternative, ChatgptLoginAccountResponse>) {
                    return "chatgpt";
                } else if constexpr (std::is_same_v<Alternative, ChatgptDeviceCodeLoginAccountResponse>) {
                    return "chatgptDeviceCode";
                } else if constexpr (std::is_same_v<Alternative, ChatgptAuthTokensLoginAccountResponse>) {
                    return "chatgptAuthTokens";
                } else {
                    return alternative.type.value_or(std::string{});
                }
            },
            response);
    }

    [[nodiscard]] inline const Json& loginAccountResponseRaw(const LoginAccountResponse& response) noexcept {
        return std::visit(
            [](const auto& alternative) -> const Json& {
                return alternative.raw;
            },
            response);
    }

    struct AccountLoginCompletedNotification {
        OptionalNullable<std::string> error;
        OptionalNullable<LoginId> loginId;
        bool success = false;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const AccountLoginCompletedNotification&) const = default;
    };

    struct RateLimitWindow {
        OptionalNullable<std::int64_t> resetsAt;
        std::int32_t usedPercent = 0;
        OptionalNullable<std::int64_t> windowDurationMins;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const RateLimitWindow&) const = default;
    };

    struct CreditsSnapshot {
        OptionalNullable<std::string> balance;
        bool hasCredits = false;
        bool unlimited = false;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const CreditsSnapshot&) const = default;
    };

    struct SpendControlLimitSnapshot {
        std::string limit;
        std::int32_t remainingPercent = 0;
        std::int64_t resetsAt = 0;
        std::string used;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const SpendControlLimitSnapshot&) const = default;
    };

    struct RateLimitSnapshot {
        OptionalNullable<CreditsSnapshot> credits;
        OptionalNullable<SpendControlLimitSnapshot> individualLimit;
        OptionalNullable<RateLimitId> limitId;
        OptionalNullable<std::string> limitName;
        OptionalNullable<PlanType> planType;
        OptionalNullable<RateLimitWindow> primary;
        OptionalNullable<RateLimitReachedType> rateLimitReachedType;
        OptionalNullable<RateLimitWindow> secondary;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const RateLimitSnapshot&) const = default;
    };

    struct AccountRateLimitsUpdatedNotification {
        RateLimitSnapshot rateLimits;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const AccountRateLimitsUpdatedNotification&) const = default;
    };

    struct AccountUpdatedNotification {
        OptionalNullable<AuthMode> authMode;
        OptionalNullable<PlanType> planType;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const AccountUpdatedNotification&) const = default;
    };

    struct AccountTokenUsageDailyBucket {
        std::string startDate;
        std::int64_t tokens = 0;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const AccountTokenUsageDailyBucket&) const = default;
    };

    struct AccountTokenUsageSummary {
        OptionalNullable<std::int64_t> currentStreakDays;
        OptionalNullable<std::int64_t> lifetimeTokens;
        OptionalNullable<std::int64_t> longestRunningTurnSec;
        OptionalNullable<std::int64_t> longestStreakDays;
        OptionalNullable<std::int64_t> peakDailyTokens;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const AccountTokenUsageSummary&) const = default;
    };

    struct CancelLoginAccountParams {
        LoginId loginId;

        bool operator==(const CancelLoginAccountParams&) const = default;
    };

    struct CancelLoginAccountResponse {
        CancelLoginAccountStatus status;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const CancelLoginAccountResponse&) const = default;
    };

    struct ConsumeAccountRateLimitResetCreditParams {
        OptionalNullable<RateLimitResetCreditId> creditId;
        IdempotencyKey idempotencyKey;

        bool operator==(const ConsumeAccountRateLimitResetCreditParams&) const = default;
    };

    struct ConsumeAccountRateLimitResetCreditResponse {
        ConsumeAccountRateLimitResetCreditOutcome outcome;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const ConsumeAccountRateLimitResetCreditResponse&) const = default;
    };

    struct GetAccountParams {
        std::optional<bool> refreshToken;

        bool operator==(const GetAccountParams&) const = default;
    };

    struct RateLimitResetCredit {
        OptionalNullable<std::string> description;
        OptionalNullable<std::int64_t> expiresAt;
        std::int64_t grantedAt = 0;
        RateLimitResetCreditId id;
        RateLimitResetType resetType;
        RateLimitResetCreditStatus status;
        OptionalNullable<std::string> title;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const RateLimitResetCredit&) const = default;
    };

    struct RateLimitResetCreditsSummary {
        std::int64_t availableCount = 0;
        OptionalNullable<std::vector<RateLimitResetCredit>> credits;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const RateLimitResetCreditsSummary&) const = default;
    };

    struct GetAccountRateLimitsResponse {
        OptionalNullable<RateLimitResetCreditsSummary> rateLimitResetCredits;
        RateLimitSnapshot rateLimits;
        OptionalNullable<std::map<RateLimitId, RateLimitSnapshot>> rateLimitsByLimitId;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const GetAccountRateLimitsResponse&) const = default;
    };

    struct GetAccountResponse {
        OptionalNullable<Account> account;
        bool requiresOpenaiAuth = false;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const GetAccountResponse&) const = default;
    };

    struct GetAccountTokenUsageResponse {
        OptionalNullable<std::vector<AccountTokenUsageDailyBucket>> dailyUsageBuckets;
        AccountTokenUsageSummary summary;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const GetAccountTokenUsageResponse&) const = default;
    };

    struct WorkspaceMessage {
        OptionalNullable<std::int64_t> archivedAt;
        OptionalNullable<std::int64_t> createdAt;
        std::string messageBody;
        WorkspaceMessageId messageId;
        WorkspaceMessageType messageType;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const WorkspaceMessage&) const = default;
    };

    struct GetWorkspaceMessagesResponse {
        bool featureEnabled = false;
        std::vector<WorkspaceMessage> messages;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const GetWorkspaceMessagesResponse&) const = default;
    };

    struct SendAddCreditsNudgeEmailParams {
        AddCreditsNudgeCreditType creditType;

        bool operator==(const SendAddCreditsNudgeEmailParams&) const = default;
    };

    struct SendAddCreditsNudgeEmailResponse {
        AddCreditsNudgeEmailStatus status;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const SendAddCreditsNudgeEmailResponse&) const = default;
    };

    class Accounts {
    public:
        using Submission = AppServerClient::RawProtocol::Submission;
        using UnitResultHandler = std::function<void(const OperationResult<Unit>&)>;
        using CancelLoginResultHandler = std::function<void(const OperationResult<CancelLoginAccountResponse>&)>;
        using StartLoginResultHandler = std::function<void(const OperationResult<LoginAccountResponse>&)>;
        using ConsumeRateLimitResetCreditResultHandler =
            std::function<void(const OperationResult<ConsumeAccountRateLimitResetCreditResponse>&)>;
        using ReadRateLimitsResultHandler = std::function<void(const OperationResult<GetAccountRateLimitsResponse>&)>;
        using ReadResultHandler = std::function<void(const OperationResult<GetAccountResponse>&)>;
        using SendAddCreditsNudgeEmailResultHandler = std::function<void(const OperationResult<SendAddCreditsNudgeEmailResponse>&)>;
        using ReadUsageResultHandler = std::function<void(const OperationResult<GetAccountTokenUsageResponse>&)>;
        using ReadWorkspaceMessagesResultHandler = std::function<void(const OperationResult<GetWorkspaceMessagesResponse>&)>;

        Submission cancelLogin(CancelLoginAccountParams params, CancelLoginResultHandler handler);
        Submission startLogin(LoginAccountParams params, StartLoginResultHandler handler);
        Submission logout(Unit params, UnitResultHandler handler);
        Submission consumeRateLimitResetCredit(ConsumeAccountRateLimitResetCreditParams params,
                                               ConsumeRateLimitResetCreditResultHandler handler);
        Submission readRateLimits(Unit params, ReadRateLimitsResultHandler handler);
        Submission read(GetAccountParams params, ReadResultHandler handler);
        Submission sendAddCreditsNudgeEmail(SendAddCreditsNudgeEmailParams params, SendAddCreditsNudgeEmailResultHandler handler);
        Submission readUsage(Unit params, ReadUsageResultHandler handler);
        Submission readWorkspaceMessages(Unit params, ReadWorkspaceMessagesResultHandler handler);

    private:
        friend class ::ai::openai::codex::AppServerClient;

        explicit Accounts(AppServerClient::RawProtocol& protocol) noexcept;

        AppServerClient::RawProtocol* protocol;
    };

} // namespace ai::openai::codex::typed

#endif // AI_OPENAI_CODEX_TYPED_ACCOUNTS_H
