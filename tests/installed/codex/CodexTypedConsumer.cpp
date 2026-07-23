/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include <ai/openai/codex/stdio/Client.h>
#include <ai/openai/codex/typed/Client.h>
#include <iostream>
#include <type_traits>
#include <variant>

int main() {
    namespace typed = ai::openai::codex::typed;

    static_assert(std::variant_size_v<typed::CodexErrorInfo> == 17);
    static_assert(std::is_same_v<typed::Item, typed::ThreadItem>);
    static_assert(!std::is_same_v<typed::ThreadItem, typed::ResponseItem>);

    [[maybe_unused]] typed::DecodeDiagnostic diagnostic{
        typed::DecodeIssueKind::UnknownDiscriminator,
        typed::DecodeIssueSeverity::ForwardCompatibility,
        "CodexErrorInfo",
        "$",
        "unrecognized protocol discriminator was retained as raw JSON"};
    [[maybe_unused]] typed::OperationResult<typed::Thread> operationResult;

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
