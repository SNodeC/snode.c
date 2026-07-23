/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include "ai/openai/codex/AppServerClient.h"
#include "ai/openai/codex/detail/Transport.h"
#include "ai/openai/codex/typed/Client.h"
#include "support/TestResult.h"

#include <memory>
#include <string>
#include <type_traits>
#include <utility>

namespace {
    namespace codex = ai::openai::codex;

    class InertTransport final : public codex::detail::Transport {
    public:
        void setCallbacks(codex::detail::TransportCallbacks) override {
        }

        void start() override {
        }

        bool send(std::string) override {
            return false;
        }

        void stop() override {
        }
    };

    class TestClient final : public codex::AppServerClient {
    public:
        TestClient()
            : AppServerClient(std::make_unique<InertTransport>(), {"typed_facade_test", "Typed Facade Test", "1"}) {
        }
    };
} // namespace

int main() {
    static_assert(!std::is_copy_constructible_v<codex::typed::Client>);
    static_assert(!std::is_move_constructible_v<codex::typed::Client>);
    static_assert(sizeof(codex::typed::Client) == sizeof(void*));
    static_assert(sizeof(codex::AppServerClient) == 2 * sizeof(void*));
    static_assert(std::is_same_v<decltype(std::declval<codex::AppServerClient&>().typed()), codex::typed::Client&>);
    static_assert(std::is_same_v<decltype(std::declval<const codex::AppServerClient&>().typed()), const codex::typed::Client&>);

    tests::support::TestResult result;
    TestClient client;
    const codex::AppServerClient& constClient = client;

    result.expectTrue(&client.typed() == &client.typed(), "non-const typed() returns one stable grouped facade");
    result.expectTrue(&constClient.typed() == &client.typed(), "const typed() returns the same grouped facade");

    result.expectTrue(&client.threads() == &client.typed().threads(),
                      "deprecated threads() forwards to the grouped facade object");
    result.expectTrue(&client.turns() == &client.typed().turns(),
                      "deprecated turns() forwards to the grouped facade object");
    result.expectTrue(&client.events() == &client.typed().events(),
                      "deprecated events() forwards to the grouped facade object");
    result.expectTrue(&client.requests() == &client.typed().requests(),
                      "deprecated requests() forwards to the grouped facade object");

    result.expectTrue(&constClient.threads() == &constClient.typed().threads(),
                      "deprecated const threads() forwards to the grouped facade object");
    result.expectTrue(&constClient.turns() == &constClient.typed().turns(),
                      "deprecated const turns() forwards to the grouped facade object");
    result.expectTrue(&constClient.events() == &constClient.typed().events(),
                      "deprecated const events() forwards to the grouped facade object");
    result.expectTrue(&constClient.requests() == &constClient.typed().requests(),
                      "deprecated const requests() forwards to the grouped facade object");

    return result.processResult();
}
