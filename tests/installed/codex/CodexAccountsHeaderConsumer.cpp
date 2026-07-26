// Compile the installed public header twice in one otherwise isolated
// translation unit to prove conventional include-guard behavior.
// clang-format off
#include <ai/openai/codex/typed/Accounts.h>
#include <ai/openai/codex/typed/Accounts.h>
// clang-format on

#include <type_traits>

int main() {
    namespace typed = ai::openai::codex::typed;

    using CancelLogin =
        typed::Accounts::Submission (typed::Accounts::*)(typed::CancelLoginAccountParams, typed::Accounts::CancelLoginResultHandler);
    using StartLogin =
        typed::Accounts::Submission (typed::Accounts::*)(typed::LoginAccountParams, typed::Accounts::StartLoginResultHandler);
    using Logout = typed::Accounts::Submission (typed::Accounts::*)(typed::Unit, typed::Accounts::UnitResultHandler);
    using ConsumeCredit = typed::Accounts::Submission (typed::Accounts::*)(typed::ConsumeAccountRateLimitResetCreditParams,
                                                                           typed::Accounts::ConsumeRateLimitResetCreditResultHandler);
    using ReadRateLimits = typed::Accounts::Submission (typed::Accounts::*)(typed::Unit, typed::Accounts::ReadRateLimitsResultHandler);
    using ReadAccount = typed::Accounts::Submission (typed::Accounts::*)(typed::GetAccountParams, typed::Accounts::ReadResultHandler);
    using SendNudge = typed::Accounts::Submission (typed::Accounts::*)(typed::SendAddCreditsNudgeEmailParams,
                                                                       typed::Accounts::SendAddCreditsNudgeEmailResultHandler);
    using ReadUsage = typed::Accounts::Submission (typed::Accounts::*)(typed::Unit, typed::Accounts::ReadUsageResultHandler);
    using ReadMessages = typed::Accounts::Submission (typed::Accounts::*)(typed::Unit, typed::Accounts::ReadWorkspaceMessagesResultHandler);

    static_assert(std::is_same_v<decltype(&typed::Accounts::cancelLogin), CancelLogin>);
    static_assert(std::is_same_v<decltype(&typed::Accounts::startLogin), StartLogin>);
    static_assert(std::is_same_v<decltype(&typed::Accounts::logout), Logout>);
    static_assert(std::is_same_v<decltype(&typed::Accounts::consumeRateLimitResetCredit), ConsumeCredit>);
    static_assert(std::is_same_v<decltype(&typed::Accounts::readRateLimits), ReadRateLimits>);
    static_assert(std::is_same_v<decltype(&typed::Accounts::read), ReadAccount>);
    static_assert(std::is_same_v<decltype(&typed::Accounts::sendAddCreditsNudgeEmail), SendNudge>);
    static_assert(std::is_same_v<decltype(&typed::Accounts::readUsage), ReadUsage>);
    static_assert(std::is_same_v<decltype(&typed::Accounts::readWorkspaceMessages), ReadMessages>);

    const typed::ChatgptAuthTokensRefreshParams refresh{
        .previousAccountId = typed::OptionalNullable<typed::AccountId>::explicitNull(),
        .reason = typed::ChatgptAuthTokensRefreshReason::unauthorized(),
    };
    return refresh.previousAccountId.isNull() ? 0 : 1;
}
