/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include <ai/openai/codex/stdio/Client.h>
#include <ai/openai/codex/typed/Client.h>
#include <ai/openai/codex/typed/Conversation.h>
#include <optional>
#include <type_traits>
#include <variant>
#include <vector>

int main() {
    namespace typed = ai::openai::codex::typed;

    static_assert(std::is_same_v<typed::TextInput, typed::TextUserInput>);
    static_assert(std::is_same_v<typed::ImageUrlInput, typed::ImageUserInput>);
    static_assert(std::is_same_v<typed::LocalImageInput, typed::LocalImageUserInput>);
    static_assert(std::is_same_v<typed::SkillInput, typed::SkillUserInput>);
    static_assert(std::is_same_v<typed::MentionInput, typed::MentionUserInput>);
    static_assert(std::is_same_v<typed::UnknownTurnInput, typed::UnknownUserInput>);
    static_assert(std::is_same_v<typed::TurnInput, typed::UserInput>);
    static_assert(std::is_same_v<typed::ExternalSandboxPolicy, typed::ExternalSandboxSandboxPolicy>);

    [[maybe_unused]] typed::TurnInput legacyInput =
        typed::ImageUrlInput{.url = "https://example.test/image", .detail = std::optional{typed::ImageDetail::high()}};
    [[maybe_unused]] typed::SandboxPolicy legacySandbox = typed::WorkspaceWriteSandboxPolicy::fromLegacy({"/tmp"}, false, false, false);

    ai::openai::codex::stdio::Client client;
    const ai::openai::codex::AppServerClient& constClient = client;

    const bool sameObjects = &client.threads() == &client.typed().threads() &&
                             &client.turns() == &client.typed().turns() &&
                             &client.events() == &client.typed().events() &&
                             &client.requests() == &client.typed().requests() &&
                             &constClient.threads() == &constClient.typed().threads() &&
                             &constClient.turns() == &constClient.typed().turns() &&
                             &constClient.events() == &constClient.typed().events() &&
                             &constClient.requests() == &constClient.typed().requests();

    const typed::ThreadId threadId{"thread-compatibility"};
    const typed::TurnId turnId{"turn-compatibility"};
    const auto threadHandler = [](const typed::OperationResult<typed::Thread>&) {};
    const auto pageHandler = [](const typed::OperationResult<typed::ThreadListResponse>&) {};
    const auto turnHandler = [](const typed::OperationResult<typed::Turn>&) {};
    const auto unitHandler = [](const typed::OperationResult<typed::Unit>&) {};
    const auto rollbackHandler = [](const typed::OperationResult<typed::ThreadRollbackResponse>&) {};

    (void) client.threads().start(typed::ThreadStartOptions{}, threadHandler);
    (void) client.threads().resume(threadId, typed::ThreadResumeOptions{}, threadHandler);
    (void) client.threads().list(typed::ThreadListOptions{}, pageHandler);
    (void) client.threads().read(threadId, typed::ThreadReadOptions{}, threadHandler);
    (void) client.turns().start(
        threadId, {typed::TextInput{.text = "Compatibility input."}}, typed::TurnStartOptions{}, turnHandler);
    (void) client.turns().interrupt(threadId, turnId, unitHandler);
    (void) client.threads().rollback({.threadId = threadId, .numTurns = 1}, rollbackHandler);

    return sameObjects ? 0 : 1;
}
