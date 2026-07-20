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

    ai::openai::codex::stdio::Client client;
    client.events().setOnEvent([](const typed::Event& event) {
        std::visit(
            [](const auto& value) {
                using Value = std::decay_t<decltype(value)>;
                if constexpr (std::is_same_v<Value, typed::AgentMessageDelta>) {
                    std::cout << value.text;
                }
            },
            event);
    });
    client.requests().setOnRequest([&client](const typed::TypedServerRequest& request) {
        if (const auto* approval = std::get_if<typed::CommandApprovalRequest>(&request)) {
            (void) client.requests().respond(*approval, typed::ApprovalDecision::decline());
        }
    });

    (void) client.threads().start({.cwd = "/tmp"}, [&client](const typed::OperationResult<typed::Thread>& result) {
        if (result) {
            (void) client.turns().start(
                result.value->id, {typed::TextInput{"Describe this directory."}}, {}, [](const typed::OperationResult<typed::Turn>&) {
                });
        }
    });

    return 0;
}
