/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include <ai/openai/codex/AppServerClient.h>
#include <ai/openai/codex/stdio/Client.h>
#include <ai/openai/codex/typed/Client.h>
#include <ai/openai/codex/typed/Conversation.h>
#include <ai/openai/codex/typed/Events.h>
#include <ai/openai/codex/typed/Items.h>
#include <ai/openai/codex/typed/Results.h>
#include <ai/openai/codex/typed/Threads.h>
#include <ai/openai/codex/typed/Turns.h>
#include <iostream>
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
    static_assert(std::variant_size_v<typed::CanonicalServerNotification> == 37);
    static_assert(std::is_same_v<typed::Item, typed::ThreadItem>);
    static_assert(!std::is_same_v<typed::ThreadItem, typed::ResponseItem>);
    static_assert(std::is_same_v<typed::TurnInput, typed::UserInput>);
    static_assert(std::is_same_v<typed::TurnInterruptResult, typed::Unit>);
    static_assert(!std::is_same_v<typed::SessionId, typed::ThreadId>);
    static_assert(!std::is_same_v<typed::ClientUserMessageId, std::string>);
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
