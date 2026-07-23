/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include <ai/openai/codex/stdio/Client.h>
#include <ai/openai/codex/typed/Client.h>
#include <ai/openai/codex/typed/Conversation.h>
#include <iostream>
#include <type_traits>
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
    static_assert(std::is_same_v<typed::Item, typed::ThreadItem>);
    static_assert(!std::is_same_v<typed::ThreadItem, typed::ResponseItem>);
    static_assert(std::is_same_v<typed::TurnInput, typed::UserInput>);
    static_assert(!std::is_same_v<typed::ClientUserMessageId, std::string>);

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

    (void) client.typed().threads().start({.cwd = "/tmp"}, [&client](const typed::OperationResult<typed::Thread>& result) {
        if (result) {
            (void) client.typed().turns().start(
                result.value->id, {typed::TextInput{"Describe this directory."}}, {}, [](const typed::OperationResult<typed::Turn>&) {
                });
        }
    });

    return 0;
}
