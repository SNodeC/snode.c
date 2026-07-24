/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include <ai/openai/codex/AppServerClient.h>
#include <ai/openai/codex/stdio/Client.h>
#include <ai/openai/codex/typed/Accounts.h>
#include <ai/openai/codex/typed/Client.h>
#include <ai/openai/codex/typed/Configuration.h>
#include <ai/openai/codex/typed/Conversation.h>
#include <ai/openai/codex/typed/Events.h>
#include <ai/openai/codex/typed/Items.h>
#include <ai/openai/codex/typed/Models.h>
#include <ai/openai/codex/typed/Results.h>
#include <ai/openai/codex/typed/ServerRequests.h>
#include <ai/openai/codex/typed/Threads.h>
#include <ai/openai/codex/typed/Turns.h>
#include <iostream>
#include <map>
#include <type_traits>
#include <utility>
#include <variant>

int main() {
    namespace typed = ai::openai::codex::typed;

    static_assert(std::variant_size_v<typed::CodexErrorInfo> == 17);
    static_assert(std::variant_size_v<typed::AskForApproval> == 3);
    static_assert(std::variant_size_v<typed::CommandAction> == 5);
    static_assert(std::variant_size_v<typed::DynamicToolCallOutputContentItem> == 3);
    static_assert(std::variant_size_v<typed::PatchChangeKind> == 4);
    static_assert(std::variant_size_v<typed::SandboxPolicy> == 5);
    static_assert(std::variant_size_v<typed::UserInput> == 6);
    static_assert(std::variant_size_v<typed::WebSearchAction> == 5);
    static_assert(std::variant_size_v<typed::ThreadItem> == 19);
    static_assert(std::variant_size_v<typed::ResponseItem> == 17);
    static_assert(std::variant_size_v<typed::CanonicalServerNotification> == 44);
    static_assert(std::variant_size_v<typed::Account> == 4);
    static_assert(std::variant_size_v<typed::LoginAccountParams> == 5);
    static_assert(std::variant_size_v<typed::LoginAccountResponse> == 5);
    static_assert(std::variant_size_v<typed::ConfigLayerSource> == 9);
    static_assert(std::is_same_v<typed::Item, typed::ThreadItem>);
    static_assert(!std::is_same_v<typed::ThreadItem, typed::ResponseItem>);
    static_assert(std::is_same_v<typed::TurnInput, typed::UserInput>);
    static_assert(std::is_same_v<typed::TurnInterruptResult, typed::Unit>);
    static_assert(std::is_same_v<typed::ChatgptAuthTokensRefreshRequest, typed::AuthenticationRequest>);
    static_assert(!std::is_same_v<typed::SessionId, typed::ThreadId>);
    static_assert(!std::is_same_v<typed::AccountId, std::string>);
    static_assert(!std::is_same_v<typed::ClientUserMessageId, std::string>);
    static_assert(!std::is_same_v<typed::ModelServiceTierId, std::string>);
    static_assert(!std::is_same_v<typed::ConfigKeyPath, std::string>);
    static_assert(sizeof(ai::openai::codex::AppServerClient) == 2 * sizeof(void*));
    static_assert(sizeof(typed::Client) == sizeof(void*));

    [[maybe_unused]] typed::DecodeDiagnostic diagnostic{
        typed::DecodeIssueKind::UnknownDiscriminator,
        typed::DecodeIssueSeverity::ForwardCompatibility,
        "CodexErrorInfo",
        "$",
        "unrecognized protocol discriminator was retained as raw JSON"};
    [[maybe_unused]] typed::OperationResult<typed::Thread> operationResult;
    [[maybe_unused]] typed::OptionalNullable<std::string> omitted = typed::OptionalNullable<std::string>::omitted();
    [[maybe_unused]] typed::OptionalNullable<std::string> explicitNull = typed::OptionalNullable<std::string>::explicitNull();
    [[maybe_unused]] typed::AskForApproval approval =
        typed::GranularAskForApproval{{.mcpElicitations = true, .rules = true, .sandboxApproval = false}};
    [[maybe_unused]] typed::CommandAction command =
        typed::ReadCommandAction{.command = "cat README.md", .name = "cat", .path = {"/tmp/README.md"}};
    [[maybe_unused]] typed::DynamicToolCallOutputContentItem dynamicOutput =
        typed::InputTextDynamicToolCallOutputContentItem{.text = "done"};
    [[maybe_unused]] typed::PatchChangeKind patch =
        typed::UpdatePatchChangeKind{.movePath = typed::OptionalNullable<std::string>::explicitNull()};
    [[maybe_unused]] typed::SandboxPolicy sandbox =
        typed::WorkspaceWriteSandboxPolicy{.writableRoots = std::vector<typed::AbsolutePathBuf>{{"/tmp"}}};
    [[maybe_unused]] typed::UserInput userInput =
        typed::TextUserInput{.text = "Describe this directory.", .textElements = std::vector<typed::TextElement>{}};
    [[maybe_unused]] typed::WebSearchAction web =
        typed::OpenPageWebSearchAction{.url = typed::OptionalNullable<std::string>::withValue("https://example.test")};
    [[maybe_unused]] typed::UnknownUserInput futureInput{
        .type = "futureInput",
        .raw = {{"type", "futureInput"}},
        .diagnostic = diagnostic,
    };
    [[maybe_unused]] typed::Unit unit;
    typed::UnknownItem futureThreadItem;
    futureThreadItem.type = "futureThreadItem";
    futureThreadItem.raw = {{"type", "futureThreadItem"}};
    futureThreadItem.diagnostic = diagnostic;
    typed::UnknownResponseItem futureResponseItem;
    futureResponseItem.type = "future_response_item";
    futureResponseItem.raw = {{"type", "future_response_item"}};
    futureResponseItem.diagnostic = diagnostic;
    [[maybe_unused]] typed::ThreadItem installedThreadItem = futureThreadItem;
    [[maybe_unused]] typed::ResponseItem installedResponseItem = futureResponseItem;

    typed::Thread installedThread;
    installedThread.id = typed::ThreadId{"thread-installed"};
    installedThread.cwd = typed::AbsolutePathBuf{"/tmp"};
    installedThread.status = typed::IdleThreadStatus{};
    installedThread.source = typed::SessionSourceKind::cli();
    installedThread.sessionId = typed::SessionId{"app-server-session"};
    installedThread.raw = {{"id", "thread-installed"}};
    typed::Turn installedTurn;
    installedTurn.id = typed::TurnId{"turn-installed"};
    installedTurn.threadId = installedThread.id;
    installedTurn.status = typed::TurnStatus::inProgress();
    installedTurn.items.emplace_back(futureThreadItem);
    installedTurn.raw = {{"id", "turn-installed"}};
    installedThread.turns.emplace_back(installedTurn);
    [[maybe_unused]] typed::ThreadStartedNotification canonicalThreadStarted{
        .thread = installedThread,
        .raw = {{"thread", installedThread.raw}},
    };
    [[maybe_unused]] typed::Event installedEvent = typed::ThreadArchivedNotification{
        .threadId = installedThread.id,
        .raw = {{"threadId", installedThread.id.value}},
    };
    [[maybe_unused]] typed::UnknownEvent futureEvent{
        .method = "thread/future",
        .params = ai::openai::codex::Json::object(),
        .raw = {{"method", "thread/future"}},
        .diagnostic = diagnostic,
    };
    [[maybe_unused]] typed::LoginAccountParams apiKeyLogin =
        typed::ApiKeyLoginAccountParams{.apiKey = "installed-test-api-key"};
    [[maybe_unused]] typed::LoginAccountParams futureLoginParams =
        typed::UnknownLoginAccountParams{.type = "futureLogin"};
    [[maybe_unused]] typed::LoginAccountResponse futureLoginResponse =
        typed::UnknownLoginAccountResponse{
            .type = "futureLogin",
            .raw = {{"type", "futureLogin"}},
            .diagnostic = diagnostic,
        };
    [[maybe_unused]] typed::Account futureAccount = typed::UnknownAccount{
        .type = "futureAccount",
        .raw = {{"type", "futureAccount"}},
        .diagnostic = diagnostic,
    };
    [[maybe_unused]] typed::GetAccountResponse installedAccountRead{
        .account = typed::OptionalNullable<typed::Account>::withValue(futureAccount),
        .requiresOpenaiAuth = true,
        .raw = {{"account", {{"type", "futureAccount"}}}, {"requiresOpenaiAuth", true}},
    };
    [[maybe_unused]] typed::GetAccountRateLimitsResponse installedRateLimits{
        .rateLimitResetCredits = typed::OptionalNullable<typed::RateLimitResetCreditsSummary>::omitted(),
        .rateLimits = {},
        .rateLimitsByLimitId =
            typed::OptionalNullable<std::map<typed::RateLimitId, typed::RateLimitSnapshot>>::explicitNull(),
        .raw = {{"rateLimitsByLimitId", nullptr}},
    };
    [[maybe_unused]] typed::ChatgptAuthTokensRefreshParams installedRefreshParams{
        .previousAccountId = typed::OptionalNullable<typed::AccountId>::explicitNull(),
        .reason = typed::ChatgptAuthTokensRefreshReason::unauthorized(),
        .raw = {{"previousAccountId", nullptr}, {"reason", "unauthorized"}},
    };
    [[maybe_unused]] typed::ChatgptAuthTokensRefreshResponse installedRefreshResponse{
        .accessToken = "installed-test-access-token",
        .chatgptAccountId = typed::AccountId{"installed-account"},
        .chatgptPlanType = typed::OptionalNullable<typed::PlanType>::withValue(typed::PlanType::plus()),
    };
    [[maybe_unused]] typed::AuthenticationResponse legacyRefreshResponse{
        .accessToken = "installed-legacy-access-token",
        .chatgptAccountId = "installed-account",
        .chatgptPlanType = std::nullopt,
    };
    [[maybe_unused]] typed::ModelListParams installedModelListParams{
        .cursor = typed::OptionalNullable<std::string>::explicitNull(),
        .includeHidden = typed::OptionalNullable<bool>::withValue(false),
        .limit = typed::OptionalNullable<std::uint32_t>::withValue(25),
    };
    [[maybe_unused]] typed::InputModality futureInputModality{"future-modality"};
    [[maybe_unused]] typed::ModelRerouteReason futureRerouteReason{"future-reroute"};
    [[maybe_unused]] typed::ModelVerification futureVerification{"future-verification"};
    [[maybe_unused]] typed::Model installedModel{
        .additionalSpeedTiers = {typed::ModelServiceTierId{"fast"}},
        .availabilityNux = typed::OptionalNullable<typed::ModelAvailabilityNux>::withValue(
            typed::ModelAvailabilityNux{.message = "Available"}),
        .defaultReasoningEffort = typed::ReasoningEffort{"medium"},
        .defaultServiceTier =
            typed::OptionalNullable<typed::ModelServiceTierId>::withValue(typed::ModelServiceTierId{"standard"}),
        .description = "Installed model",
        .displayName = "Installed Model",
        .hidden = false,
        .id = typed::ModelId{"installed-model-id"},
        .inputModalities = {typed::InputModality::text(), futureInputModality},
        .isDefault = true,
        .model = typed::ModelId{"installed-model-wire"},
        .serviceTiers =
            {typed::ModelServiceTier{
                .description = "Standard service",
                .id = typed::ModelServiceTierId{"standard"},
                .name = "Standard",
            }},
        .supportedReasoningEfforts =
            {typed::ReasoningEffortOption{
                .description = "Balanced",
                .reasoningEffort = typed::ReasoningEffort{"medium"},
            }},
        .supportsPersonality = true,
        .upgrade = typed::OptionalNullable<std::string>::explicitNull(),
        .upgradeInfo = typed::OptionalNullable<typed::ModelUpgradeInfo>::omitted(),
    };
    [[maybe_unused]] typed::ModelListResponse installedModelListResponse{
        .data = {installedModel},
        .nextCursor = typed::OptionalNullable<std::string>::explicitNull(),
        .raw = {{"data", ai::openai::codex::Json::array()}, {"nextCursor", nullptr}},
    };
    [[maybe_unused]] typed::ModelProviderCapabilitiesReadResponse installedProviderCapabilities{
        .imageGeneration = true,
        .namespaceTools = false,
        .webSearch = true,
        .raw = {{"imageGeneration", true}, {"namespaceTools", false}, {"webSearch", true}},
    };
    [[maybe_unused]] typed::ConfigLayerSource installedConfigLayerSource =
        typed::UserConfigLayerSource{
            .file = typed::AbsolutePathBuf{"/synthetic/config.toml"},
            .profile = typed::OptionalNullable<std::string>::explicitNull(),
        };
    [[maybe_unused]] typed::ConfigLayerSource futureConfigLayerSource =
        typed::UnknownConfigLayerSource{
            .discriminator = "futureLayer",
            .raw = {{"type", "futureLayer"}},
            .diagnostic = diagnostic,
        };
    [[maybe_unused]] typed::ConfigReadParams installedConfigReadParams{
        .cwd = typed::OptionalNullable<std::string>::withValue("/synthetic/project"),
        .includeLayers = true,
    };
    [[maybe_unused]] typed::ConfigReadResponse installedConfigReadResponse{
        .config =
            typed::Config{
                .model = typed::OptionalNullable<typed::ModelId>::withValue(
                    typed::ModelId{"installed-model"}),
                .modelVerbosity =
                    typed::OptionalNullable<typed::Verbosity>::withValue(
                        typed::Verbosity::medium()),
                .webSearch =
                    typed::OptionalNullable<typed::WebSearchMode>::withValue(
                        typed::WebSearchMode::cached()),
            },
        .layers = typed::OptionalNullable<std::vector<typed::ConfigLayer>>::
            explicitNull(),
        .origins = {},
        .raw = {{"config", ai::openai::codex::Json::object()},
                {"layers", nullptr},
                {"origins", ai::openai::codex::Json::object()}},
    };
    [[maybe_unused]] typed::ConfigRequirementsReadResponse
        installedConfigRequirements{
            .requirements =
                typed::OptionalNullable<typed::ConfigRequirements>::withValue(
                    typed::ConfigRequirements{
                        .allowRemoteControl =
                            typed::OptionalNullable<bool>::withValue(false),
                        .allowedWebSearchModes = typed::OptionalNullable<
                            std::vector<typed::WebSearchMode>>::withValue(
                            {typed::WebSearchMode::disabled()}),
                    }),
            .raw = {{"requirements", ai::openai::codex::Json::object()}},
        };
    typed::TurnStartParams installedStartParams;
    installedStartParams.threadId = installedThread.id;
    installedStartParams.clientUserMessageId = typed::ClientUserMessageId{"client-message"};

    ai::openai::codex::stdio::Client client;
    client.typed().events().setOnEvent([](const typed::Event& event) {
        std::visit(
            [](const auto& value) {
                using Value = std::decay_t<decltype(value)>;
                if constexpr (std::is_same_v<Value, typed::AgentMessageDelta>) {
                    std::cout << value.text;
                }
            },
            event);
    });
    client.typed().requests().setOnRequest([&client](const typed::TypedServerRequest& request) {
        if (const auto* approval = std::get_if<typed::CommandApprovalRequest>(&request)) {
            (void) client.typed().requests().respond(*approval, typed::ApprovalDecision::decline());
        } else if (const auto* authentication =
                       std::get_if<typed::AuthenticationRequest>(&request)) {
            if (authentication->canonicalParams.previousAccountId.isOmitted()) {
                // Preserve the pre-A1 source form: braced AuthenticationResponse
                // must select the legacy respond overload unambiguously.
                (void) client.typed().requests().respond(
                    *authentication,
                    {"installed-legacy-access-token",
                     "installed-account",
                     std::nullopt});
            } else {
                (void) client.typed().requests().respondRefresh(
                    *authentication,
                    typed::ChatgptAuthTokensRefreshResponse{
                        .accessToken = "installed-test-access-token",
                        .chatgptAccountId =
                            typed::AccountId{"installed-account"},
                        .chatgptPlanType =
                            typed::OptionalNullable<typed::PlanType>::
                                withValue(typed::PlanType::plus()),
                    });
            }
        }
    });

    const auto archiveSubmission = client.typed().threads().archive(
        {.threadId = installedThread.id}, [](const typed::OperationResult<typed::Unit>&) {});
    const auto goalSubmission = client.typed().threads().getGoal(
        {.threadId = installedThread.id}, [](const typed::OperationResult<typed::ThreadGoalGetResponse>&) {});
    const auto steerSubmission = client.typed().turns().steer(
        {.threadId = installedThread.id,
         .expectedTurnId = installedTurn.id,
         .input = {typed::TextUserInput{.text = "Steer the current turn."}}},
        [](const typed::OperationResult<typed::TurnSteerResponse>&) {});
    (void) archiveSubmission;
    (void) goalSubmission;
    (void) steerSubmission;

    const auto cancelLoginSubmission = client.typed().accounts().cancelLogin(
        {.loginId = typed::LoginId{"installed-login"}},
        [](const typed::OperationResult<typed::CancelLoginAccountResponse>&) {});
    const auto startLoginSubmission = client.typed().accounts().startLogin(
        typed::ApiKeyLoginAccountParams{.apiKey = "installed-test-api-key"},
        [](const typed::OperationResult<typed::LoginAccountResponse>&) {});
    const auto logoutSubmission =
        client.typed().accounts().logout({}, [](const typed::OperationResult<typed::Unit>&) {});
    const auto consumeCreditSubmission = client.typed().accounts().consumeRateLimitResetCredit(
        {.creditId = typed::OptionalNullable<typed::RateLimitResetCreditId>::explicitNull(),
         .idempotencyKey = typed::IdempotencyKey{"installed-idempotency"}},
        [](const typed::OperationResult<typed::ConsumeAccountRateLimitResetCreditResponse>&) {});
    const auto readRateLimitsSubmission = client.typed().accounts().readRateLimits(
        {}, [](const typed::OperationResult<typed::GetAccountRateLimitsResponse>&) {});
    const auto readAccountSubmission = client.typed().accounts().read(
        {.refreshToken = true}, [](const typed::OperationResult<typed::GetAccountResponse>&) {});
    const auto nudgeSubmission = client.typed().accounts().sendAddCreditsNudgeEmail(
        {.creditType = typed::AddCreditsNudgeCreditType::credits()},
        [](const typed::OperationResult<typed::SendAddCreditsNudgeEmailResponse>&) {});
    const auto readUsageSubmission = client.typed().accounts().readUsage(
        {}, [](const typed::OperationResult<typed::GetAccountTokenUsageResponse>&) {});
    const auto readMessagesSubmission = client.typed().accounts().readWorkspaceMessages(
        {}, [](const typed::OperationResult<typed::GetWorkspaceMessagesResponse>&) {});
    (void) cancelLoginSubmission;
    (void) startLoginSubmission;
    (void) logoutSubmission;
    (void) consumeCreditSubmission;
    (void) readRateLimitsSubmission;
    (void) readAccountSubmission;
    (void) nudgeSubmission;
    (void) readUsageSubmission;
    (void) readMessagesSubmission;

    const auto modelListSubmission = client.typed().models().list(
        std::move(installedModelListParams),
        [](const typed::OperationResult<typed::ModelListResponse>&) {});
    const auto providerCapabilitiesSubmission = client.typed().models().readProviderCapabilities(
        {}, [](const typed::OperationResult<typed::ModelProviderCapabilitiesReadResponse>&) {});
    (void) modelListSubmission;
    (void) providerCapabilitiesSubmission;

    const auto configReadSubmission = client.typed().configuration().read(
        std::move(installedConfigReadParams),
        [](const typed::OperationResult<typed::ConfigReadResponse>&) {});
    const auto configRequirementsSubmission =
        client.typed().configuration().readRequirements(
            {},
            [](const typed::OperationResult<
                typed::ConfigRequirementsReadResponse>&) {});
    (void) configReadSubmission;
    (void) configRequirementsSubmission;

    typed::ThreadStartParams launchParams;
    launchParams.cwd = typed::OptionalNullable<std::string>::withValue("/tmp");
    (void) client.typed().threads().start(
        std::move(launchParams), [&client](const typed::OperationResult<typed::ThreadStartResponse>& result) {
            if (result) {
                typed::TurnStartParams turnParams;
                turnParams.threadId = result.value->thread.id;
                turnParams.input.emplace_back(typed::TextUserInput{.text = "Describe this directory."});
                (void) client.typed().turns().start(
                    std::move(turnParams), [](const typed::OperationResult<typed::TurnStartResponse>&) {});
            }
        });

    return 0;
}
